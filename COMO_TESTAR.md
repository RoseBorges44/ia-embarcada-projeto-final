# Como compilar, flashar e testar na BitDogLab

Passo a passo no **Windows 11**. Tempo estimado: 1-2 horas na primeira vez
(quase tudo é instalar o Pico SDK).

---

## Sumário

1. [Instalar o Pico SDK](#1-instalar-o-pico-sdk-caminho-fácil-do-vs-code)
2. [Clonar o pico-tflmicro](#2-clonar-o-pico-tflmicro)
3. [Treinar o modelo no Colab](#3-treinar-o-modelo-no-colab)
4. [Substituir o `model_data.c` placeholder](#4-substituir-o-model_datac-placeholder)
5. [Compilar o firmware](#5-compilar-o-firmware)
6. [Flashar a BitDogLab](#6-flashar-a-bitdoglab)
7. [Testar e ver o serial](#7-testar-e-ver-o-serial)
8. [Troubleshooting](#8-troubleshooting)

---

## 1. Instalar o Pico SDK (caminho fácil — VS Code)

Existe uma extensão oficial do **Raspberry Pi** que instala SDK, toolchain
ARM, CMake e Ninja automaticamente. Vai por ela, é muito mais simples.

### Passos:

1. Abrir o **VS Code**
2. `Ctrl+Shift+X` → buscar **"Raspberry Pi Pico"** (autor: Raspberry Pi)
3. Clicar **Install**
4. Esperar a barra inferior mostrar "Pico SDK installed"
   - Ela baixa ~600 MB. Demora 10-20 min dependendo da internet.

A extensão cria automaticamente:
- `C:\Users\rosej\.pico-sdk\sdk\<versão>\`
- `C:\Users\rosej\.pico-sdk\toolchain\<versão>\`
- E configura as variáveis de ambiente

### Caminho alternativo (manual):

Se preferir não usar VS Code, baixa o instalador `pico-setup-windows`
em https://github.com/raspberrypi/pico-setup-windows/releases — gera um
ambiente standalone com tudo configurado.

---

## 2. Clonar o pico-tflmicro

É o porte oficial do **TensorFlow Lite Micro** pro RP2040, mantido pela
própria Raspberry Pi. Coloca **ao lado** da pasta `firmware/`, não dentro.

Abre o PowerShell na pasta do projeto:

```powershell
cd "C:\Users\rosej\OneDrive\Desktop\IA Embarcada\Projeto_final"
git clone --recurse-submodules https://github.com/raspberrypi/pico-tflmicro.git
```

Estrutura final deve ficar:

```
Projeto_final/
├── firmware/         ← seu código
├── pico-tflmicro/    ← TFLite Micro pro RP2040 (recém-clonado)
├── treino/
└── ...
```

⚠️ **Atenção:** clone pode demorar (~300 MB com submodules). Se cair,
roda de novo dentro de `pico-tflmicro/`:

```powershell
cd pico-tflmicro
git submodule update --init --recursive
```

---

## 3. Treinar o modelo no Colab

1. Vai em https://colab.research.google.com
2. **File → Upload notebook** → escolhe `treino/treino_kws.ipynb`
3. **Runtime → Change runtime type → T4 GPU** (grátis)
4. **Runtime → Run all** — leva ~15-20 minutos
5. No fim, ele baixa automaticamente 3 arquivos:
   - `model_data.c` ← o modelo embarcado
   - `model_data.h` ← parâmetros (escalas de quantização)
   - `model.tflite` ← backup do modelo binário

**O que olhar enquanto roda:**
- A matriz de confusão tem que ter diagonal forte (> 80% por classe)
- O tamanho final do modelo tem que ser < 80 KB
- Se algo der errado, o erro fica nas células do notebook

---

## 4. Substituir o `model_data.c` placeholder

Copia os 2 arquivos baixados do Colab pra dentro de `firmware/`,
**sobrescrevendo** os placeholders:

```
firmware/model_data.c  ← substituir
firmware/model_data.h  ← substituir
```

⚠️ Se você não fizer isso, o firmware vai **compilar** mas o ADC vai
detectar áudio e o `inference_run` vai falhar (modelo inválido). É
sinal claro que falta substituir.

---

## 5. Compilar o firmware

### Pelo VS Code (recomendado)

1. Abre o VS Code na pasta `firmware/`
2. `Ctrl+Shift+P` → **Raspberry Pi Pico: Configure CMake**
3. Selecione **pico-w** quando perguntar a board
4. `Ctrl+Shift+P` → **Raspberry Pi Pico: Compile Project**

Saída esperada na barra inferior:

```
[100%] Built target kws_bitdoglab
```

E aparece o arquivo `firmware/build/kws_bitdoglab.uf2` (~150 KB).

### Pela linha de comando (alternativa)

Abre o **Pico - Visual Studio Code** terminal (não o PowerShell normal —
precisa do env do SDK carregado):

```powershell
cd firmware
mkdir build
cd build
cmake -G Ninja ..
ninja
```

---

## 6. Flashar a BitDogLab

1. **Desconecta** o USB-C da placa
2. **Segura** o botão `BOOTSEL` (botão pequeno preto, próximo ao USB)
3. **Conecta** o USB-C mantendo o `BOOTSEL` pressionado
4. Solta o `BOOTSEL`
5. Uma unidade nova aparece no Windows: **`RPI-RP2`** (parece um pendrive)
6. **Arraste** o arquivo `firmware/build/kws_bitdoglab.uf2` pra essa unidade
7. A placa reinicia sozinha em ~2 segundos, a unidade desaparece e o firmware
   começa a rodar

---

## 7. Testar e ver o serial

### Sinais de que está funcionando

- LED RGB de 5mm da BitDogLab acende **verde** = pronto pra escutar
- Display OLED mostra "KWS BitDogLab" + "Fale!"
- Matriz 5x5 fica apagada esperando

### Falar com a placa

1. Aproxima a boca uns 10-15 cm do microfone (etiqueta "MIC" no canto da placa)
2. Fala uma das palavras claramente: `happy`, `yes`, `no`, `stop`
3. LED RGB fica **amarelo** enquanto processa (~300-500 ms)
4. Matriz acende o pictograma correspondente (coração, check, X, mão de pare)
5. OLED mostra o label + % de confiança
6. Após 2 segundos volta ao estado "pronto"

### Ver o log no serial

A BitDogLab aparece como porta COM no Windows. Pra abrir:

**VS Code:** `Ctrl+Shift+P` → **Raspberry Pi Pico: Open serial monitor**

**Alternativa (PuTTY/TeraTerm):** `COMx` a 115200 baud, 8N1.

Saída esperada:

```
=== KWS BitDogLab — IA Embarcada ===
Setup ok. Aguardando voz (threshold=800)...
Trigger! level=1245, capturando...
Audio: 1002ms | Features: 87ms | Inferência: 124ms
Predição: yes (89%)
   happy      0.04
   yes        0.89
   no         0.03
   stop       0.02
   silence    0.01
   unknown    0.01
```

---

## 8. Troubleshooting

### "cmake: command not found" / "ninja: command not found"
Você abriu o terminal errado. Use **"Pico - Visual Studio Code"** terminal
(criado pela extensão), não PowerShell genérico.

### "pico-tflmicro não encontrado"
Você clonou em outra pasta. Move pra `Projeto_final/pico-tflmicro/` ou
edita `PICO_TFLMICRO_PATH` no `CMakeLists.txt`.

### Compila mas a placa só fica vermelha (LED RGB)
Indica que `inference_init` falhou. 99% é porque você esqueceu de
substituir o `model_data.c` placeholder pelo modelo treinado.

### O serial não aparece
- Garante que `pico_enable_stdio_usb(kws_bitdoglab 1)` está no CMakeLists (já está)
- Espera ~3 segundos depois de plugar o USB (driver Windows demora)
- Se nada aparecer, pressiona `BOOTSEL` e reflasha

### Acurácia ruim em campo (acerta só 50% mesmo o modelo dando 95% no test set)
- O microfone analógico capta mais ruído que os mics usados no Speech Commands
- Mitigações em ordem de esforço:
  1. Aumenta `CONFIDENCE_THRESHOLD` em `main.c` (0.6 → 0.8)
  2. Fala mais perto (5-10 cm) e mais alto
  3. Coleta amostras suas e fine-tune o modelo (rodar células extras do notebook)

### "Arena allocations failed" no log
O modelo cresceu além dos 30 KB de arena. Aumenta em `inference.cc`:
```cpp
constexpr int kTensorArenaSize = 50 * 1024;
```

### A matriz 5x5 acende mas com pictograma "torto" ou "espelhado"
A tabela `LED_LOGICAL_TO_PHYSICAL` em `led_matrix.c` depende de como a placa
foi cabeada. Se a sua placa estiver com cabeamento diferente da v6.3 documentada,
ajusta a tabela.

### Travou no boot
Conecta segurando `BOOTSEL`, arrasta o `flash_nuke.uf2` (baixa em
`raspberrypi.com/documentation/microcontrollers/pico-series.html`) pra
limpar a flash e tenta de novo.
