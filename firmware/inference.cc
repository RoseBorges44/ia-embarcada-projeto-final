// Wrapper TFLite Micro pro modelo de KWS.

#include "inference.h"
#include "model_data.h"
#include "features.h"

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include <math.h>
#include <string.h>

// Arena de memória pro TFLM (todos os tensores intermediários cabem aqui).
// O DS-CNN treinado precisou de ~64 KB (medido pelo erro "Requested: 64000").
// 80 KB dá folga de 25%.
//
// Cuidado: RP2040 tem só 264 KB de RAM. Tem audio_buffer (32KB),
// feature_buffer (8KB), arena (80KB), stack+heap+globais (~40KB) = ~160KB.
constexpr int kTensorArenaSize = 80 * 1024;
alignas(16) static uint8_t tensor_arena[kTensorArenaSize];

// Ops necessários pra DS-CNN do nosso modelo. MicroMutableOpResolver é
// preferível ao AllOpsResolver porque só inclui o que é usado de fato —
// economiza ~20 KB de flash.
// Se trocar a arquitetura no notebook e o init falhar com "Didn't find op",
// é só adicionar o op faltante aqui.
static tflite::MicroMutableOpResolver<10> op_resolver;
static const tflite::Model* model = nullptr;
static tflite::MicroInterpreter* interpreter = nullptr;
static TfLiteTensor* input_tensor = nullptr;
static TfLiteTensor* output_tensor = nullptr;

extern "C" int inference_init(void) {
    // Verifica versão do schema
    model = tflite::GetModel(g_model);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        return -1;
    }

    // Registra os ops que a DS-CNN usa
    if (op_resolver.AddConv2D()              != kTfLiteOk) return -2;
    if (op_resolver.AddDepthwiseConv2D()     != kTfLiteOk) return -2;
    if (op_resolver.AddMean()                != kTfLiteOk) return -2;   // pro GlobalAveragePooling
    if (op_resolver.AddFullyConnected()      != kTfLiteOk) return -2;
    if (op_resolver.AddSoftmax()             != kTfLiteOk) return -2;
    if (op_resolver.AddReshape()             != kTfLiteOk) return -2;
    if (op_resolver.AddQuantize()            != kTfLiteOk) return -2;
    if (op_resolver.AddDequantize()          != kTfLiteOk) return -2;

    // Constrói o interpretador
    static tflite::MicroInterpreter static_interpreter(
        model, op_resolver, tensor_arena, kTensorArenaSize);
    interpreter = &static_interpreter;

    // Aloca tensores
    if (interpreter->AllocateTensors() != kTfLiteOk) {
        return -3;
    }

    input_tensor  = interpreter->input(0);
    output_tensor = interpreter->output(0);

    // Sanity: confere dimensões esperadas
    if (input_tensor->dims->size != 4 ||
        input_tensor->dims->data[1] != MODEL_N_FRAMES ||
        input_tensor->dims->data[2] != MODEL_N_MEL_BINS ||
        input_tensor->dims->data[3] != 1) {
        return -4;
    }
    if (output_tensor->dims->data[1] != MODEL_NUM_CLASSES) {
        return -5;
    }

    return 0;
}

extern "C" int inference_run(const float* features_log_mel, inference_result_t* out) {
    if (!interpreter || !input_tensor || !output_tensor || !out) {
        return -1;
    }

    // Quantiza float → int8 usando as escalas do treino
    const int n = MODEL_N_FRAMES * MODEL_N_MEL_BINS;
    int8_t* input_data = input_tensor->data.int8;
    const float scale = input_tensor->params.scale;
    const int   zero  = input_tensor->params.zero_point;

    for (int i = 0; i < n; i++) {
        int32_t q = (int32_t)roundf(features_log_mel[i] / scale) + zero;
        if (q < -128) q = -128;
        if (q >  127) q =  127;
        input_data[i] = (int8_t)q;
    }

    // Roda
    if (interpreter->Invoke() != kTfLiteOk) {
        return -2;
    }

    // Dequantiza saída int8 → float, escolhe top class
    const int8_t* output_data = output_tensor->data.int8;
    const float  out_scale = output_tensor->params.scale;
    const int    out_zero  = output_tensor->params.zero_point;

    int   best_idx   = 0;
    float best_score = -1.0f;
    for (int c = 0; c < MODEL_NUM_CLASSES; c++) {
        float score = (output_data[c] - out_zero) * out_scale;
        out->scores[c] = score;
        if (score > best_score) {
            best_score = score;
            best_idx   = c;
        }
    }

    out->predicted_class = best_idx;
    out->confidence      = best_score;
    return 0;
}
