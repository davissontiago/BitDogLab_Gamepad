/*
 * Controle físico
 *
 * Lê o joystick analógico e os botões da BitDogLab e monta o relatório HID
 * enviado ao PC.
 */

// Bibliotecas usadas:
// stdlib.h -> abs() para calcular a distância do joystick ao centro
// pico/stdlib.h -> GPIO, temporização e funções básicas do Pico SDK
// hardware/adc.h -> conversor analógico-digital usado para ler o joystick
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "controle.h"

// ---------------------------------------------------------------------------
// Pinos e constantes
// ---------------------------------------------------------------------------

// O joystick da BitDogLab tem dois potenciômetros (eixos X e Y), ligados às
// entradas analógicas GPIO 27 (ADC1) e GPIO 26 (ADC0), e um botão de clique
// ligado ao GPIO 22
const int JOYSTICK_X = 27;
const int JOYSTICK_Y = 26;
const int BOTAO_JOYSTICK = 22;
const int ADC_CANAL_X = 1;
const int ADC_CANAL_Y = 0;

// O ADC do RP2040 tem 12 bits, então as leituras vão de 0 a 4095
#define ADC_MAX ((1 << 12) - 1)

// Zona morta: movimentos menores que isso em torno do centro são ignorados,
// para que o joystick parado não gere movimento por causa de ruído
#define ZONA_MORTA 200

// Limiares do "pedal": empurrar o joystick além de LIMIAR_PEDAL aciona o
// acelerador (ou o freio), que só é solto quando o joystick volta para menos
// de LIMIAR_PEDAL_SOLTAR. Essa diferença entre os dois valores (histerese)
// evita que o botão fique ligando e desligando perto do limite
#define LIMIAR_PEDAL 600
#define LIMIAR_PEDAL_SOLTAR 450

// Quantidade de leituras usadas para descobrir o centro do joystick ao ligar
#define AMOSTRAS_CALIBRACAO 32

// Botões A e B da BitDogLab
#define BUTTON_A 5
#define BUTTON_B 6

// ---------------------------------------------------------------------------
// Estado
// ---------------------------------------------------------------------------

// Posição de repouso de cada eixo, medida na calibração
static uint16_t centro_x;
static uint16_t centro_y;

// Lembram se o acelerador/freio estão acionados, necessário para a histerese
static bool acelerando = false;
static bool freando = false;

// ---------------------------------------------------------------------------
// Configuração do hardware
// ---------------------------------------------------------------------------

// Liga o ADC, prepara os pinos dos eixos como entradas analógicas e configura
// o botão do joystick como entrada com pull-up (fica em 1 solto e 0 apertado)
static void configurar_joystick()
{
    adc_init();
    adc_gpio_init(JOYSTICK_X);
    adc_gpio_init(JOYSTICK_Y);
    gpio_init(BOTAO_JOYSTICK);
    gpio_set_dir(BOTAO_JOYSTICK, GPIO_IN);
    gpio_pull_up(BOTAO_JOYSTICK);
}

// Configura os botões A e B como entradas com pull-up. Os botões ligam o pino
// ao GND quando apertados, então a leitura é 0 quando pressionados
static void configurar_botoes()
{
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);
}

// ---------------------------------------------------------------------------
// Leitura do joystick
// ---------------------------------------------------------------------------

// Lê os dois eixos do joystick. O ADC tem um único conversor, então é preciso
// selecionar o canal de cada eixo antes de cada leitura
static void ler_eixos_joystick(uint16_t *x, uint16_t *y)
{
    adc_select_input(ADC_CANAL_X);
    *x = adc_read();
    adc_select_input(ADC_CANAL_Y);
    *y = adc_read();
}

// Descobre a posição de repouso do joystick tirando a média de várias
// leituras. Cada joystick tem um centro um pouco diferente, então medir em
// vez de assumir o meio do ADC deixa o controle mais preciso.
// Por isso o joystick não deve ser tocado enquanto a placa liga
static void calibrar_centro()
{
    uint32_t soma_x = 0, soma_y = 0;
    for (int i = 0; i < AMOSTRAS_CALIBRACAO; i++)
    {
        uint16_t x, y;
        ler_eixos_joystick(&x, &y);
        soma_x += x;
        soma_y += y;
        sleep_ms(1);
    }
    centro_x = soma_x / AMOSTRAS_CALIBRACAO;
    centro_y = soma_y / AMOSTRAS_CALIBRACAO;
}

