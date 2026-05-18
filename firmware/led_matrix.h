// Driver da matriz 5x5 WS2812 da BitDogLab.
//
// Usa o PIO0 SM0 do RP2040 pra gerar o protocolo WS2812 com
// timing exato (1.25us por bit). Pode acionar 25 LEDs sem
// consumir CPU.

#ifndef LED_MATRIX_H_
#define LED_MATRIX_H_

#include <stdint.h>
#include "pictograms.h"

// GPIO da BitDogLab onde a matriz está conectada
#define LED_MATRIX_PIN 7

#ifdef __cplusplus
extern "C" {
#endif

// Inicializa o PIO e o state machine. Chamar 1x no setup.
void led_matrix_init(void);

// Apaga todos os LEDs imediatamente.
void led_matrix_clear(void);

// Pinta a matriz com um pictograma (máscara 5x5 + cor única).
void led_matrix_show_pictogram(const pictogram_t* p);

// Pinta a matriz com um pictograma por classe (lookup em PICTOGRAMS[]).
void led_matrix_show_class(class_id_t cls);

#ifdef __cplusplus
}
#endif

#endif  // LED_MATRIX_H_
