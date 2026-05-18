// Extração de features de áudio: bruto int16 → mel spectrogram log.
//
// IMPORTANTE: estes parâmetros TÊM que casar EXATAMENTE com os
// usados no notebook treino_kws.ipynb. Se alterar aqui, alterar lá
// também — senão o modelo recebe entrada diferente do que foi
// treinado e a acurácia despenca.

#ifndef FEATURES_H_
#define FEATURES_H_

#include <stdint.h>

#define FEAT_SAMPLE_RATE         16000
#define FEAT_CLIP_SAMPLES        16000      // 1 segundo

#define FEAT_WINDOW_SIZE_MS      30
#define FEAT_WINDOW_STRIDE_MS    20
#define FEAT_WINDOW_SAMPLES      ((FEAT_SAMPLE_RATE * FEAT_WINDOW_SIZE_MS) / 1000)    // 480
#define FEAT_WINDOW_STRIDE       ((FEAT_SAMPLE_RATE * FEAT_WINDOW_STRIDE_MS) / 1000)  // 320

#define FEAT_FFT_SIZE            512
#define FEAT_N_MEL_BINS          40
#define FEAT_N_FRAMES            49

// Tamanho total do tensor de saída: 49 * 40 * 1 = 1960 floats
#define FEAT_OUTPUT_SIZE         (FEAT_N_FRAMES * FEAT_N_MEL_BINS)

#ifdef __cplusplus
extern "C" {
#endif

// Inicializa tabelas (Hann window, filtros Mel). Chamar 1x no setup.
void features_init(void);

// Computa mel spectrogram log a partir de áudio bruto.
//
//   audio_in:  16 000 amostras int16 (1 segundo a 16 kHz)
//   out_log_mel: 49 * 40 floats, layout (frame, mel_bin)
void features_extract(const int16_t* audio_in, float* out_log_mel);

#ifdef __cplusplus
}
#endif

#endif  // FEATURES_H_
