/*
 * Display OLED da BitDogLab
 *
 * Driver simples para o display SSD1306 de 128x64 pixels, ligado ao I2C1
 * (SDA no GPIO 14 e SCL no GPIO 15). Os desenhos são feitos em um buffer na
 * memória e só aparecem no display ao chamar tela_atualizar().
 * A fonte não tem acentos.
 */
#ifndef TELA_H
#define TELA_H

#include <stdint.h>

#define TELA_LARGURA 128
#define TELA_ALTURA 64

// Configura o I2C e liga o display
void tela_iniciar(void);

// Apaga o buffer (não muda o display até tela_atualizar)
void tela_limpar(void);

// Escreve um texto centralizado na horizontal. "y" é a linha de pixels do
// topo do texto, e "escala" aumenta o tamanho (1 = 6x8 pixels por letra)
void tela_escrever_centralizado(int y, const char *texto, int escala);

// Desenha uma linha horizontal na altura "y", de ponta a ponta
void tela_linha_horizontal(int y);

// Envia o buffer para o display (leva cerca de 25 ms)
void tela_atualizar(void);

#endif
