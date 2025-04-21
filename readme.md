# Projeto de Revisão - Embarcatech - Fase 2

- [x] Leitura analógica por meio do potenciômetro do joystick, utilizando o conversor ADC do
RP2040;
- [x] Leitura de botões físicos (push-buttons) com tratamento de debounce, essencial para garantir a
confiabilidade das entradas digitais;
- Utilização da matriz de LEDs, do LED RGB e do buzzer como saídas para feedback visual e
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