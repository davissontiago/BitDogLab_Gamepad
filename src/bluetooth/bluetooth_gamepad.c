/*
 * Gamepad Bluetooth (BLE)
 *
 * Implementa o controle via Bluetooth Low Energy com o BTstack, usando o
 * perfil HID over GATT (HOGP). Funcionamento:
 * 1. A placa anuncia que é um gamepad, com o mesmo nome usado no USB
 * 2. O PC conecta e pareia (o pareamento fica salvo na flash)
 * 3. O PC ativa as notificações do relatório HID
 * 4. A cada mudança no controle, o relatório é enviado como notificação
 *
 * Este arquivo não pode incluir o TinyUSB (tusb.h), pois as duas bibliotecas
 * definem tipos com o mesmo nome.
 */
#include <string.h>
#include "btstack.h"
#include "ble/gatt-service/device_information_service_server.h"
#include "ble/gatt-service/hids_device.h"
#include "gamepad_ble.h"
#include "bluetooth_gamepad.h"
#include "usb_descriptors.h"
#include "identificacao.h"

// Tamanho máximo do relatório HID guardado para envio
#define TAMANHO_MAX_RELATORIO 16

// Intervalo de anúncio, em unidades de 0,625 ms (0x30 = 30 ms): quanto menor,
// mais rápido o PC encontra a placa
#define INTERVALO_ANUNCIO 0x0030

// Intervalo de conexão pedido ao PC, em unidades de 1,25 ms (6 a 12 =
// 7,5 a 15 ms). Define de quanto em quanto tempo o controle pode enviar dados
#define INTERVALO_CONEXAO_MIN 6
#define INTERVALO_CONEXAO_MAX 12

// ---------------------------------------------------------------------------
// Estado da conexão
// ---------------------------------------------------------------------------

// Conexão atual com o PC (inválida quando ninguém está conectado)
static hci_con_handle_t conexao = HCI_CON_HANDLE_INVALID;

// Indica se o PC ativou as notificações do relatório, ou seja, se está pronto
// para receber o estado do controle
static bool notificacoes_ativas = false;

// Último relatório recebido para envio. O BTstack avisa quando é possível
// enviar (HIDS_SUBEVENT_CAN_SEND_NOW); nesse momento vai o relatório mais
// recente, mesmo que outros tenham sido gerados enquanto se esperava
static uint8_t relatorio_pendente[TAMANHO_MAX_RELATORIO];
static uint16_t tamanho_pendente;
static bool envio_solicitado = false;

static btstack_packet_callback_registration_t registro_hci;
static btstack_packet_callback_registration_t registro_sm;

// ---------------------------------------------------------------------------
// Dados de anúncio
// ---------------------------------------------------------------------------

// Pacote de anúncio: diz que a placa é um dispositivo BLE detectável, que
// oferece o serviço HID (0x1812) e que tem aparência de gamepad (0x03C4)
static uint8_t dados_anuncio[] = {
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, 0x06,
    0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS,
    ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE & 0xff, ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE >> 8,
    0x03, BLUETOOTH_DATA_TYPE_APPEARANCE, 0xC4, 0x03,
};

// Resposta ao escaneamento: o nome não cabe junto no pacote de anúncio
// (limite de 31 bytes), então é enviado quando o PC pede mais informações
static uint8_t dados_resposta[31];

// ---------------------------------------------------------------------------
// Eventos do Bluetooth
// ---------------------------------------------------------------------------

// Responde à leitura do nome do dispositivo (característica GAP_DEVICE_NAME),
// que é dinâmica porque depende do ID da placa
static uint16_t ler_atributo(hci_con_handle_t con_handle, uint16_t atributo, uint16_t offset,
                             uint8_t *buffer, uint16_t tamanho_buffer)
{
    UNUSED(con_handle);
    if (atributo == ATT_CHARACTERISTIC_GAP_DEVICE_NAME_01_VALUE_HANDLE)
    {
        const char *nome = obter_nome_produto();
        return att_read_callback_handle_blob((const uint8_t *)nome, strlen(nome), offset, buffer, tamanho_buffer);
    }
    return 0;
}

