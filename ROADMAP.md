# Roadmap — o que está pronto e o que falta

Este projeto foi entregue **estruturalmente completo** mas você (Rose) precisa
fazer 3 coisas pra ele rodar de verdade na placa. Cada uma está marcada com
🔴 abaixo.

---

## ✅ O que está pronto

### Treino
- [x] Notebook completo `treino/treino_kws.ipynb` rodável no Colab
- [x] Pipeline Keras → TFLite int8 → array C automático
- [x] Avaliação no test set com matriz de confusão
- [x] Export do `model_data.c` pronto pra copiar pro firmware

### Firmware (estrutura)
- [x] `CMakeLists.txt` configurado pro Pico SDK + pico-tflmicro
- [x] Captura de áudio (ADC + DMA) a 16 kHz no GPIO28
- [x] Extração de features (FFT + filterbank Mel 40 bins)
- [x] Wrapper TFLite Micro (MicroMutableOpResolver + arena de 30 KB)
- [x] Driver da matriz 5x5 WS2812 via PIO
- [x] 6 pictogramas 5x5 desenhados
- [x] Driver do OLED SSD1306 (I2C)
- [x] Loop principal integrando tudo
- [x] LED RGB de status (estado do firmware: gravando/inferindo/pronto)

### Documentação
- [x] README com visão geral
- [x] COMO_TESTAR.md com instalação Windows passo a passo
- [x] docs/arquitetura.md com diagrama de blocos
- [x] docs/perguntas_prof.md com 20 perguntas-tipo + respostas

---

## 🔴 O que falta você fazer

### 🔴 1. Treinar o modelo (~30 min)

```
1. Abrir treino/treino_kws.ipynb no Google Colab
2. Runtime → Change runtime type → GPU (T4 grátis)
3. Run all cells
4. Baixar o arquivo model_data.c gerado no fim
5. Substituir firmware/model_data.c por esse arquivo
```

**Por que está como TODO:** o modelo precisa rodar no seu Colab/conta pra você
poder explicar pro prof como foi treinado (perguntas individuais cobram isso).

### 🔴 2. Instalar Pico SDK no Windows (~1 hora)

Tem 2 caminhos no `COMO_TESTAR.md`:

- **Caminho A (recomendado, mais fácil):** extensão **Raspberry Pi Pico** no
  VS Code — instala SDK, toolchain ARM e CMake automaticamente
- **Caminho B (manual):** instalador `pico-setup-windows`

### 🔴 3. Compilar + flashar + testar (~30 min)

Depois que o SDK estiver instalado e o `model_data.c` substituído:

```powershell
cd firmware
mkdir build
cd build
cmake -G "Ninja" ..
ninja
```

Sai um `kws_bitdoglab.uf2`. Pressiona BOOTSEL na placa enquanto conecta
o USB, e arrasta o `.uf2` pra unidade "RPI-RP2" que aparece.

Detalhes completos em `COMO_TESTAR.md`.

---

## ⚠️ Riscos conhecidos

### 1. Mic analógico ruidoso
O microfone da BitDogLab é eletreto analógico, não I2S — então tem mais
ruído. Mitigação: o pré-processamento aplica remoção de DC offset + 
normalização (igual a Aula 6 slide 21).

Se a acurácia em campo cair muito, você pode aumentar o threshold de 
confiança em `main.c` (variável `CONFIDENCE_THRESHOLD`, default 0.7).

### 2. Latência
RP2040 não tem unidade SIMD/DSP. Inferência deve ficar em 100-200ms.
Se passar de 300ms, simplificar o modelo no notebook (reduzir `n_filters`).

### 3. Modelo grande pra flash
Se o `model_data.c` ficar maior que 80 KB depois da quantização, o linker
do Pico vai reclamar. Mitigação documentada no notebook.

### 4. Pico SDK no Windows
Foi a parte que mais dá problema. Por isso a recomendação de usar a 
extensão oficial do VS Code, que cuida de tudo.

---

## 📅 Cronograma sugerido (3 semanas)

| Semana | O quê | Entregável |
| --- | --- | --- |
| 1 | Instalar Pico SDK + compilar "hello world" do Pico | Pisca o LED da placa |
| 1 | Compilar este projeto SEM modelo (com placeholder) | Matriz acende um ícone fixo |
| 2 | Rodar notebook + treinar modelo + ver matriz de confusão | model_data.c gerado |
| 2 | Substituir model_data.c, recompilar, testar com voz | Reconhece pelo menos 2 das 4 palavras |
| 3 | Coletar amostras pra fine-tuning se necessário | Acurácia > 85% em uso |
| 3 | Gravar vídeo demo (~1 min) | Vídeo |
| 3 | Montar slides (10 min de apresentação) | PDF |
| 3 | Estudar `docs/perguntas_prof.md` | Preparada pras perguntas |

---

## 🎯 Critério de "feito"

Pra apresentar com confiança, antes do dia da entrega o checklist mínimo é:

- [ ] Modelo treinado, acurácia ≥ 80% no test set
- [ ] Firmware compilando sem warning
- [ ] Placa reconhece ao menos 2 das 4 palavras quando você fala perto do mic
- [ ] Pictograma correto acende na matriz
- [ ] OLED mostra label e % de confiança
- [ ] Vídeo de 30-60s mostrando o funcionamento
- [ ] Slides em PDF
- [ ] Repositório público no GitHub seguindo git flow
- [ ] Arquivo `.txt` com link do repo + PDF anexo (formato da entrega oficial)
