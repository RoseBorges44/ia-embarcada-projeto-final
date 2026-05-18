// Implementação minimalista de mel-spectrogram para keyword spotting.
//
// Pra evitar dependências externas pesadas (kissFFT, CMSIS-DSP), faz a FFT
// na mão com um Cooley-Tukey radix-2 in-place sobre buffer estático.
// Não é o mais rápido, mas roda em ~80ms no RP2040 a 125MHz pra FFT 512 —
// totalmente aceitável dentro do orçamento de < 200ms por inferência.

#include "features.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// --- Buffers estáticos pra não estourar a stack ---
static float hann_window[FEAT_WINDOW_SAMPLES];
static float fft_real[FEAT_FFT_SIZE];
static float fft_imag[FEAT_FFT_SIZE];

// Filterbank Mel: cada bin é uma combinação triangular dos bins de FFT
// Pra economizar memória só guardamos os pontos start/peak/end de cada filtro
typedef struct {
    int   start;
    int   peak;
    int   end;
} mel_filter_t;
static mel_filter_t mel_filters[FEAT_N_MEL_BINS];

// --- Tabelas pré-computadas pra FFT (twiddle factors) ---
static float cos_table[FEAT_FFT_SIZE / 2];
static float sin_table[FEAT_FFT_SIZE / 2];

// Conversão Hz <-> Mel
static inline float hz_to_mel(float hz) {
    return 2595.0f * log10f(1.0f + hz / 700.0f);
}
static inline float mel_to_hz(float mel) {
    return 700.0f * (powf(10.0f, mel / 2595.0f) - 1.0f);
}

void features_init(void) {
    // 1) Janela de Hann
    for (int i = 0; i < FEAT_WINDOW_SAMPLES; i++) {
        hann_window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (FEAT_WINDOW_SAMPLES - 1)));
    }

    // 2) Twiddle factors pra FFT
    for (int i = 0; i < FEAT_FFT_SIZE / 2; i++) {
        float angle = -2.0f * M_PI * i / FEAT_FFT_SIZE;
        cos_table[i] = cosf(angle);
        sin_table[i] = sinf(angle);
    }

    // 3) Filtros Mel — distribuição triangular entre 20 Hz e 4000 Hz
    float low_mel  = hz_to_mel(20.0f);
    float high_mel = hz_to_mel(4000.0f);

    // 42 pontos pra 40 filtros (cada filtro precisa de 3 pontos: start/peak/end)
    float mel_points[FEAT_N_MEL_BINS + 2];
    for (int i = 0; i < FEAT_N_MEL_BINS + 2; i++) {
        float mel = low_mel + (high_mel - low_mel) * i / (FEAT_N_MEL_BINS + 1);
        float hz = mel_to_hz(mel);
        mel_points[i] = hz;
    }

    // Converte Hz → bin de FFT (cada bin cobre sample_rate/fft_size Hz)
    float hz_per_bin = (float)FEAT_SAMPLE_RATE / FEAT_FFT_SIZE;
    for (int i = 0; i < FEAT_N_MEL_BINS; i++) {
        mel_filters[i].start = (int)(mel_points[i]     / hz_per_bin);
        mel_filters[i].peak  = (int)(mel_points[i + 1] / hz_per_bin);
        mel_filters[i].end   = (int)(mel_points[i + 2] / hz_per_bin);
    }
}

// Reordenação bit-reversa (in-place) — pré-passo da FFT radix-2
static void bit_reverse_reorder(float* real, float* imag, int n) {
    int j = 0;
    for (int i = 1; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j |= bit;
        if (i < j) {
            float tr = real[i]; real[i] = real[j]; real[j] = tr;
            float ti = imag[i]; imag[i] = imag[j]; imag[j] = ti;
        }
    }
}

// FFT Cooley-Tukey radix-2 in-place, n=FEAT_FFT_SIZE
static void fft_radix2(float* real, float* imag) {
    const int n = FEAT_FFT_SIZE;
    bit_reverse_reorder(real, imag, n);

    for (int size = 2; size <= n; size <<= 1) {
        int half = size >> 1;
        int step = n / size;
        for (int i = 0; i < n; i += size) {
            for (int k = 0; k < half; k++) {
                int idx = k * step;
                float c = cos_table[idx];
                float s = sin_table[idx];
                int a = i + k;
                int b = a + half;
                float tr = real[b] * c - imag[b] * s;
                float ti = real[b] * s + imag[b] * c;
                real[b] = real[a] - tr;
                imag[b] = imag[a] - ti;
                real[a] += tr;
                imag[a] += ti;
            }
        }
    }
}

void features_extract(const int16_t* audio_in, float* out_log_mel) {
    for (int frame = 0; frame < FEAT_N_FRAMES; frame++) {
        int offset = frame * FEAT_WINDOW_STRIDE;

        // 1) Copia janela e aplica Hann
        memset(fft_real, 0, sizeof(fft_real));
        memset(fft_imag, 0, sizeof(fft_imag));
        for (int i = 0; i < FEAT_WINDOW_SAMPLES; i++) {
            if (offset + i >= FEAT_CLIP_SAMPLES) break;
            float sample = (float)audio_in[offset + i] / 32768.0f;
            fft_real[i] = sample * hann_window[i];
        }

        // 2) FFT in-place
        fft_radix2(fft_real, fft_imag);

        // 3) Magnitude do espectro (só metade positiva, FEAT_FFT_SIZE/2 + 1)
        // Aproveita fft_real pra guardar a magnitude (não precisa mais dos
        // valores complexos depois desse ponto)
        for (int i = 0; i <= FEAT_FFT_SIZE / 2; i++) {
            fft_real[i] = sqrtf(fft_real[i] * fft_real[i] + fft_imag[i] * fft_imag[i]);
        }

        // 4) Aplica filterbank Mel triangular
        for (int m = 0; m < FEAT_N_MEL_BINS; m++) {
            mel_filter_t f = mel_filters[m];
            float sum = 0.0f;

            // Lado ascendente: start → peak
            for (int k = f.start; k < f.peak; k++) {
                if (k <= FEAT_FFT_SIZE / 2 && f.peak > f.start) {
                    float weight = (float)(k - f.start) / (f.peak - f.start);
                    sum += fft_real[k] * weight;
                }
            }
            // Lado descendente: peak → end
            for (int k = f.peak; k <= f.end; k++) {
                if (k <= FEAT_FFT_SIZE / 2 && f.end > f.peak) {
                    float weight = (float)(f.end - k) / (f.end - f.peak);
                    sum += fft_real[k] * weight;
                }
            }

            // 5) Log (estabilizado, igual ao notebook)
            out_log_mel[frame * FEAT_N_MEL_BINS + m] = logf(sum + 1e-6f);
        }
    }
}
