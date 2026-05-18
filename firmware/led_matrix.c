#include "led_matrix.h"
#include "ws2812.pio.h"

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

static PIO  pio_inst = pio0;
static uint pio_sm   = 0;

// A matriz da BitDogLab é cabeada em zigzag (snake):
//   linha 0 (topo):     LEDs 20, 21, 22, 23, 24 (esquerda → direita)
//   linha 1:            LEDs 19, 18, 17, 16, 15 (direita → esquerda)
//   linha 2:            LEDs 10, 11, 12, 13, 14
//   linha 3:            LEDs  9,  8,  7,  6,  5
//   linha 4 (base):     LEDs  0,  1,  2,  3,  4
//
// Esta tabela traduz índice "lógico" (top-left = 0) → índice físico no hardware.
static const uint8_t LED_LOGICAL_TO_PHYSICAL[MATRIX_LEDS] = {
    20, 21, 22, 23, 24,   // linha 0
    19, 18, 17, 16, 15,   // linha 1
    10, 11, 12, 13, 14,   // linha 2
     9,  8,  7,  6,  5,   // linha 3
     0,  1,  2,  3,  4,   // linha 4
};

// Empacota RGB em uint32_t no formato GRB (que o WS2812 espera)
static inline uint32_t rgb_to_grb(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)g << 24) | ((uint32_t)r << 16) | ((uint32_t)b << 8);
}

static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio_inst, pio_sm, pixel_grb);
}

void led_matrix_init(void) {
    uint offset = pio_add_program(pio_inst, &ws2812_program);
    ws2812_program_init(pio_inst, pio_sm, offset, LED_MATRIX_PIN, 800000.0f, false);
    led_matrix_clear();
}

void led_matrix_clear(void) {
    for (int i = 0; i < MATRIX_LEDS; i++) {
        put_pixel(0);
    }
    sleep_us(100);   // sinal de "reset" (>50us low)
}

void led_matrix_show_pictogram(const pictogram_t* p) {
    if (!p) {
        led_matrix_clear();
        return;
    }

    // Pré-monta buffer de 25 pixels em ordem física
    uint32_t pixels[MATRIX_LEDS] = {0};
    uint32_t on_color = rgb_to_grb(p->r, p->g, p->b);

    for (int logical = 0; logical < MATRIX_LEDS; logical++) {
        bool on = (p->mask >> logical) & 1u;
        uint8_t physical = LED_LOGICAL_TO_PHYSICAL[logical];
        pixels[physical] = on ? on_color : 0;
    }

    // Manda em ordem física (índice 0 primeiro)
    for (int i = 0; i < MATRIX_LEDS; i++) {
        put_pixel(pixels[i]);
    }
    sleep_us(100);
}

void led_matrix_show_class(class_id_t cls) {
    if (cls < 0 || cls >= NUM_CLASSES) {
        led_matrix_clear();
        return;
    }
    led_matrix_show_pictogram(&PICTOGRAMS[cls]);
}
