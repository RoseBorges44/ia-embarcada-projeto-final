# Arquitetura do Sistema

Este documento serve como base pra montar os slides da apresentação
de 10 minutos. Cobre as 4 etapas exigidas pelo prof (`Descrição do
projeto final.pdf`):

1. Coleta de dados de sensores
2. Treinamento de um modelo
3. Conversão e compressão do modelo
4. Pipeline de inferência no dispositivo

---

## Visão de blocos

```
┌─────────────────┐                          ┌──────────────────┐
│ Microfone       │   sinal analógico        │ Matriz 5x5 RGB   │
│ analógico       │ ─────────┐    ┌────────► │ (WS2812)         │
│ BitDogLab       │          │    │          └──────────────────┘
└─────────────────┘          │    │
                             ▼    │          ┌──────────────────┐
                          ┌────────────┐     │ Display OLED     │
                          │ RP2040     │ ──► │ SSD1306 (I2C)    │
                          │ Cortex-M0+ │     └──────────────────┘
                          │ 125 MHz    │
                          └────────────┘     ┌──────────────────┐
                                       └───► │ LED RGB 5mm      │
                                             │ (status visual)  │
                                             └──────────────────┘
```

---

## Pipeline interno (dentro do RP2040)

```
┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌──────────────┐
│ 1. Captura   │►│ 2. Features  │►│ 3. Inferência│►│ 4. Saída     │
│              │ │              │ │              │ │              │
│ ADC + DMA    │ │ FFT 512      │ │ DS-CNN int8  │ │ Matriz 5x5   │
│ 16 kHz       │ │ Mel 40 bins  │ │ TFLite Micro │ │ OLED 128x64  │
│ 1 segundo    │ │ log spec     │ │ 6 classes    │ │ LED RGB      │
│ ~16 KB int16 │ │ ~8 KB float  │ │ ~50 KB flash │ │              │
│ ~1 s         │ │ ~90 ms       │ │ ~120 ms      │ │ < 5 ms       │
└──────────────┘ └──────────────┘ └──────────────┘ └──────────────┘
```

**Latência total:** ~1.2 segundos (1s captura + 200ms processamento)

---

## 1. Coleta de dados — Microfone analógico via ADC

| Item              | Valor                                          |
| ----------------- | ---------------------------------------------- |
| Sensor            | Eletreto omnidirecional GY-MAX4466 (BitDogLab) |
| Interface         | Analógica (ADC do RP2040, GPIO 28)             |
| Taxa de amostragem| 16 000 amostras/segundo                        |
| Resolução         | 12-bit (4096 níveis) escalado pra int16        |
| Janela            | 1 segundo (16 000 amostras)                    |
| DMA               | Sim — captura sem ocupar CPU                   |

**Pré-processamento aplicado:**
- Remoção de DC offset (Aula 6 slide 21)
- Escalonamento 12-bit → 16-bit

---

## 2. Treinamento — DS-CNN sobre Speech Commands v2

| Item              | Valor                                          |
| ----------------- | ---------------------------------------------- |
| Dataset           | Google Speech Commands v2 (sugerido pelo prof) |
| Tamanho           | ~105 000 áudios WAV de 1s a 16 kHz             |
| Palavras-alvo     | happy, yes, no, stop                           |
| Classes extras    | silence, unknown                               |
| Total classes     | 6                                              |
| Split             | train / validation / test (oficial do dataset) |

**Features de áudio:**
- Mel spectrogram log
- Janela: 30 ms com stride de 20 ms → 49 frames por clipe
- 40 bins Mel entre 20 Hz e 4000 Hz
- Saída: tensor (49, 40, 1)

**Modelo:** DS-CNN (Depthwise Separable CNN — Aula 6 slide 40)
- Conv2D inicial (64 filtros, kernel 10×4, stride 2×2)
- 4× blocos {DepthwiseConv 3×3 → BN → ReLU → Conv 1×1 → BN → ReLU}
- GlobalAveragePooling
- Dense 6 + softmax
- **~30 000 parâmetros**

**Hiperparâmetros:**
- Optimizer: Adam, learning rate 1e-3
- Loss: sparse categorical crossentropy
- Batch size: 128
- Epochs: 25 com early stopping + ReduceLROnPlateau

---

## 3. Conversão e compressão — Quantização int8

