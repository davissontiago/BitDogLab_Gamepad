/*
 * Identificação da placa
 *
 * Gera o nome, o número de série, o ID curto e o PID a partir do ID único do
 * chip (pico_unique_id).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/unique_id.h"
#include "identificacao.h"

static char numero_serie[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];
static char nome_produto[32];
static const char *id_placa;
static uint16_t pid;

void identificacao_iniciar(void)
{
    // O ID vem em texto hexadecimal (16 dígitos); os 4 últimos identificam a placa
    pico_get_unique_board_id_string(numero_serie, sizeof(numero_serie));
    id_placa = &numero_serie[strlen(numero_serie) - 4];
    snprintf(nome_produto, sizeof(nome_produto), "BitDogLab Gamepad %s", id_placa);
    pid = (uint16_t)strtoul(id_placa, NULL, 16);
}

const char *obter_nome_produto(void)
{
    return nome_produto;
}

const char *obter_id_placa(void)
{
    return id_placa;
}

const char *obter_numero_serie(void)
{
    return numero_serie;
}

uint16_t obter_pid(void)
{
    return pid;
}
