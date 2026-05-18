// Funções de alto nível pro OLED — só o que o app precisa.

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <stdint.h>

// GPIOs da BitDogLab pro I2C do OLED
#define DISPLAY_I2C_PORT i2c1
#define DISPLAY_SDA_PIN  14
#define DISPLAY_SCL_PIN  15
#define DISPLAY_I2C_HZ   400000

#ifdef __cplusplus
extern "C" {
#endif

void display_init(void);
void display_show_status(const char* status);

// Mostra o resultado da inferência: label centralizado + barra de confiança.
void display_show_prediction(const char* label, float confidence);

#ifdef __cplusplus
}
#endif

#endif  // DISPLAY_H_
