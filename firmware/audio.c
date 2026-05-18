#include "audio.h"

#include <string.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"

// Buffer onde o DMA escreve as amostras raw (uint16, 12-bit ADC alinhado em 16)
static uint16_t adc_buffer[AUDIO_CLIP_SAMPLES];

// Canal DMA usado
static int dma_chan = -1;
static dma_channel_config dma_cfg;

void audio_init(void) {
    // 1) Configura ADC
    adc_init();
    adc_gpio_init(MIC_GPIO_PIN);
    adc_select_input(MIC_ADC_CHANNEL);

    adc_fifo_setup(
        true,    // habilita FIFO
        true,    // DREQ enable (pra DMA)
        1,       // DREQ a cada amostra
        false,   // não inclui error bit
        false    // não desloca pra 8-bit
    );

    // Clock do ADC: 48 MHz / divisor = sample rate
    // Pra 16 kHz: divisor = 48 000 000 / 16 000 = 3000
    adc_set_clkdiv(48000000.0f / AUDIO_SAMPLE_RATE - 1.0f);

    // 2) Configura DMA
    dma_chan = dma_claim_unused_channel(true);
    dma_cfg = dma_channel_get_default_config(dma_chan);

    channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&dma_cfg, false);   // sempre lê do FIFO
    channel_config_set_write_increment(&dma_cfg, true);   // escreve incrementalmente
    channel_config_set_dreq(&dma_cfg, DREQ_ADC);
}

void audio_capture_clip(int16_t* out) {
    // Limpa FIFO de qualquer resíduo
    adc_run(false);
    adc_fifo_drain();

    // Configura DMA pra capturar exatamente 1 segundo
    dma_channel_configure(
        dma_chan,
        &dma_cfg,
        adc_buffer,              // destino
        &adc_hw->fifo,           // origem
        AUDIO_CLIP_SAMPLES,      // count
        true                     // start now
    );

    // Liga ADC
    adc_run(true);

    // Bloqueia até DMA terminar
    dma_channel_wait_for_finish_blocking(dma_chan);

    // Para o ADC
    adc_run(false);
    adc_fifo_drain();

    // Converte uint12 → int16 e remove DC offset (média = ~2048 quando silêncio)
    // Aula 6 slide 21: signal = signal - mean(signal)
    uint32_t sum = 0;
    for (uint32_t i = 0; i < AUDIO_CLIP_SAMPLES; i++) {
        sum += adc_buffer[i];
    }
    int32_t mean = (int32_t)(sum / AUDIO_CLIP_SAMPLES);

    for (uint32_t i = 0; i < AUDIO_CLIP_SAMPLES; i++) {
        // (adc - mean) escalado pra faixa int16
        int32_t centered = (int32_t)adc_buffer[i] - mean;
        // ADC é 12-bit → multiplica por 16 pra ocupar 16-bit
        // + ganho digital pra compensar mic analógico fraco
        int32_t scaled = centered * 16 * AUDIO_GAIN;
        // Clipa pra int16
        if (scaled > 32767)  scaled = 32767;
        if (scaled < -32768) scaled = -32768;
        out[i] = (int16_t)scaled;
    }
}

uint16_t audio_get_level(void) {
    // Lê 128 amostras rapidinhas e calcula RMS aproximado
    const int N = 128;
    adc_run(false);
    adc_fifo_drain();

    uint32_t sum_sq = 0;
    uint32_t mean = 0;
    uint16_t samples[N];

    adc_run(true);
    for (int i = 0; i < N; i++) {
        samples[i] = adc_fifo_get_blocking();
        mean += samples[i];
    }
    adc_run(false);
    mean /= N;

    for (int i = 0; i < N; i++) {
        int32_t centered = (int32_t)samples[i] - (int32_t)mean;
        sum_sq += (uint32_t)(centered * centered);
    }

    // sqrt(mean_sq) aproximado por iteração de Newton
    uint32_t mean_sq = sum_sq / N;
    uint32_t r = mean_sq;
    if (r > 0) {
        for (int i = 0; i < 6; i++) r = (r + mean_sq / r) / 2;
    }
    // Escala 12-bit → 16-bit
    uint32_t level = r * 16;
    return level > 32767 ? 32767 : (uint16_t)level;
}
