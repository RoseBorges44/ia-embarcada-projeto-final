// Bitmaps 5x5 desenhados linha por linha (top-down, left-right).
// Bit 0 = top-left, bit 4 = top-right, bit 24 = bottom-right.
//
// Visualmente:
//     . X . X .       ← bits 1 e 3 acesos
//     X X X X X
//     X X X X X
//     . X X X .
//     . . X . .

#include "pictograms.h"

// Macro pra ler 5 linhas de 5 caracteres e empacotar em 25 bits
#define ROW(b4,b3,b2,b1,b0) ((b0)|(b1<<1)|(b2<<2)|(b3<<3)|(b4<<4))
#define BMP5x5(r0,r1,r2,r3,r4) (r0 | (r1<<5) | (r2<<10) | (r3<<15) | (r4<<20))

// HAPPY — smiley amarelo
//   . X . X .
//   . X . X .
//   . . . . .
//   X . . . X
//   . X X X .
static const uint32_t BMP_HAPPY = BMP5x5(
    ROW(0,1,0,1,0),
    ROW(0,1,0,1,0),
    ROW(0,0,0,0,0),
    ROW(1,0,0,0,1),
    ROW(0,1,1,1,0)
);

// YES — check verde
//   . . . . X
//   . . . X .
//   X . X . .
//   . X . . .
//   . . . . .
static const uint32_t BMP_YES = BMP5x5(
    ROW(0,0,0,0,1),
    ROW(0,0,0,1,0),
    ROW(1,0,1,0,0),
    ROW(0,1,0,0,0),
    ROW(0,0,0,0,0)
);

// NO — X vermelho
//   X . . . X
//   . X . X .
//   . . X . .
//   . X . X .
//   X . . . X
static const uint32_t BMP_NO = BMP5x5(
    ROW(1,0,0,0,1),
    ROW(0,1,0,1,0),
    ROW(0,0,1,0,0),
    ROW(0,1,0,1,0),
    ROW(1,0,0,0,1)
);

// STOP — mão de pare (octógono cheio) vermelho
//   . X X X .
//   X X X X X
//   X X X X X
//   X X X X X
//   . X X X .
static const uint32_t BMP_STOP = BMP5x5(
    ROW(0,1,1,1,0),
    ROW(1,1,1,1,1),
    ROW(1,1,1,1,1),
    ROW(1,1,1,1,1),
    ROW(0,1,1,1,0)
);

// SILENCE — apagado
static const uint32_t BMP_SILENCE = 0;

// UNKNOWN — ponto de interrogação azul
//   . X X X .
//   X . . . X
//   . . . X .
//   . . X . .
//   . . X . .
static const uint32_t BMP_UNKNOWN = BMP5x5(
    ROW(0,1,1,1,0),
    ROW(1,0,0,0,1),
    ROW(0,0,0,1,0),
    ROW(0,0,1,0,0),
    ROW(0,0,1,0,0)
);

const pictogram_t PICTOGRAMS[NUM_CLASSES] = {
    [CLASS_HAPPY]   = {"happy",   BMP_HAPPY,   255, 200,   0},   // amarelo
    [CLASS_YES]     = {"yes",     BMP_YES,       0, 200,   0},   // verde
    [CLASS_NO]      = {"no",      BMP_NO,      200,   0,   0},   // vermelho
    [CLASS_STOP]    = {"stop",    BMP_STOP,    220,  20,  20},   // vermelho forte
    [CLASS_SILENCE] = {"silence", BMP_SILENCE,   0,   0,   0},   // apagado
    [CLASS_UNKNOWN] = {"unknown", BMP_UNKNOWN,   0,  50, 200},   // azul
};
