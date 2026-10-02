/*
 * BitDogLab Gamepad
 *
 * Firmware que transforma a BitDogLab (Raspberry Pi Pico W) em um gamepad.
 * O joystick analógico e os botões da placa são lidos continuamente e enviados
 * ao computador como um controle de jogo padrão (HID), sem precisar de driver.
 * Com o cabo USB ligado a um PC, o controle funciona pelo cabo e o Bluetooth
 * fica desligado; sem o cabo, funciona por Bluetooth (BLE).
 * O mapeamento foi pensado para Mario Kart 64 no emulador Project64.
 *
 * Este arquivo coordena os módulos: escolhe entre USB e Bluetooth, envia o
 * estado do controle e atualiza o display.
 */

// Bibliotecas usadas:
// string.h -> memcmp() para comparar o relatório atual com o anterior
// pico/stdlib.h -> temporização e funções básicas do Pico SDK
// pico/cyw43_arch.h -> chip sem fio do Pico W (Bluetooth e detecção do cabo)
// tusb.h -> pilha USB TinyUSB, que faz a placa se comportar como gamepad
// controle.h -> leitura do joystick e dos botões (controle/)
// identificacao.h -> nome e ID da placa (identificacao/)
// bluetooth_gamepad.h -> gamepad Bluetooth (bluetooth/)
// tela.h -> display OLED (tela/)
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "tusb.h"
#include "controle.h"
#include "identificacao.h"
#include "bluetooth_gamepad.h"
#include "tela.h"

// ---------------------------------------------------------------------------
// Constantes
// ---------------------------------------------------------------------------

// De quantos em quantos milissegundos o estado do controle é verificado
#define INTERVALO_RELATORIO_MS 1

// De quantos em quantos milissegundos a presença do cabo é verificada
#define INTERVALO_VERIFICAR_CABO_MS 100

// Tempo que se espera o PC reconhecer a placa depois que o cabo é ligado.
// Se o cabo estiver ligado a um carregador (sem PC), o PC nunca reconhece a
// placa, e depois desse tempo o Bluetooth é ligado
#define TEMPO_ESPERA_USB_MS 3000

// ---------------------------------------------------------------------------
// Estado global
// ---------------------------------------------------------------------------

// Último relatório enviado ao PC. Serve para enviar um novo apenas quando
// algo mudar. forcar_envio obriga o próximo envio (ex.: logo após conectar)
hid_gamepad_report_t ultimo_relatorio;
bool forcar_envio = true;

// Meio de comunicação em uso: cabo USB ou Bluetooth
typedef enum
{
    MODO_INDEFINIDO,
    MODO_USB,
    MODO_BLUETOOTH
} modo_t;
modo_t modo = MODO_INDEFINIDO;

// Momento em que o cabo foi ligado, para contar o TEMPO_ESPERA_USB_MS
uint32_t inicio_cabo = 0;
bool cabo_anterior = false;

// O que está sendo mostrado no display, para redesenhar só quando mudar
modo_t modo_na_tela = MODO_INDEFINIDO;
bool conectado_na_tela = false;

// ---------------------------------------------------------------------------
// Envio ao computador
// ---------------------------------------------------------------------------

// Lê o estado atual do controle e envia ao PC, pelo meio em uso, se algo mudou
void enviar_estado_controle()
{
    // No USB, se o envio anterior ainda não foi entregue, tenta de novo na
    // próxima volta
    if (modo == MODO_USB && !tud_hid_ready())
        return;

    hid_gamepad_report_t relatorio;
    controle_ler(&relatorio);

    // O PC mantém o último estado recebido, então só é preciso enviar mudanças
    if (!forcar_envio && memcmp(&relatorio, &ultimo_relatorio, sizeof(relatorio)) == 0)
        return;

    // Envia pelo meio em uso. Guarda o relatório como "último enviado" apenas
    // se o envio deu certo (no Bluetooth, falha se nenhum PC estiver conectado)
    bool enviado = false;
    if (modo == MODO_USB)
        enviado = tud_hid_report(0, &relatorio, sizeof(relatorio));
    else if (modo == MODO_BLUETOOTH)
        enviado = bluetooth_enviar_relatorio(&relatorio, sizeof(relatorio));

    if (enviado)
    {
        ultimo_relatorio = relatorio;
        forcar_envio = false;
    }
}

// ---------------------------------------------------------------------------
// Escolha entre cabo e Bluetooth
// ---------------------------------------------------------------------------

