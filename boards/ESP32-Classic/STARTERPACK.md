# BRUCER LITE ESP32 DOIT

Perfil exclusivo para o kit ESP32 clássico com OLED SSD1306 bicolor 128x64 e três botões.

Versão desta build: **0.9**.

## Hardware

- ESP32-D0WD-V3 / DOIT ESP32 DevKit V1, flash de 4 MB
- OLED SSD1306 I2C: SDA GPIO21, SCL GPIO22, endereço 0x3C (fallback 0x3D)
- D32: descer / próxima opção
- D33: subir em listas / direita ou próximo campo quando a tela oferecer essa ação
- D25: selecionar; pressionar por 700 ms para voltar
- D33 + D32 + D25 por 2 segundos: reinício de emergência

Os botões devem fechar cada GPIO diretamente com GND. A entrada usa `INPUT_PULLUP`, considera
pressionado em nível `LOW` e aplica debounce de 20 ms. Não ligue o sinal dos botões ao 3,3 V.

## Interface 128x64

- faixa física amarela (linhas 0..15): título, posição e contexto
- faixa física azul (linhas 16..63): conteúdo e seleção
- menus em lista com quatro opções visíveis
- teclado, diálogos, avisos, progresso e navegador de arquivos compactos
- fonte limitada a 1x para impedir que telas originalmente feitas para TFT saiam da área visível
- imagens e ferramentas muito gráficas são recortadas pelo driver; quando necessário, a WebUI continua
  disponível como interface complementar

## Build

Ambiente PlatformIO: `esp32doit-devkit-v1-headless`

Binário final: `BRUCER-LITE-ESP32DOIT.bin`
