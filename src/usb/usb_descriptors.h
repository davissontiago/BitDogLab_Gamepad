/*
 * Descritores USB
 *
 * Exporta o formato do relatório HID para o Bluetooth. Este arquivo não
 * inclui o TinyUSB nem o BTstack, porque as duas bibliotecas definem tipos com
 * o mesmo nome e não podem ser usadas juntas no mesmo arquivo .c
 */
#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <stdint.h>

// Formato do relatório HID para o Bluetooth. É o mesmo do USB, mas com
// "Report ID 1", exigido pelo serviço HID do BLE. Fica no usb_descriptors.c
// porque só lá é possível usar as macros do TinyUSB que montam o descritor
extern const uint8_t desc_hid_report_ble[];
extern const uint16_t desc_hid_report_ble_tamanho;

#endif