// Verifica se o cabo está ligado a um PC e escolhe o meio de comunicação.
// A presença do cabo é lida no pino VBUS, que no Pico W fica no chip sem fio
// (CYW43). Só ter energia no cabo não basta: um carregador também fornece
// energia. Por isso o USB só é escolhido se o PC reconhecer a placa
// (tud_mounted), com um tempo de espera logo após o cabo ser ligado
void atualizar_modo()
{
    uint32_t agora = to_ms_since_boot(get_absolute_time());
    bool cabo = cyw43_arch_gpio_get(CYW43_WL_GPIO_VBUS_PIN);
    if (cabo && !cabo_anterior)
        inicio_cabo = agora;
    cabo_anterior = cabo;

    bool usar_usb = cabo && (tud_mounted() || agora - inicio_cabo < TEMPO_ESPERA_USB_MS);
    modo_t novo_modo = usar_usb ? MODO_USB : MODO_BLUETOOTH;
    if (novo_modo == modo)
        return;

    // Troca de meio: liga ou desliga o Bluetooth e reenvia o estado atual,
    // já que o novo meio ainda não recebeu nada
    modo = novo_modo;
    bluetooth_ligar(modo == MODO_BLUETOOTH);
    forcar_envio = true;
}

// ---------------------------------------------------------------------------
// Display
// ---------------------------------------------------------------------------

// Mostra no display a identificação da placa, o modo (USB ou Bluetooth) e se
// há um PC conectado. Só redesenha quando algo muda, porque cada envio ao
// display pausa o laço principal por cerca de 25 ms
void atualizar_tela()
{
    bool conectado = (modo == MODO_USB) ? tud_mounted() : bluetooth_conectado();
    if (modo == modo_na_tela && conectado == conectado_na_tela)
        return;
    modo_na_tela = modo;
    conectado_na_tela = conectado;

    // Layout (128x64): título, ID da placa em destaque, linha divisória, modo
    // em destaque e situação da conexão
    tela_limpar();
    tela_escrever_centralizado(0, "BitDogLab Gamepad", 1);
    tela_escrever_centralizado(11, obter_id_placa(), 2);
    tela_linha_horizontal(28);
    tela_escrever_centralizado(33, modo == MODO_USB ? "USB" : "Bluetooth", 2);
    tela_escrever_centralizado(55, conectado ? "Conectado" : "Desconectado", 1);
    tela_atualizar();
}

// ---------------------------------------------------------------------------
// Programa principal
// ---------------------------------------------------------------------------

int main()
{
    // Monta o nome e o PID da placa, usados pelo USB e pelo Bluetooth
    identificacao_iniciar();

    // Liga o display e avisa que a placa está iniciando (o joystick não deve
    // ser tocado durante a calibração)
    tela_iniciar();
    tela_escrever_centralizado(20, "Iniciando...", 1);
    tela_escrever_centralizado(36, "Nao mexa no joystick", 1);
    tela_atualizar();

    // Inicia o chip sem fio (necessário para o Bluetooth e para ler o VBUS)
    // e configura os serviços Bluetooth, que começam desligados
    cyw43_arch_init();
    bluetooth_iniciar();

    // Inicia a pilha USB e o controle (que mede o centro do joystick)
    tusb_init();
    controle_iniciar();

    // Laço infinito. tud_task() e cyw43_arch_poll() precisam rodar o tempo
    // todo para atender o USB e o Bluetooth. A cada INTERVALO_RELATORIO_MS o
    // controle é verificado, e a cada INTERVALO_VERIFICAR_CABO_MS o cabo e o
    // display
    uint32_t ultimo_envio = 0;
    uint32_t ultima_verificacao_cabo = 0;
    while (true)
    {
        tud_task();
        cyw43_arch_poll();

        uint32_t agora = to_ms_since_boot(get_absolute_time());
        if (modo == MODO_INDEFINIDO || agora - ultima_verificacao_cabo >= INTERVALO_VERIFICAR_CABO_MS)
        {
            ultima_verificacao_cabo = agora;
            atualizar_modo();
            atualizar_tela();
        }
        if (agora - ultimo_envio >= INTERVALO_RELATORIO_MS)
        {
            ultimo_envio = agora;
            enviar_estado_controle();
        }
    }
}

// ---------------------------------------------------------------------------
// Callbacks do TinyUSB
// ---------------------------------------------------------------------------

// Chamada quando a placa é conectada (reconhecida) pelo PC. Força o envio do
// estado atual, já que o PC ainda não sabe nada sobre o controle
void tud_mount_cb(void)
{
    forcar_envio = true;
}

// Chamada quando o PC sai do modo de suspensão. Reenvia o estado pelo mesmo motivo
void tud_resume_cb(void)
{
    forcar_envio = true;
}

// O TinyUSB exige estas duas funções, usadas quando o PC pede ou envia um
// relatório diretamente. Este controle não precisa delas, então elas só
// ignoram os parâmetros
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)bufsize;
}
