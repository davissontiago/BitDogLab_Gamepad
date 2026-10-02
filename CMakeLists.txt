# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 1.5.1)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(BitDogLabGamepad C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# ---------------------------------------------------------------------------
# Executável
# ---------------------------------------------------------------------------

add_executable(BitDogLabGamepad
        src/main.c
        src/controle/controle.c
        src/identificacao/identificacao.c
        src/usb/usb_descriptors.c
        src/bluetooth/bluetooth_gamepad.c
        src/tela/tela.c
        )

pico_set_program_name(BitDogLabGamepad "BitDogLabGamepad")
pico_set_program_version(BitDogLabGamepad "0.1")

# stdio desativado: a porta USB é usada como gamepad (HID)
pico_enable_stdio_uart(BitDogLabGamepad 0)
pico_enable_stdio_usb(BitDogLabGamepad 0)

# Pastas dos módulos. tusb_config.h (src/usb) e btstack_config.h
# (src/bluetooth) também precisam estar aqui para o TinyUSB e o BTstack os
# encontrarem
target_include_directories(BitDogLabGamepad PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/src/controle
        ${CMAKE_CURRENT_LIST_DIR}/src/identificacao
        ${CMAKE_CURRENT_LIST_DIR}/src/usb
        ${CMAKE_CURRENT_LIST_DIR}/src/bluetooth
        ${CMAKE_CURRENT_LIST_DIR}/src/tela
        )

# ---------------------------------------------------------------------------
# Bibliotecas
# ---------------------------------------------------------------------------

# pico_stdlib -> funções básicas do Pico SDK
# hardware_adc / hardware_gpio -> joystick e botões
# hardware_i2c -> display OLED
# pico_unique_id -> ID único do chip (nome, série e PID)
# tinyusb_device -> gamepad pelo cabo USB
# pico_cyw43_arch_poll -> chip sem fio do Pico W, atendido pelo laço principal
# pico_btstack_ble / pico_btstack_cyw43 -> gamepad por Bluetooth Low Energy
target_link_libraries(BitDogLabGamepad
        pico_stdlib
        hardware_adc
        hardware_gpio
        hardware_i2c
        pico_unique_id
        tinyusb_device
        pico_cyw43_arch_poll
        pico_btstack_ble
        pico_btstack_cyw43
        )

# O Wi-Fi não é usado, então a pilha de rede (lwIP) fica desativada
target_compile_definitions(BitDogLabGamepad PRIVATE CYW43_LWIP=0)

# Gera gamepad_ble.h (serviços BLE) a partir de gamepad_ble.gatt
pico_btstack_make_gatt_header(BitDogLabGamepad PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/bluetooth/gamepad_ble.gatt")

# Gera os arquivos .uf2, .hex, .bin e .dis além do .elf
pico_add_extra_outputs(BitDogLabGamepad)