// Trata os eventos do BTstack: conexão, desconexão, pareamento e serviço HID
static void tratar_evento(uint8_t tipo_pacote, uint16_t canal, uint8_t *pacote, uint16_t tamanho)
{
    UNUSED(canal);
    UNUSED(tamanho);
    if (tipo_pacote != HCI_EVENT_PACKET)
        return;

    switch (hci_event_packet_get_type(pacote))
    {
    // PC desconectou: volta a anunciar automaticamente
    case HCI_EVENT_DISCONNECTION_COMPLETE:
        conexao = HCI_CON_HANDLE_INVALID;
        notificacoes_ativas = false;
        envio_solicitado = false;
        break;

    // Pareamento sem senha ("Just Works"): a placa não tem tela nem teclado,
    // então o pedido é aceito automaticamente
    case SM_EVENT_JUST_WORKS_REQUEST:
        sm_just_works_confirm(sm_event_just_works_request_get_handle(pacote));
        break;

    // Eventos do serviço HID
    case HCI_EVENT_HIDS_META:
        switch (hci_event_hids_meta_get_subevent_code(pacote))
        {
        // PC ativou (ou desativou) as notificações: a partir daqui pode enviar
        case HIDS_SUBEVENT_INPUT_REPORT_ENABLE:
            conexao = hids_subevent_input_report_enable_get_con_handle(pacote);
            notificacoes_ativas = hids_subevent_input_report_enable_get_enable(pacote);
            // Pede um intervalo de conexão curto, para o controle responder rápido
            gap_request_connection_parameter_update(conexao, INTERVALO_CONEXAO_MIN, INTERVALO_CONEXAO_MAX, 0, 200);
            break;

        // Momento liberado para enviar: manda o relatório mais recente
        case HIDS_SUBEVENT_CAN_SEND_NOW:
            envio_solicitado = false;
            if (notificacoes_ativas)
                hids_device_send_input_report(conexao, relatorio_pendente, tamanho_pendente);
            break;

        default:
            break;
        }
        break;

    default:
        break;
    }
}

// ---------------------------------------------------------------------------
// Funções públicas
// ---------------------------------------------------------------------------

void bluetooth_iniciar(void)
{
    // Camadas básicas: L2CAP (canais de dados) e SM (segurança e pareamento).
    // Pareamento com vínculo salvo (bonding) e conexão segura, sem senha
    l2cap_init();
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);

    // Servidor de atributos com os serviços definidos em gamepad_ble.gatt
    att_server_init(profile_data, ler_atributo, NULL);

    // Informações do dispositivo: fabricante e VID/PID (origem "USB" = 0x02),
    // os mesmos do modo cabo
    device_information_service_server_init();
    device_information_service_server_set_manufacturer_name("BitDogLab");
    device_information_service_server_set_pnp_id(0x02, GAMEPAD_VID, obter_pid(), 0x0100);

    // Serviço HID com o formato de relatório de gamepad
    hids_device_init(0, desc_hid_report_ble, desc_hid_report_ble_tamanho);

    // Anúncio: pacote principal e resposta ao escaneamento com o nome
    const char *nome = obter_nome_produto();
    uint8_t tamanho_nome = strlen(nome);
    if (tamanho_nome > sizeof(dados_resposta) - 2)
        tamanho_nome = sizeof(dados_resposta) - 2;
    dados_resposta[0] = tamanho_nome + 1;
    dados_resposta[1] = BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME;
    memcpy(&dados_resposta[2], nome, tamanho_nome);

    bd_addr_t endereco_nulo = {0};
    gap_advertisements_set_params(INTERVALO_ANUNCIO, INTERVALO_ANUNCIO, 0, 0, endereco_nulo, 0x07, 0x00);
    gap_advertisements_set_data(sizeof(dados_anuncio), dados_anuncio);
    gap_scan_response_set_data(tamanho_nome + 2, dados_resposta);
    gap_advertisements_enable(1);

    // Registra a função que recebe os eventos
    registro_hci.callback = &tratar_evento;
    hci_add_event_handler(&registro_hci);
    registro_sm.callback = &tratar_evento;
    sm_add_event_handler(&registro_sm);
    hids_device_register_packet_handler(tratar_evento);
}

void bluetooth_ligar(bool ligar)
{
    if (!ligar)
    {
        conexao = HCI_CON_HANDLE_INVALID;
        notificacoes_ativas = false;
        envio_solicitado = false;
    }
    hci_power_control(ligar ? HCI_POWER_ON : HCI_POWER_OFF);
}

bool bluetooth_conectado(void)
{
    return notificacoes_ativas;
}

bool bluetooth_enviar_relatorio(const void *relatorio, uint16_t tamanho)
{
    if (!notificacoes_ativas || tamanho > TAMANHO_MAX_RELATORIO)
        return false;

    // Guarda o relatório e pede ao BTstack para avisar quando puder enviar.
    // Se já houver um pedido em andamento, o relatório novo substitui o antigo
    memcpy(relatorio_pendente, relatorio, tamanho);
    tamanho_pendente = tamanho;
    if (!envio_solicitado)
    {
        envio_solicitado = true;
        hids_device_request_can_send_now_event(conexao);
    }
    return true;
}
