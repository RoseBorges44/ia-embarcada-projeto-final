// Header do modelo TFLite quantizado
// Gerado automaticamente pelo treino_kws.ipynb

#ifndef MODEL_DATA_H_
#define MODEL_DATA_H_

#ifdef __cplusplus
extern "C" {
#endif

// alignas fica só na DEFINIÇÃO em model_data.c (C++17 estrito gcc 14.2)
extern const unsigned char g_model[];
extern const unsigned int g_model_len;

// Parâmetros do treino — mantenha sincronizado com firmware/features.h
#define MODEL_SAMPLE_RATE       16000
#define MODEL_CLIP_SAMPLES      16000
#define MODEL_N_MEL_BINS        40
#define MODEL_N_FRAMES          49
#define MODEL_NUM_CLASSES       6

// Escalas da quantização int8 (do TFLite converter)
#define MODEL_INPUT_SCALE       0.07223919034004211f
#define MODEL_INPUT_ZERO_POINT  63
#define MODEL_OUTPUT_SCALE      0.00390625f
#define MODEL_OUTPUT_ZERO_POINT -128

#ifdef __cplusplus
}
#endif

#endif  // MODEL_DATA_H_
