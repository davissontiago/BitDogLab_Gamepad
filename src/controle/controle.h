/*
 * Controle físico
 *
 * Leitura do joystick e dos botões da BitDogLab e montagem do relatório HID
 * (o "estado do controle" enviado ao PC).
 * Inclui o TinyUSB por causa do tipo hid_gamepad_report_t, então não pode ser
 * usado em arquivos que incluem o BTstack.
 */
#ifndef CONTROLE_H
#define CONTROLE_H

#include "tusb.h"

// Configura o joystick e os botões e mede o centro do joystick.
// O joystick não deve ser tocado enquanto esta função roda
void controle_iniciar(void);

// Lê o joystick e os botões e preenche o relatório HID
void controle_ler(hid_gamepad_report_t *relatorio);

#endif