| Métrica           | Float32     | Int8        | Redução |
| ----------------- | ----------- | ----------- | ------- |
| Tamanho do modelo | ~120 KB     | ~50 KB      | 2.4×    |
| RAM (arena)       | ~80 KB      | ~30 KB      | 2.7×    |
| Acurácia (test)   | ~94%        | ~93%        | -1pp    |
| Latência          | --          | ~120 ms     | --      |

**Pipeline da quantização:**
1. `TFLiteConverter.from_keras_model(model)`
2. `optimizations = [Optimize.DEFAULT]`
3. `representative_dataset` com 100 amostras pra calibrar escalas
4. `inference_input_type = inference_output_type = int8`

**Por que int8?** O RP2040 é Cortex-M0+ sem unidade FPU — operações float
são emuladas (lentas). Int8 roda nativo, é 3-5× mais rápido e ocupa
1/4 da memória.

---

## 4. Pipeline de inferência embarcada

### Stack de software

| Camada       | Tecnologia                                          |
| ------------ | --------------------------------------------------- |
| Aplicação    | Loop principal em `main.c`                          |
| Runtime ML   | TensorFlow Lite Micro                               |
| Porte RP2040 | pico-tflmicro (porte oficial da Raspberry Pi)       |
| HAL          | Raspberry Pi Pico SDK 2.x                           |
| Toolchain    | arm-none-eabi-gcc + CMake + Ninja                   |

### Estados do firmware

```
   ┌─────────────┐
   │  BOOT       │  azul
   └──────┬──────┘
          ▼
   ┌─────────────┐
   │  PRONTO     │  verde   ◄──────────────┐
   │ (esperando) │                          │
   └──────┬──────┘                          │
          │ áudio > threshold               │
          ▼                                 │
   ┌─────────────┐                          │
   │ CAPTURANDO  │  amarelo                 │
   │ (1 segundo) │                          │
   └──────┬──────┘                          │
          ▼                                 │
   ┌─────────────┐                          │
   │ INFERINDO   │  amarelo                 │
   │ (~200 ms)   │                          │
   └──────┬──────┘                          │
          ▼                                 │
   ┌─────────────┐                          │
   │ MOSTRANDO   │  cor do pictograma       │
   │ (2 seg)     │ ─────────────────────────┘
   └─────────────┘
```

### Memória

| Recurso       | Disponível RP2040 | Usado pelo projeto |
| ------------- | ------------------ | ------------------ |
| Flash         | 2 MB               | ~200 KB (10%)      |
| RAM           | 264 KB             | ~110 KB (40%)      |
|   – audio_buffer  |               | 32 KB              |
|   – feature_buffer|               | 8 KB               |
|   – tensor arena  |               | 30 KB              |
|   – stack/heap    |               | ~40 KB             |

---

## Decisões de projeto importantes

### Por que mel spectrogram e não MFCC completo?
MFCC adiciona uma DCT depois do log-mel. Pra KWS embarcado o ganho é
marginal (sub-1pp de acurácia) mas custa CPU. MicroSpeech (Google) usa
só log-mel, e nossa rede aprende a "DCT implícita" se precisar.

### Por que int8 com calibração e não dynamic range?
Calibração com representative_dataset usa as estatísticas reais do
dataset pra escolher escalas. Dynamic range usa heurísticas e perde
mais acurácia. Custo: ter um pequeno subset de calibração (100
amostras).

### Por que DS-CNN e não LSTM/Transformer?
LSTM/Transformer pra áudio são bem mais pesados em RAM (estado
recorrente) e os ops não são todos suportados no TFLM int8.
DS-CNN é a arquitetura padrão de KWS embarcado desde 2017 (paper
Zhang et al. "Hello Edge").

### Por que palavras em inglês?
Speech Commands é o dataset clássico de KWS, está em inglês e tem
105k amostras balanceadas. Coletar 100+ amostras por palavra em
PT-BR levaria semanas e seria desnecessário pra demo (o pipeline é
o mesmo, só muda o idioma).

### Por que a BitDogLab e não ESP32-S3 (que era o "principal" da disciplina)?
- A BitDogLab traz o microfone, matriz 5x5 e OLED **já soldados** —
  zero hardware adicional pra demo
- O prof explicitamente permite outras placas (slide 6 do PDF do
  projeto final cita Raspberry Pi Pico)
- Demo física > simulador na hora da apresentação
