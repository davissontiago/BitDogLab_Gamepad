/*
 * Descritores USB
 *
 * Quando a placa é conectada, o computador pergunta "o que é você?". As
 * respostas ficam nos descritores deste arquivo: eles dizem que a placa é um
 * gamepad, quais dados ela envia e qual nome deve aparecer no Windows.
 * O TinyUSB chama as funções tud_descriptor_*_cb para buscar cada descritor.
 * Nome, número de série e PID vêm do módulo de identificação.
 */
#include <string.h>
#include "tusb.h"
#include "usb_descriptors.h"
#include "identificacao.h"

// USB_BCD indica a versão do USB (2.0)
#define USB_BCD 0x0200

// ---------------------------------------------------------------------------
// Descritor do dispositivo
// ---------------------------------------------------------------------------

// Informações gerais do dispositivo. A classe fica 0 porque o tipo (HID) é
// definido na interface, mais abaixo. Os campos iManufacturer, iProduct e
// iSerialNumber apontam para posições da tabela de strings.
// Não é const porque o idProduct é preenchido em tempo de execução
tusb_desc_device_t desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = USB_BCD,
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = GAMEPAD_VID,
    .idProduct = 0,
    .bcdDevice = 0x0100,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01,
};

// Entrega o descritor do dispositivo quando o PC pede, já com o PID da placa
uint8_t const *tud_descriptor_device_cb(void)
{
    desc_device.idProduct = obter_pid();
    return (uint8_t const *)&desc_device;
}

// ---------------------------------------------------------------------------
// Descritor HID (formato do relatório)
// ---------------------------------------------------------------------------

// Descreve o formato dos dados enviados. Usa o modelo pronto de gamepad do
// TinyUSB: 6 eixos analógicos, um direcional (hat) e 32 botões, que é o mesmo
// formato da estrutura hid_gamepad_report_t usada no código principal
uint8_t const desc_hid_report[] = {
    TUD_HID_REPORT_DESC_GAMEPAD()};

// Mesmo formato para o Bluetooth, com o Report ID 1 que o serviço HID do BLE
// exige. Fica aqui porque só este arquivo pode usar as macros do TinyUSB
const uint8_t desc_hid_report_ble[] = {
    TUD_HID_REPORT_DESC_GAMEPAD(HID_REPORT_ID(1))};
const uint16_t desc_hid_report_ble_tamanho = sizeof(desc_hid_report_ble);

// Entrega o descritor HID quando o PC pede
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void)instance;
    return desc_hid_report;
}

// ---------------------------------------------------------------------------
// Descritor de configuração
// ---------------------------------------------------------------------------

// Lista das interfaces do dispositivo: apenas uma, a do gamepad (HID)
enum
{
    ITF_NUM_HID,
    ITF_NUM_TOTAL
};

// Tamanho total do descritor e endpoint usado para enviar dados ao PC
// (0x81 = endpoint 1, direção dispositivo -> PC)
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)
#define EPNUM_HID 0x81

// Configuração completa: consumo máximo de 100 mA e uma interface HID.
// O último parâmetro (1) faz o PC buscar dados a cada 1 ms, para o controle
// responder rápido no jogo
uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 1)};

// Entrega o descritor de configuração quando o PC pede
uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
    return desc_configuration;
}

// ---------------------------------------------------------------------------
// Strings (textos exibidos pelo sistema)
// ---------------------------------------------------------------------------

// Tabela de strings. A posição 0 é especial: informa o idioma (0x0409 = inglês
// EUA, o padrão esperado pelos sistemas). As demais são os textos em si:
// fabricante, produto e número de série. Produto e série são preenchidos em
// tud_descriptor_string_cb, pois dependem do ID da placa
char const *string_desc_arr[] = {
    (const char[]){0x09, 0x04},
    "BitDogLab",
    NULL,
    NULL,
};

// Buffer onde a string pedida é montada no formato USB
static uint16_t _desc_str[32];

// Entrega a string de número "index" quando o PC pede
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void)langid;
    uint8_t chr_count;

    string_desc_arr[2] = obter_nome_produto();
    string_desc_arr[3] = obter_numero_serie();

    if (index == 0)
    {
        // Índice 0: copia o código de idioma
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    }
    else
    {
        // Índice inexistente: responde que não há essa string
        if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0]))
            return NULL;

        // O USB usa texto em UTF-16 (2 bytes por caractere), então cada
        // caractere ASCII é copiado para uma posição de 16 bits.
        // O limite de 31 caracteres respeita o tamanho do buffer
        const char *str = string_desc_arr[index];
        chr_count = strlen(str);
        if (chr_count > 31)
            chr_count = 31;

        for (uint8_t i = 0; i < chr_count; i++)
            _desc_str[1 + i] = str[i];
    }

    // O primeiro elemento é o cabeçalho: tipo "string" e tamanho total em bytes
    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);
    return _desc_str;
}