// Converte uma leitura do ADC (0 a 4095) para a escala de um eixo de gamepad
// (-127 a 127), onde 0 é o joystick parado
static int8_t converter_eixo(uint16_t valor, uint16_t centro)
{
    // Distância da leitura até o centro; dentro da zona morta, conta como parado
    int desvio = (int)valor - centro;
    if (abs(desvio) < ZONA_MORTA)
        return 0;

    // Fora da zona morta, a distância restante é reescalada para -127..127.
    // Cada lado usa sua própria faixa, porque o centro calibrado nem sempre
    // fica exatamente no meio: assim os dois lados alcançam o valor máximo
    int eixo;
    if (desvio > 0)
        eixo = (desvio - ZONA_MORTA) * 127 / (ADC_MAX - centro - ZONA_MORTA);
    else
        eixo = (desvio + ZONA_MORTA) * 127 / (centro - ZONA_MORTA);

    // Garante que o resultado não passe dos limites do eixo
    if (eixo > 127)
        eixo = 127;
    if (eixo < -127)
        eixo = -127;
    return (int8_t)eixo;
}

// ---------------------------------------------------------------------------
// Botões
// ---------------------------------------------------------------------------

// Monta o conjunto de botões do gamepad, em que cada bit representa um botão.
// Recebe a leitura do eixo Y porque empurrar o joystick para frente/trás
// também funciona como acelerador/freio
static uint32_t ler_botoes(uint16_t y)
{
    uint32_t botoes = 0;
    int desvio_y = (int)y - centro_y; // Positivo = joystick para frente

    // Histerese: se o pedal já está acionado, usa o limiar menor para soltar;
    // se está solto, exige passar do limiar maior para acionar
    acelerando = desvio_y > (acelerando ? LIMIAR_PEDAL_SOLTAR : LIMIAR_PEDAL);
    freando = desvio_y < -(freando ? LIMIAR_PEDAL_SOLTAR : LIMIAR_PEDAL);

    // Mapeamento (nome no Project64 -> botão sugerido no N64):
    // Button 0 (joystick para frente) -> livre, ex.: R (pular/derrapar)
    // Button 1 (joystick para trás) -> B (frear/ré)
    // Button 2 (botão B) -> Z (item)
    // Button 3 (botão A) -> A (acelerar)
    // Button 4 (clique do joystick) -> Start.
    // Os botões físicos são lidos com "!" porque 0 significa apertado
    if (acelerando)
        botoes |= GAMEPAD_BUTTON_0;
    if (freando)
        botoes |= GAMEPAD_BUTTON_1;
    if (!gpio_get(BUTTON_B))
        botoes |= GAMEPAD_BUTTON_2;
    if (!gpio_get(BUTTON_A))
        botoes |= GAMEPAD_BUTTON_3;
    if (!gpio_get(BOTAO_JOYSTICK))
        botoes |= GAMEPAD_BUTTON_4;
    return botoes;
}

// ---------------------------------------------------------------------------
// Funções públicas
// ---------------------------------------------------------------------------

void controle_iniciar(void)
{
    configurar_joystick();
    configurar_botoes();
    calibrar_centro();
}

void controle_ler(hid_gamepad_report_t *relatorio)
{
    uint16_t x, y;
    ler_eixos_joystick(&x, &y);

    // Monta o relatório HID: eixos analógicos, direcional (hat) e botões.
    // Os dois eixos são invertidos para seguir o padrão de gamepad USB, em que
    // direita é positivo e cima é negativo. Na BitDogLab, o eixo X dá valores
    // maiores para a esquerda e o eixo Y dá valores maiores para cima.
    // Os demais eixos do relatório (z, rz, rx, ry) ficam em 0, pois não são usados
    *relatorio = (hid_gamepad_report_t){0};
    relatorio->x = -converter_eixo(x, centro_x);
    relatorio->y = -converter_eixo(y, centro_y);
    relatorio->hat = GAMEPAD_HAT_CENTERED;
    relatorio->buttons = ler_botoes(y);
}
