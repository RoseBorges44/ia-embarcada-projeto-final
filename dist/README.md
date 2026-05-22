# Binário pronto pra flashar — `kws_bitdoglab.uf2`

Esse `.uf2` é o firmware **já compilado** do projeto KWS na BitDogLab.
Se você só quer **testar a placa** (sem compilar nada), é só seguir os
4 passos abaixo. Não precisa instalar Pico SDK, CMake, nada.

Se quiser compilar do zero, vai no [`COMO_TESTAR.md`](../COMO_TESTAR.md) na raiz do repo.

---

## Como colocar na placa (BitDogLab / Raspberry Pi Pico)

1. **Desconecta** o cabo USB-C da BitDogLab.
2. **Segura pressionado** o botão `BOOTSEL` (botão pequeno, do lado do conector USB-C da placa).
3. **Com o `BOOTSEL` ainda apertado**, conecta o cabo USB-C no computador.
4. **Solta o `BOOTSEL`** depois que conectou.

Pronto — uma unidade nova aparece no Windows chamada **`RPI-RP2`** (parece um pendrive).

5. **Arrasta** o arquivo `kws_bitdoglab.uf2` (esse que está aqui na pasta) **pra dentro** da unidade `RPI-RP2`.
6. A placa **reinicia sozinha** em ~2 segundos. A unidade `RPI-RP2` some e o firmware já está rodando.

---

## Como saber que funcionou

- LED RGB de 5 mm (canto da placa) acende **verde** → pronto pra ouvir
- Display **OLED** mostra `KWS BitDogLab` + `Fale!`
- Matriz **WS2812 5x5** fica apagada esperando comando

## Como usar

1. Aproxima a boca uns **10–15 cm do microfone** (etiqueta `MIC` no canto da placa).
2. Fala uma das 4 palavras claramente, em **inglês**:
   - `happy` → coração na matriz
   - `yes`   → ✓ (check) na matriz
   - `no`    → ✗ (X) na matriz
   - `stop`  → mão de pare na matriz
3. O LED RGB fica **amarelo** enquanto processa (~300–500 ms).
4. A matriz acende o pictograma + o OLED mostra a palavra reconhecida e a % de confiança.
5. Volta ao estado "pronto" depois de 2 segundos.

## Ver o log pelo serial (opcional)

A placa aparece como porta `COM` no Windows. Abre qualquer terminal serial a **115200 baud, 8N1** (PuTTY, TeraTerm, ou `Ctrl+Shift+P` → **Raspberry Pi Pico: Open serial monitor** no VS Code).

Saída esperada:

```
=== KWS BitDogLab — IA Embarcada ===
Setup ok. Aguardando voz (threshold=800)...
Trigger! level=1245, capturando...
Audio: 1002ms | Features: 87ms | Inferência: 124ms
Predição: yes (89%)
```

---

## Não funcionou?

- **Unidade `RPI-RP2` não apareceu** → você não entrou em modo BOOTSEL. Repete os passos 1–4, garantindo que o botão estava apertado **antes** de plugar o USB.
- **LED RGB acende vermelho e fica parado** → modelo não carregou. Provavelmente arrastou um `.uf2` antigo/errado. Confere se é o `kws_bitdoglab.uf2` desse `dist/`.
- **Acurácia ruim** → o mic analógico é sensível a ruído. Fala mais perto (5–10 cm), mais alto, e em ambiente silencioso.
- **Mais troubleshooting** → seção 8 do [`COMO_TESTAR.md`](../COMO_TESTAR.md).
