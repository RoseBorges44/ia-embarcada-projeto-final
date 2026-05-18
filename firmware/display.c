#include "display.h"
#include "ssd1306.h"

#include <stdio.h>
#include <string.h>
#include "hardware/i2c.h"
#include "pico/stdlib.h"

void display_init(void) {
    i2c_init(DISPLAY_I2C_PORT, DISPLAY_I2C_HZ);
    gpio_set_function(DISPLAY_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(DISPLAY_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(DISPLAY_SDA_PIN);
    gpio_pull_up(DISPLAY_SCL_PIN);

    ssd1306_init(DISPLAY_I2C_PORT);
}

void display_show_status(const char* status) {
    ssd1306_clear();
    ssd1306_draw_string(0, 0,  "KWS BitDogLab");
    ssd1306_draw_string(0, 16, "Status:");
    ssd1306_draw_string(0, 32, status);
    ssd1306_show();
}

void display_show_prediction(const char* label, float confidence) {
    ssd1306_clear();
    ssd1306_draw_string(0, 0, "Reconheceu:");

    // Label em destaque (poderia usar fonte maior, mas a 8x8 dá)
    uint8_t label_x = (SSD1306_W - (uint8_t)strlen(label) * 8) / 2;
    ssd1306_draw_string(label_x, 24, label);

    // % de confiança
    char buf[16];
    snprintf(buf, sizeof(buf), "%.0f%%", confidence * 100.0f);
    uint8_t conf_x = (SSD1306_W - (uint8_t)strlen(buf) * 8) / 2;
    ssd1306_draw_string(conf_x, 48, buf);

    ssd1306_show();
}
