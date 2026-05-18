# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
# =========================================================================
#  KWS BitDogLab — build com Pico SDK + pico-tflmicro
#
#  Requisitos (ver COMO_TESTAR.md pra instalar tudo no Windows):
#    - Raspberry Pi Pico SDK 2.x
#    - pico-tflmicro clonado em ../pico-tflmicro (ou ajustar PICO_TFLMICRO_PATH)
#    - arm-none-eabi-gcc, CMake, Ninja
# =========================================================================

cmake_minimum_required(VERSION 3.13)

# Pico SDK — a variável PICO_SDK_PATH é setada pela extensão "Raspberry Pi
# Pico" do VS Code, ou manualmente se você usou o pico-setup-windows.
include(pico_sdk_import.cmake)

# Placa: a BitDogLab v6.3 usa o Raspberry Pi Pico W
set(PICO_BOARD pico_w CACHE STRING "Board type")

project(kws_bitdoglab C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# --- pico-tflmicro (porte oficial do TFLite Micro pro RP2040) ---
# Clone com:
#   cd ..
#   git clone --recurse-submodules https://github.com/raspberrypi/pico-tflmicro.git
set(PICO_TFLMICRO_PATH ${CMAKE_CURRENT_LIST_DIR}/../pico-tflmicro CACHE PATH "Path to pico-tflmicro")
if(NOT EXISTS ${PICO_TFLMICRO_PATH}/CMakeLists.txt)
    message(FATAL_ERROR
        "pico-tflmicro não encontrado em ${PICO_TFLMICRO_PATH}.\n"
        "Veja instruções em COMO_TESTAR.md")
endif()
add_subdirectory(${PICO_TFLMICRO_PATH} pico-tflmicro EXCLUDE_FROM_ALL)

# --- Executável principal ---
add_executable(kws_bitdoglab
    main.c
    audio.c
    features.c
    inference.cc
    led_matrix.c
    pictograms.c
    display.c
    ssd1306.c
    model_data.c
)

# Gera header .pio.h a partir do programa PIO
pico_generate_pio_header(kws_bitdoglab ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

# Includes
target_include_directories(kws_bitdoglab PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Libs do Pico SDK
target_link_libraries(kws_bitdoglab PRIVATE
    pico_stdlib
    hardware_adc
    hardware_dma
    hardware_pio
    hardware_i2c
    hardware_clocks
    hardware_gpio
    pico-tflmicro
)

# Saída no USB serial em vez de UART (BitDogLab não tem UART exposto)
pico_enable_stdio_usb(kws_bitdoglab 1)
pico_enable_stdio_uart(kws_bitdoglab 0)

# Gera .uf2 (arquivo pra arrastar pro BOOTSEL drive)
pico_add_extra_outputs(kws_bitdoglab)

message(STATUS "------------------------------------------------------------")
message(STATUS "  Projeto:           ${PROJECT_NAME}")
message(STATUS "  Placa:             ${PICO_BOARD}")
message(STATUS "  pico-tflmicro:     ${PICO_TFLMICRO_PATH}")
message(STATUS "  Build dir:         ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "------------------------------------------------------------")
