#include "ssd1306.h"
#include "ssd1306_font.h"
#include <string.h>
#include "pico/stdlib.h"

static i2c_inst_t* i2c = NULL;
static uint8_t framebuffer[SSD1306_W * SSD1306_PAGES];

static void cmd(uint8_t c) {
    uint8_t buf[2] = {0x00, c};   // 0x00 = "command stream"
    i2c_write_blocking(i2c, SSD1306_I2C_ADDR, buf, 2, false);
}

static void init_commands(void) {
    cmd(0xAE);                    // display off
    cmd(0x20); cmd(0x00);         // memory addressing mode: horizontal
    cmd(0xB0);                    // page start address
    cmd(0xC8);                    // COM scan dir remap
    cmd(0x00);                    // low column
    cmd(0x10);                    // high column
    cmd(0x40);                    // start line 0
    cmd(0x81); cmd(0xCF);         // contrast
    cmd(0xA1);                    // segment remap
    cmd(0xA6);                    // normal display
    cmd(0xA8); cmd(SSD1306_H-1);  // multiplex ratio
    cmd(0xA4);                    // disable entire display on
    cmd(0xD3); cmd(0x00);         // display offset 0
    cmd(0xD5); cmd(0xF0);         // display clock divide ratio
    cmd(0xD9); cmd(0x22);         // pre-charge period
    cmd(0xDA); cmd(0x12);         // COM pins hw config
    cmd(0xDB); cmd(0x20);         // vcomh deselect
    cmd(0x8D); cmd(0x14);         // charge pump on
    cmd(0xAF);                    // display on
}

void ssd1306_init(i2c_inst_t* inst) {
    i2c = inst;
    sleep_ms(100);
    init_commands();
    ssd1306_clear();
    ssd1306_show();
}

void ssd1306_clear(void) {
    memset(framebuffer, 0, sizeof(framebuffer));
}

void ssd1306_show(void) {
    cmd(0x21); cmd(0); cmd(SSD1306_W - 1);          // col start, end
    cmd(0x22); cmd(0); cmd(SSD1306_PAGES - 1);      // page start, end

    // Manda framebuffer em chunks (i2c_write_blocking tem limite)
    uint8_t buf[SSD1306_W + 1];
    buf[0] = 0x40;   // data stream marker
    for (int page = 0; page < SSD1306_PAGES; page++) {
        memcpy(&buf[1], &framebuffer[page * SSD1306_W], SSD1306_W);
        i2c_write_blocking(i2c, SSD1306_I2C_ADDR, buf, SSD1306_W + 1, false);
    }
}

void ssd1306_draw_char(uint8_t x, uint8_t y, char c) {
    if (c < ' ' || c > 'z') c = '?';
    const uint8_t* glyph = FONT_8x8[c - ' '];

    uint8_t page = y / 8;
    if (page >= SSD1306_PAGES) return;

    for (int col = 0; col < 8; col++) {
        if (x + col >= SSD1306_W) break;
        framebuffer[page * SSD1306_W + x + col] = glyph[col];
    }
}

void ssd1306_draw_string(uint8_t x, uint8_t y, const char* s) {
    while (*s) {
        ssd1306_draw_char(x, y, *s);
        x += 8;
        s++;
        if (x >= SSD1306_W) break;
    }
}

void ssd1306_invert(bool on) {
    cmd(on ? 0xA7 : 0xA6);
}
