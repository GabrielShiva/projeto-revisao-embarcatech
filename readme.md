# Projeto de Revisão - Embarcatech - Fase 2

## Objetivo
O projeto é um sistema interativo que demonstra a integração de múltiplos periféricos (joystick, botões, display, LEDs, buzzer) em um Raspberry Pi Pico W. Ele permite controlar um cursor (quadrado 8x8) no display OLED via joystick, ajustar um volume (0-10) com botões, visualizar o volume em uma matriz de LEDs, alternar entre LEDs verde e vermelho, e emitir sons no buzzer conforme o volume. Serve como uma demonstração de controle de hardware, tratamento de interrupções, e feedback visual/auditivo em tempo real.

## Funcionalidades
- Controle do Cursor (Joystick):
    -  O joystick captura movimentos nos eixos X/Y via ADC. Esses valores são convertidos em posições normalizadas para mover um quadrado 8x8 no display OLED, mantendo-o dentro dos limites da tela.
- Controle de Volume (Botões A/B):
    - Botão A: Decrementa volume_state (se led_rgb_state = true).
    - Botão B: Incrementa volume_state (se led_rgb_state = true).
    - O volume varia de 0 a 10 e define a frequência do buzzer (200 Hz a 2000 Hz).
- Alternância de LEDs (Botão SW):
    - Pressionar o botão SW do joystick inverte led_rgb_state, alternando entre LED verde (ligado) e vermelho (desligado).
- Feedback na Matriz de LEDs:
    - Se led_rgb_state = true, a matriz exibe o sprite correspondente ao volume atual (0-10). Se false, a matriz é desligada.
- Buzzer:
    - A frequência do buzzer é proporcional ao volume (volume_state). Se led_rgb_state = false ou volume_state = 0, o buzzer é desligado.
- Interrupções e Debounce:
    - Botões (A, B, SW) acionam interrupções com tratamento de debounce para evitar leituras múltiplias. Ações são executadas apenas após um intervalo mínimo (debounce_delay_ms).

## Escopo de Projeto
- [x] Leitura analógica por meio do potenciômetro do joystick, utilizando o conversor ADC do
RP2040;
- [x] Leitura de botões físicos (push-buttons) com tratamento de debounce, essencial para garantir a
confiabilidade das entradas digitais;
- [x] Utilização da matriz de LEDs, do LED RGB e do buzzer como saídas para feedback visual e
sonoro;
- [x] Exibição de informações em tempo real no display gráfico 128x64 (SSD1306), que se comunica
com o RP2040 via interface I2C;
- [x] Transmissão de dados e mensagens de depuração através da interface UART, permitindo a
visualização em um terminal serial no computador;
- [x] Emprego de interrupções para o tratamento eficiente de eventos gerados pelos botões;
- [x] Estruturação do projeto no ambiente VS Code, previamente configurado para o desenvolvimento
com o RP2040;
- [x] o projeto deverá exibir no display SSD1306 um quadrado de 8x8 pixels, inicialmente centralizado, que se moverá proporcionalmente aos valores capturados pelo
joystick.