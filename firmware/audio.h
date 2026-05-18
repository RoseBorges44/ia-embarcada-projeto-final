// Captura de áudio do mic analógico da BitDogLab.
//
// Usa o ADC do RP2040 com DMA pra amostrar 16 000 amostras/segundo
// (16 kHz) sem ocupar a CPU. Doublé-buffering: enquanto um buffer
// está sendo preenchido, o outro está disponível pra processar.

#ifndef AUDIO_H_
#define AUDIO_H_

#include <stdint.h>
#include <stdbool.h>

// GPIO/ADC channel do microfone na BitDogLab
//   GPIO28 = ADC2 (mic eletreto da BitDogLab v6.3)
#define MIC_GPIO_PIN     28
#define MIC_ADC_CHANNEL  2

// Taxa de amostragem (1 amostra a cada 62.5us)
#define AUDIO_SAMPLE_RATE 16000

// Duração de 1 clipe = 1 segundo = 16 000 amostras
#define AUDIO_CLIP_SAMPLES AUDIO_SAMPLE_RATE

// Ganho digital aplicado ao áudio capturado.
// O mic analógico da BitDogLab é fraco — sem ganho, o RMS fica em 3-5% da
// faixa max do int16, muito abaixo do que o Speech Commands gravou.
// 8x leva pra ~30-40%, próximo da escala esperada pelo modelo.
// Picos saturam (clip), mas isso é OK pra KWS.
#define AUDIO_GAIN 8

#ifdef __cplusplus
extern "C" {
#endif

// Inicializa ADC + DMA. Chamar 1x no setup.
void audio_init(void);

// Captura 1 segundo (16 000 amostras) de áudio em modo bloqueante.
// O buffer 'out' deve ter no mínimo AUDIO_CLIP_SAMPLES int16_t (32 KB).
// As amostras já vêm com DC offset removido.
void audio_capture_clip(int16_t* out);

// Lê o nível atual de áudio (RMS dos últimos N samples), 0..32767.
// Útil pra threshold de "começou a falar" e pro LED RGB.
uint16_t audio_get_level(void);

#ifdef __cplusplus
}
#endif

#endif  // AUDIO_H_
