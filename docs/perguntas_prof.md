# Preparação pras 5 minutos de perguntas individuais

> A descrição oficial do projeto deixa claro: "perguntas específicas
> sobre o apresentado pra verificar nível de entendimento dos
> integrantes". Aqui estão **20 perguntas-tipo** que o prof costuma
> fazer em projeto de IA Embarcada, com respostas curtas que você
> pode dar de cabeça.

---

## 🎯 Sobre o problema

### 1. "Por que escolheu Keyword Spotting?"
Combina os 3 temas centrais da disciplina: pré-processamento de
sinais temporais (Aula 6), classificação supervisionada com modelo
compacto (Aulas 3 e 4), e deploy em microcontrolador com restrição
de RAM/flash (Aula 4). Além disso a BitDogLab tem todos os
periféricos necessários já soldados.

### 2. "Por que essas 4 palavras (happy, yes, no, stop)?"
São acusticamente bem distintas (poucos fonemas comuns), o que
ajuda o modelo a separar com pouco overfitting. E têm pictogramas
óbvios — facilita a demo visual.

### 3. "E se o dataset fosse em português?"
Mesma pipeline. Datasets PT-BR existem (FLA-Dataset com "direita/
esquerda/frente/pare" — Aula 6 slide 39) mas têm 100× menos amostras
que Speech Commands, então o modelo overfit-aria rápido. Pra demo de
final de semestre, EN é a escolha mais segura.

---

## 🎙️ Sobre captura e pré-processamento

### 4. "Por que 16 kHz e não 44.1 kHz?"
Teorema de Nyquist (Aula 6 slide 6): pra capturar até 8 kHz preciso
amostrar a ≥ 16 kHz. Voz humana fica entre ~80 Hz e 8 kHz — sub-amostrar
acima disso só adiciona memória sem ganho informacional.

### 5. "Por que remove DC offset?"
O mic analógico tem ponto de operação em ~Vcc/2 (alimentado
single-ended). Se eu não remover, o componente DC domina o espectro
e a FFT fica enviesada nos primeiros bins. Aula 6 slide 21 mostra
isso explicitamente.

### 6. "O que é janelamento (Hann) e por que precisa?"
A FFT assume sinal periódico. Recortar uma janela retangular cria
descontinuidade nas bordas → vazamento espectral (Aula 6 slide 25).
Multiplicar por Hann suaviza as bordas → espectro mais limpo.

### 7. "O ADC é 12 bits, por que armazena em int16?"
Pra ter cabeça pra remover DC offset sem perder resolução.
Subtraio a média (que é ~2048), o resultado pode ser negativo, e
multiplico por 16 pra ocupar toda a faixa int16.

---

## 🧮 Sobre as features

### 8. "Por que mel spectrogram e não FFT direto?"
O ouvido humano é não-linear em frequência (Aula 6 slide 34) — somos
melhores em discriminar graves do que agudos. A escala Mel imita
isso: bins logarítmicos. Pra fala, mel separa fonemas muito melhor
que magnitude linear da FFT.

### 9. "Por que 40 bins Mel e 49 frames?"
- 40 bins é o sweet spot pra KWS — menos perde fonemas, mais
  satura o modelo. Padrão do MicroSpeech (Aula 6 slide 42).
- 49 frames vem de: 1s − (30ms janela) = 970ms; 970/20ms = 48 strides + 1 = 49

### 10. "Por que log do mel spectrogram?"
Comprime a faixa dinâmica (Aula 6 slide 35 item 4). Som forte vs
fraco no tempo cobre 4-5 ordens de grandeza; log mapeia isso pra ~5
unidades, evitando saturação no quantizador int8.

### 11. "Por que não MFCC completo (com DCT)?"
DCT descorrelaciona, mas adiciona uma matmul 40×40 sem ganho
mensurável em KWS — a CNN aprende a transformação implícita.
Economiza ~20 KB de código.

---

## 🧠 Sobre o modelo

### 12. "Por que DS-CNN e não uma CNN normal?"
DepthwiseConv separa a convolução em duas: uma por canal (espacial)
+ uma 1×1 (mistura canais). Reduz parâmetros 4-8× com perda
acurácia mínima (Aula 6 slide 40). É a arquitetura padrão pra KWS
embarcado.

### 13. "Quantos parâmetros tem o modelo?"
~30 000 (depende do `n_filters`). Em int8 isso vira ~50 KB de
flash. Pra referência, o MicroSpeech (Aula 6 slide 44) cabe em 32 KB
flash + 20 KB RAM.

### 14. "Por que GlobalAveragePooling no fim?"
Substitui Flatten + Dense gigante. Sai com `n_filters` valores
independente do tamanho de entrada. Acaba com a maior fonte de
parâmetros e dá um efeito regularizador.

### 15. "Como mediu a acurácia?"
Test set oficial do Speech Commands v2 — ~11 000 áudios que o modelo
nunca viu. Avalio em float32 (Keras evaluate) e depois em int8
(TFLite interpreter sobre 1000 amostras pra confirmar que a
quantização não destruiu o modelo). Diferença típica: <2pp.

---

## 🗜️ Sobre quantização e deploy

### 16. "O que é quantização int8 com calibração?"
Cada peso/ativação é representado por inteiro 8-bit (−128 a 127)
com uma escala e zero point. A calibração roda 100 amostras
representativas pelo modelo float pra observar a faixa real de
cada tensor → escolhe escala que minimiza erro.

### 17. "Por que isso roda no RP2040 e não em float?"
Cortex-M0+ não tem FPU. Cada multiplicação float emulada custa
~50 ciclos, int8 custa 1 ciclo. Inferência float demoraria ~5s,
int8 demora ~120ms.

### 18. "O que é o `MicroMutableOpResolver`?"
Em vez de incluir todos os ~150 ops do TFLite (que ocupa ~200 KB
flash), só registra os que o modelo usa. Pro nosso: Conv2D,
DepthwiseConv2D, Mean, FullyConnected, Softmax, Reshape, Quantize,
Dequantize. Economiza ~150 KB.

### 19. "O que é a tensor arena?"
Bloco de RAM estático onde o TFLite Micro aloca todos os tensores
intermediários da rede. Tamanho calculado offline pelo próprio
TFLM no `AllocateTensors()`. No nosso caso, 30 KB é suficiente.

### 20. "Como sei que o modelo no firmware é o mesmo que treinei?"
O `model_data.c` é o `.tflite` binário convertido em array de bytes.
Checksum (sha256) bate. Além disso, o `model_data.h` carrega as
escalas exatas da quantização — se eu trocar de modelo sem trocar
o header, o `inference_init` falha no sanity check de dimensões.

---

## 🔧 Sobre o sistema completo

### 21. "Qual a latência total?"
~1.2 segundos: 1s pra capturar áudio (limite físico do clipe) +
~90ms features + ~120ms inferência + ~10ms display. Pra demo é
aceitável; pra produção real daria pra usar streaming com overlap.

### 22. "Como o sistema decide que alguém falou?"
RMS dos últimos 128 samples do mic > threshold (configurável,
default 800 sobre escala 0-32767). Quando passa, dispara a captura
de 1 segundo.

### 23. "E se for ruído tipo bater na mesa?"
Vai disparar a captura, mas o modelo tem a classe "unknown" treinada
com palavras que não estão na lista. Como a confiança vai ser baixa,
o firmware decide mostrar "?" em vez de uma das 4 palavras.

### 24. "Se eu falar uma palavra que não está na lista?"
Mesmo caso da 23 — cai em "unknown" se a confiança ficar
abaixo de 0.6. Se passar (modelo confiante de errado), aparece a
palavra mais próxima.

### 25. "Como eu adicionaria uma 5ª palavra?"
- No notebook: adicionar a palavra em `TARGET_WORDS`, retreinar
- No firmware: adicionar enum em `pictograms.h`, criar pictograma
  em `pictograms.c`, recompilar
- Substituir `model_data.c` pelo novo
