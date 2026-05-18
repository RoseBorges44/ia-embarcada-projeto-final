// Pictogramas 5x5 — um por classe do modelo.
// Cada pictograma é uma máscara binária 25 bits (1 = aceso, 0 = apagado)
// + uma cor RGB pra todos os pixels acesos.

#ifndef PICTOGRAMS_H_
#define PICTOGRAMS_H_

#include <stdint.h>

#define MATRIX_W 5
#define MATRIX_H 5
#define MATRIX_LEDS (MATRIX_W * MATRIX_H)

typedef struct {
    const char* label;       // "happy", "yes", etc
    uint32_t    mask;        // bit i = LED i aceso (0=top-left, lendo em zigzag)
    uint8_t     r, g, b;     // cor de cada LED aceso (escala 0-255)
} pictogram_t;

// Índices das classes (têm que casar com a ordem do treino)
typedef enum {
    CLASS_HAPPY   = 0,
    CLASS_YES     = 1,
    CLASS_NO      = 2,
    CLASS_STOP    = 3,
    CLASS_SILENCE = 4,
    CLASS_UNKNOWN = 5,
    NUM_CLASSES   = 6,
} class_id_t;

extern const pictogram_t PICTOGRAMS[NUM_CLASSES];

#endif  // PICTOGRAMS_H_
