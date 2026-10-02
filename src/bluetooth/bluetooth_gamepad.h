/*
 * Gamepad Bluetooth (BLE)
 *
 * Faz a placa funcionar como um controle Bluetooth Low Energy, usando o
 * perfil HID over GATT, que o Windows reconhece como controle de jogo.
 */
#ifndef BLUETOOTH_GAMEPAD_H
#define BLUETOOTH_GAMEPAD_H

#include <stdbool.h>
#include <stdint.h>

// Configura os serviços BLE. Deve ser chamada uma vez, depois de
// cyw43_arch_init(). O rádio continua desligado até bluetooth_ligar(true)
void bluetooth_iniciar(void);

// Liga (anuncia e aceita conexões) ou desliga o rádio Bluetooth
void bluetooth_ligar(bool ligar);

// Indica se há um PC conectado e pronto para receber o estado do controle
bool bluetooth_conectado(void);

// Envia um relatório HID ao PC. Retorna false se não houver PC conectado
bool bluetooth_enviar_relatorio(const void *relatorio, uint16_t tamanho);

#endif
