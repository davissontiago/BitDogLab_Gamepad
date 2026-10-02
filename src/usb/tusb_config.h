/*
 * Configuração do TinyUSB
 *
 * Define como a pilha USB deve funcionar neste projeto: a placa atua apenas
 * como dispositivo (não como host) e oferece somente a classe HID (gamepad).
 * O TinyUSB lê este arquivo durante a compilação.
 */
#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

// ---------------------------------------------------------------------------
// Configuração geral
// ---------------------------------------------------------------------------

// Microcontrolador usado: RP2040, o chip do Raspberry Pi Pico W
#ifndef CFG_TUSB_MCU
#define CFG_TUSB_MCU OPT_MCU_RP2040
#endif

// A porta USB funciona como dispositivo em Full Speed (12 Mbit/s)
#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)

// Ambiente de execução: Pico SDK, sem sistema operacional de tempo real
#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS OPT_OS_PICO
#endif

// Seção de memória e alinhamento dos buffers USB (alinhados em 4 bytes,
// como o controlador USB do RP2040 espera)
#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif

#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))
#endif

// ---------------------------------------------------------------------------
// Configuração do dispositivo
// ---------------------------------------------------------------------------

// Tamanho máximo dos pacotes de controle (usados na enumeração)
#define CFG_TUD_ENDPOINT0_SIZE 64

// Classes USB habilitadas: apenas HID (gamepad). Serial (CDC), armazenamento
// (MSC), MIDI e classe de fabricante ficam desativados
#define CFG_TUD_HID 1
#define CFG_TUD_CDC 0
#define CFG_TUD_MSC 0
#define CFG_TUD_MIDI 0
#define CFG_TUD_VENDOR 0

// Tamanho do buffer do endpoint HID, suficiente para o relatório do gamepad
#define CFG_TUD_HID_EP_BUFSIZE 16

#endif
