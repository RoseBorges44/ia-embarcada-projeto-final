// ============================================================================
//  KWS BitDogLab — Projeto Final IA Embarcada
//
//  Loop principal:
//    1. Espera o nível de áudio passar do threshold (alguém falou)
//    2. Captura 1 segundo de áudio (ADC + DMA a 16 kHz)
//    3. Extrai mel spectrogram (49 × 40)
//    4. Roda inferência TFLite Micro
//    5. Mostra resultado na matriz 5x5 + OLED
//    6. LED RGB indica estado: amarelo=processando, verde=pronto, vermelho=erro
//
//  Pinos da BitDogLab usados:
//    GPIO  7: matriz 5x5 WS2812
//    GPIO 11/12/13: LED RGB de 5mm (verde, azul, vermelho)
//    GPIO 14/15: I2C1 do OLED (SDA, SCL)
//    GPIO 28: ADC do microfone
// ============================================================================

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"

#include "audio.h"
#include "features.h"
#include "inference.h"
#include "led_matrix.h"
#include "display.h"
#include "pictograms.h"

// LED RGB de 5mm (catodo comum) — indica estado do firmware
#define LED_R_PIN 13
#define LED_G_PIN 11
#define LED_B_PIN 12

// Threshold pra começar a gravar (ADC RMS). Ajuste se o ambiente
// for muito silencioso ou muito ruidoso.
#define AUDIO_TRIGGER_LEVEL  800

// Confiança mínima entre as 4 palavras-alvo (happy/yes/no/stop) pra mostrar.
// O dataset Speech Commands tem MUITO mais exemplos de "unknown" que das
// outras classes, então o modelo enviesa pra unknown — ignoramos essa
// classe e pegamos a melhor das palavras-alvo se passar deste threshold.
#define CONFIDENCE_THRESHOLD 0.30f

// Buffers principais (estáticos — evita fragmentação na heap)
static int16_t audio_buffer[AUDIO_CLIP_SAMPLES];
static float   feature_buffer[FEAT_OUTPUT_SIZE];

static void led_rgb_init(void) {
    gpio_init(LED_R_PIN); gpio_set_dir(LED_R_PIN, GPIO_OUT);
    gpio_init(LED_G_PIN); gpio_set_dir(LED_G_PIN, GPIO_OUT);
    gpio_init(LED_B_PIN); gpio_set_dir(LED_B_PIN, GPIO_OUT);
}

static void led_rgb_set(bool r, bool g, bool b) {
    gpio_put(LED_R_PIN, r);
    gpio_put(LED_G_PIN, g);
    gpio_put(LED_B_PIN, b);
}

int main(void) {
    stdio_init_all();
    sleep_ms(2000);  // dá tempo pro serial USB conectar

    printf("\n=== KWS BitDogLab — IA Embarcada ===\n");

    // --- Setup dos periféricos ---
    led_rgb_init();
    led_rgb_set(false, false, true);   // azul = inicializando

    led_matrix_init();
    led_matrix_clear();

    display_init();
    display_show_status("Inicializando");

    audio_init();
    features_init();

    int err = inference_init();
    if (err != 0) {
        printf("[ERRO] inference_init falhou: %d\n", err);
        printf("       Você esqueceu de substituir model_data.c pelo modelo treinado?\n");
        display_show_status("ERRO modelo");
        led_rgb_set(true, false, false);   // vermelho = erro fatal
        while (true) tight_loop_contents();
    }

    printf("Setup ok. Aguardando voz (threshold=%d)...\n", AUDIO_TRIGGER_LEVEL);
    display_show_status("Fale!");
    led_rgb_set(false, true, false);    // verde = pronto

    // --- Loop principal ---
    while (true) {
        // 1) Polling do nível pra detectar voz
        uint16_t level = audio_get_level();

        if (level < AUDIO_TRIGGER_LEVEL) {
            sleep_ms(20);
            continue;
        }

        printf("Trigger! level=%d, capturando...\n", level);
        led_rgb_set(true, true, false);   // amarelo = processando
        display_show_status("Gravando...");

        // 2) Captura 1 segundo
        uint32_t t0 = to_ms_since_boot(get_absolute_time());
        audio_capture_clip(audio_buffer);
        uint32_t t1 = to_ms_since_boot(get_absolute_time());

        // 3) Features
        display_show_status("Processando");
        features_extract(audio_buffer, feature_buffer);
        uint32_t t2 = to_ms_since_boot(get_absolute_time());

        // Diagnóstico: estatística das features (DEVE ser tipicamente -13..+5)
        float feat_min = feature_buffer[0], feat_max = feature_buffer[0];
        float feat_sum = 0.0f;
        for (int i = 0; i < FEAT_OUTPUT_SIZE; i++) {
            float v = feature_buffer[i];
            if (v < feat_min) feat_min = v;
            if (v > feat_max) feat_max = v;
            feat_sum += v;
        }
        printf("Features stats: min=%.2f max=%.2f mean=%.2f\n",
               feat_min, feat_max, feat_sum / FEAT_OUTPUT_SIZE);

        // 4) Inferência
        inference_result_t result;
        if (inference_run(feature_buffer, &result) != 0) {
            printf("[ERRO] inference_run falhou\n");
            led_rgb_set(true, false, false);
            sleep_ms(500);
            led_rgb_set(false, true, false);
            continue;
        }
        uint32_t t3 = to_ms_since_boot(get_absolute_time());

        // 5) Log no serial
        printf("Audio: %lums | Features: %lums | Inferência: %lums\n",
               t1 - t0, t2 - t1, t3 - t2);
        printf("Predição raw: %s (%.0f%%)\n",
               PICTOGRAMS[result.predicted_class].label,
               result.confidence * 100.0f);
        for (int c = 0; c < MODEL_NUM_CLASSES; c++) {
            printf("   %-10s %.2f\n", PICTOGRAMS[c].label, result.scores[c]);
        }

        // 6) Decisão: ignora silence (4) e unknown (5), pega a melhor das
        //    4 palavras-alvo (happy/yes/no/stop). Isso anula o viés do dataset.
        int   best_target_idx   = 0;
        float best_target_score = -1.0f;
        for (int c = 0; c < 4; c++) {
            if (result.scores[c] > best_target_score) {
                best_target_score = result.scores[c];
                best_target_idx   = c;
            }
        }
        printf("Melhor palavra-alvo: %s (%.0f%%)\n",
               PICTOGRAMS[best_target_idx].label,
               best_target_score * 100.0f);

        // 7) Saída visual
        if (best_target_score >= CONFIDENCE_THRESHOLD) {
            led_matrix_show_class((class_id_t)best_target_idx);
            display_show_prediction(PICTOGRAMS[best_target_idx].label,
                                    best_target_score);
        } else {
            led_matrix_show_class(CLASS_UNKNOWN);
            display_show_prediction("?", best_target_score);
        }

        // Mantém o resultado na tela por 2 segundos
        sleep_ms(2000);

        // Volta ao estado "pronto"
        led_matrix_clear();
        display_show_status("Fale!");
        led_rgb_set(false, true, false);
    }

    return 0;
}
