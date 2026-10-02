/*
 * Configuração do BTstack
 *
 * Define como a pilha Bluetooth deve funcionar neste projeto: apenas BLE
 * (Bluetooth Low Energy), no papel de periférico, ou seja, a placa é o
 * dispositivo e o PC se conecta a ela. O BTstack lê este arquivo durante a
 * compilação. Os valores de buffers seguem os exemplos oficiais do Pico W.
 */
#ifndef BTSTACK_CONFIG_H
#define BTSTACK_CONFIG_H

#ifndef ENABLE_BLE
#error "Este projeto precisa da biblioteca pico_btstack_ble"
#endif

// ---------------------------------------------------------------------------
// Recursos habilitados
// ---------------------------------------------------------------------------

// A placa atua como periférico BLE (anuncia e aceita conexões)
#define ENABLE_LE_PERIPHERAL

// Apenas mensagens de erro (o stdio está desativado, então não aparecem).
// ENABLE_PRINTF_HEXDUMP é exigido por um arquivo de depuração do SDK
#define ENABLE_LOG_ERROR
#define ENABLE_PRINTF_HEXDUMP

// Nenhum cliente GATT: a placa só oferece serviços, não consulta outros
#define MAX_NR_GATT_CLIENTS 0

// Pareamento seguro (LE Secure Connections), com a criptografia feita em
// software
#define ENABLE_LE_SECURE_CONNECTIONS
#define ENABLE_SOFTWARE_AES128
#define ENABLE_MICRO_ECC_FOR_LE_SECURE_CONNECTIONS

// ---------------------------------------------------------------------------
// Buffers e limites
// ---------------------------------------------------------------------------

// Tamanho dos pacotes de dados e alinhamento exigido pelo chip CYW43
#define HCI_OUTGOING_PRE_BUFFER_SIZE 4
#define HCI_ACL_PAYLOAD_SIZE (255 + 4)
#define HCI_ACL_CHUNK_SIZE_ALIGNMENT 4

// Apenas uma conexão por vez (um PC)
#define MAX_NR_HCI_CONNECTIONS 1
#define MAX_NR_SM_LOOKUP_ENTRIES 3
#define MAX_NR_WHITELIST_ENTRIES 16
#define MAX_NR_LE_DEVICE_DB_ENTRIES 16

// Limita os buffers usados no chip para não sobrecarregar o barramento
// compartilhado entre Wi-Fi e Bluetooth do CYW43
#define MAX_NR_CONTROLLER_ACL_BUFFERS 3
#define MAX_NR_CONTROLLER_SCO_PACKETS 3

// Controle de fluxo do chip para a placa, pelo mesmo motivo
#define ENABLE_HCI_CONTROLLER_TO_HOST_FLOW_CONTROL
#define HCI_HOST_ACL_PACKET_LEN (255 + 4)
#define HCI_HOST_ACL_PACKET_NUM 3
#define HCI_HOST_SCO_PACKET_LEN 120
#define HCI_HOST_SCO_PACKET_NUM 3

// ---------------------------------------------------------------------------
// Memória permanente
// ---------------------------------------------------------------------------

// Os pareamentos ficam salvos na flash, para o PC reconectar sozinho depois
// de a placa ser desligada. Cabem até 16 dispositivos pareados
#define NVM_NUM_DEVICE_DB_ENTRIES 16
#define NVM_NUM_LINK_KEYS 16

// Tamanho máximo do banco de atributos (serviços BLE)
#define MAX_ATT_DB_SIZE 512

// ---------------------------------------------------------------------------
// Integração com o Pico SDK
// ---------------------------------------------------------------------------

// O SDK fornece a contagem de tempo em milissegundos e o assert()
#define HAVE_EMBEDDED_TIME_MS
#define HAVE_ASSERT

// Tempo de espera pela resposta do chip ao reiniciar o Bluetooth
#define HCI_RESET_RESEND_TIMEOUT_MS 1000

#endif
