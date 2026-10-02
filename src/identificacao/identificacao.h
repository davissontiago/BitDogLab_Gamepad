/*
 * Identificação da placa
 *
 * Nome, número de série, ID e VID/PID usados tanto no USB quanto no Bluetooth
 * e mostrados no display. Tudo é gerado a partir do ID único do chip, para que
 * várias placas ligadas no mesmo PC não sejam confundidas.
 */
#ifndef IDENTIFICACAO_H
#define IDENTIFICACAO_H

#include <stdint.h>

// VID (fabricante): o de teste dos exemplos do TinyUSB
#define GAMEPAD_VID 0xCafe

// Lê o ID único do chip e monta o nome, o número de série e o PID.
// Deve ser chamada uma vez, no início do programa, antes de iniciar o USB
void identificacao_iniciar(void);

// Nome do controle, ex.: "BitDogLab Gamepad B729"
const char *obter_nome_produto(void);

// ID curto da placa: os 4 últimos dígitos do ID do chip, ex.: "B729"
const char *obter_id_placa(void);

// Número de série: o ID do chip completo, em hexadecimal (16 dígitos)
const char *obter_numero_serie(void);

// PID do controle: o ID curto lido como número hexadecimal, ex.: 0xB729.
// Precisa ser diferente em cada placa porque o Windows guarda o nome do
// controle pelo par VID/PID: com um PID fixo, todas as placas apareceriam com
// o nome da primeira que foi conectada
uint16_t obter_pid(void);

#endif
