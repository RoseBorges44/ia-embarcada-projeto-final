// Driver low-level pro display OLED SSD1306 128x64 via I2C.
// Mínimo necessário pra escrever texto e limpar tela.

#ifndef SSD1306_H_
#define SSD1306_H_

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"

#define SSD1306_W      128
#define SSD1306_H      64
#define SSD1306_PAGES  (SSD1306_H / 8)
#define SSD1306_I2C_ADDR  0x3C

#ifdef __cplusplus
extern "C" {
#endif

void ssd1306_init(i2c_inst_t* i2c);
void ssd1306_clear(void);
void ssd1306_show(void);
void ssd1306_draw_char(uint8_t x, uint8_t y, char c);
void ssd1306_draw_string(uint8_t x, uint8_t y, const char* s);
void ssd1306_invert(bool on);

#ifdef __cplusplus
}
#endif

#endif  // SSD1306_H_
