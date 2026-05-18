// Wrapper bem fininho sobre o TensorFlow Lite Micro.
// Esconde toda a parte C++ do TFLM atrás de uma API C limpa.

#ifndef INFERENCE_H_
#define INFERENCE_H_

#include <stdint.h>
#include "model_data.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int    predicted_class;   // 0..NUM_CLASSES-1
    float  confidence;        // 0..1 (softmax do top)
    float  scores[MODEL_NUM_CLASSES];   // todas as probabilidades
} inference_result_t;

// Carrega o modelo, aloca tensor arena. Chamar 1x no setup.
// Retorna 0 se ok, !=0 se erro.
int inference_init(void);

// Roda inferência: features (49*40 floats) → classe + confiança.
int inference_run(const float* features_log_mel, inference_result_t* out);

#ifdef __cplusplus
}
#endif

#endif  // INFERENCE_H_
