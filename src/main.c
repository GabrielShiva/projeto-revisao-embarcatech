#include <stdio.h> // inclui a biblioteca padrão para I/O 
#include <stdlib.h> // utilizar a função abs
#include "pico/stdlib.h" // inclui a biblioteca padrão do pico para gpios e temporizadores
#include "hardware/adc.h" // inclui a biblioteca para manipular o hardware adc
#include "hardware/pwm.h"
#include "hardware/irq.h" // inclui a biblioteca para interrupções
#include "hardware/i2c.h" // inclui a biblioteca para utilizar o protocolo i2c
#include "inc/ssd1306.h" // inclui a biblioteca com definição das funções para manipulação do display OLED
#include "inc/font.h" // inclui a biblioteca com as fontes dos caracteres para o display OLED

#include "ws2818b.pio.h"
#include "inc/leds_matrix.h"
#include "inc/convert_to_rgba.h"
#include "inc/sprites.h"

#define LED_R 13
#define LED_G 11
#define JOYSTICK_SW 22
#define JOYSTICK_X 27
#define JOYSTICK_Y 26
#define BTN_A 5
#define BTN_B 6
#define BUZZER_PIN 21

#define MATRIX_ROWS 5
#define MATRIX_COLS 5
#define MATRIX_LEDS 25 // define o numero de LEDS da matriz

// Define os valores máximo e mínimo para configuração
#define VOL_MIN 0
#define VOL_MAX 10

// definição de parametros para o protocolo i2c
#define I2C_ID i2c1
#define I2C_FREQ 400000
#define I2C_SDA 14
#define I2C_SCL 15
#define SSD_1306_ADDR 0x3C

float buzzer_freq = 0.0;

// inicia a estrutura do display OLED
ssd1306_t ssd;

// definição de constantes para o display
volatile uint volume_scale = 0;

const uint16_t central_x_pos = 2049;
const uint16_t central_y_pos = 1988;

// define variáveis para debounce do botão
volatile bool btn_a_state = false;
volatile bool btn_b_state = false;
volatile uint32_t last_time_btn_press = 0; 

// debounce delay 
const uint32_t debounce_delay_ms = 260;

// define qual o LED RGB que estará ligado (vermelho (0) ou verde (1))
volatile bool led_rgb_state = true;

npLED_t leds[LED_COUNT];
int rgb_matrix[MATRIX_ROWS][MATRIX_COLS][LED_COUNT];    

// configurações para o PWM do buzzer
uint slice_num;
uint channel_num;

// inicializa os botões
void btn_init(uint gpio) {
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_IN);
    gpio_pull_up(gpio);
}

void insert_sprite(int sprite_index) {
    npLED_t leds[LED_COUNT];
    int rgb_matrix[MATRIX_ROWS][MATRIX_COLS][3];

    convertARGBtoMatriz(matrix_sprites[sprite_index], rgb_matrix);
    spriteWrite(rgb_matrix, leds);
    matrizWrite(leds);
}

// configuração do protocolo i2c
void i2c_setup() {
    // inicia o modulo i2c (i2c1) do rp2040 com uma frequencia de 400kHz
    i2c_init(I2C_ID, I2C_FREQ);

    // define o pino 14 como o barramento de dados
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    // define o pino 15 como o barramento de clock
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);

    // ativa os resistores internos de pull-up para os dois barramentos para evitar flutuações nos dois barramentos quando está no estado de espera (idle)
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

void display_init(){
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, SSD_1306_ADDR, I2C_ID); // Inicializa o display
    ssd1306_config(&ssd); // Configura o display
    ssd1306_send_data(&ssd); // Envia os dados para o display
  
    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}

// inicialização e configuração do hardware adc
void adc_setup() {
    // inicializa o hardware adc
    adc_init();
    
    // inicialização dos pinos analógicos
    adc_gpio_init(JOYSTICK_X);
    adc_gpio_init(JOYSTICK_Y);
}

// realiza a leitura do canal especificado e retorna o valor digital convertido
uint16_t adc_start_read(uint channel_selected) {
    uint16_t channel_value = 0;

    if (channel_selected == 0) {
        // seleciona o canal para o eixo x e realiza a leitura e conversão para o valor digital e armazena em uma variável de 2 bytes
        adc_select_input(0);
        // lê o valor do canal 0 para o eixo x [0-4095]
        channel_value = adc_read();
    } else if (channel_selected == 1) {
        // seleciona o canal para o eixo y e realiza a leitura e conversão para o valor digital e armazena em uma variável de 2 bytes
        adc_select_input(1);
        // lê o valor do canal 0 para o eixo y [0-4095]
        channel_value = adc_read();
    } 

    return channel_value;
}

uint16_t adc_convert_value(uint16_t central_pos, uint16_t raw_value) {
    uint16_t current_differential_pos = abs(central_pos - raw_value);

    float conversion_factor = 4095/2047.0;

    uint16_t value_converted = current_differential_pos * conversion_factor;

    return value_converted;
}

void set_display_border() {
    // limpa o display
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
    ssd1306_rect(&ssd, 1, 1, 126, 62, true, false);
}

void buzzer_init() {
    // Configura o pino do buzzer para PWM
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    channel_num = pwm_gpio_to_channel(BUZZER_PIN);
    
    // Configuração inicial do PWM
    pwm_config config = pwm_get_default_config();
    pwm_init(slice_num, &config, true);
    pwm_set_enabled(slice_num, false); // desliga PWM do pino ligado ao buzzer
}

void define_buzzer_state() {
    if(led_rgb_state && volume_scale > 0) {
        buzzer_freq = 200.0f + (volume_scale - 1) * 200.0f;
        
        // Cálculos para configuração do PWM
        uint32_t clock = 125000000; // Clock base de 125MHz
        uint32_t divider = 125000000 / (uint32_t)(buzzer_freq * 1000);
        uint32_t wrap = 125000000 / (divider * (uint32_t)buzzer_freq) - 1;
        
        // Aplica as configurações
        pwm_set_clkdiv_int_frac(slice_num, divider, 0);
        pwm_set_wrap(slice_num, wrap);
        pwm_set_chan_level(slice_num, channel_num, wrap / 2); // Define o Duty cycle de 50%
        pwm_set_enabled(slice_num, true);
    } else {
        // Desliga o PWM
        pwm_set_enabled(slice_num, false);
        gpio_put(BUZZER_PIN, 0); // Garante o silêncio
    }
}

// função para tratar as interrupções das gpios
void gpio_irq_handler(uint gpio, uint32_t events) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time()); // retorna o tempo total em ms desde o boot do rp2040

    // verifica se a diff entre o tempo atual e a ultima vez que o botão foi pressionado é maior que o tempo de debounce
    if (current_time - last_time_btn_press > debounce_delay_ms) {
        last_time_btn_press = current_time;

        // verifica se o botão A foi pressionado
        if (gpio == BTN_A) {
            if (led_rgb_state && volume_scale > VOL_MIN) {
                volume_scale = volume_scale - 1;
            }

            printf("Botao A pressionado!\n");
        } else if (gpio == BTN_B) { // verifica se o botão B foi pressionado
            if (led_rgb_state && volume_scale < VOL_MAX) {
                volume_scale = volume_scale + 1;
            }

            printf("Botao B pressionado!\n");
        } else if (gpio == JOYSTICK_SW) { // verifica se o botão SW foi pressionado
            led_rgb_state = !led_rgb_state; // muda o led que estará aceso (vermelho ou verde)
            
            printf("Botao do joystick (SW) pressionado!\n");

            led_rgb_state ? 
                printf("LED aceso: verde!\n") :
                printf("LED aceso: vermelho!\n");
        }

        printf("Volume: %d\n", volume_scale);
        printf("BUZZER: %.2f\n", buzzer_freq);
    } 
}

// calcula a nova posição do quadrado de acordo com as coordenadas fornecidas pelo joystick
void move_square(uint16_t x_value, uint16_t y_value) {
    // calcula a distância do centro
    int16_t x_diff = x_value - central_x_pos;
    int16_t y_diff = central_y_pos - y_value;

    // normaliza a distância do centro para a faixa [-1, 1]
    float max_displacement_x = 2049.0f;
    float max_displacement_y = 2107.0f;
    float normalized_x = (float)x_diff / max_displacement_x;
    float normalized_y = (float)y_diff / max_displacement_y;

    // calcula a nova posição do quadrado que está centrado inicialmente em (60, 28)
    int new_x = 60 + (int)(normalized_x * 60.0f);
    int new_y = 28 + (int)(normalized_y * 28.0f);

    // limita as posições para o tamnho da tela
    new_x = (new_x < 0) ? 0 : (new_x > 120) ? 120 : new_x;
    new_y = (new_y < 0) ? 0 : (new_y > 56) ? 56 : new_y;

    // cria o quadrado 8X8
    ssd1306_rect(&ssd, new_y, new_x, 8, 8, true, true);
}

int main() {
    // chama função para comunicação serial via usb para debug
    stdio_init_all(); 

    // chama a função para configuração do protocolo i2c
    i2c_setup();

    // inicializa o display OLED
    display_init();
    
    // chama a função que inicializa o adc
    adc_setup();

    // inicializa os LEDs RGB
    gpio_init(LED_R);
    gpio_set_dir(LED_R, GPIO_OUT);
    gpio_put(LED_R, !led_rgb_state);

    gpio_init(LED_G);
    gpio_set_dir(LED_G, GPIO_OUT);
    gpio_put(LED_G, led_rgb_state);

    // configuração dos botões B, A e SW
    btn_init(BTN_A);
    btn_init(BTN_B);
    btn_init(JOYSTICK_SW);

    // configuração da interrupção para o botão A, B e SW
    gpio_set_irq_enabled_with_callback(BTN_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled(BTN_B, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(JOYSTICK_SW, GPIO_IRQ_EDGE_FALL, true);

    //Inicializa a matriz de LEDs
    matrizInit(LED_PIN, leds);
    
    // limpa a matriz de leds
    npClear(leds);
    matrizWrite(leds);
    
    // Configuração do buzzer
    buzzer_init();

    while (true) {
        // define o tipo de borda
        set_display_border();

        // Limpa a matriz de LEDs
        npClear(leds);

        // aplica o estado atual para os LED
        if (led_rgb_state) {
            gpio_put(LED_G, 1); 
            gpio_put(LED_R, 0); 
        } else {
            gpio_put(LED_R, 1); 
            gpio_put(LED_G, 0); 
        }

        // realiza leitura para o eixo x
        uint16_t x_value = adc_start_read(1);
        // converte o valor para controle da intensidade do led vermelho tomado como menor intensidade a posição central
        uint16_t x_value_converted = adc_convert_value(central_x_pos, x_value);

        // realiza leitura para o eixo y
        uint16_t y_value = adc_start_read(0);

        // converte o valor para controle da intensidade do led azul tomado como menor intensidade a posição central
        uint16_t y_value_converted = adc_convert_value(central_y_pos, y_value);

        // chama a função que calcula a nova posição do quadrado de acordo com as coordenadas do joystick
        move_square(x_value, y_value);

        // atualiza o display OLED
        ssd1306_send_data(&ssd);

        if (led_rgb_state) {
            insert_sprite(volume_scale);
        } else {
            npClear(leds);
        } 

        define_buzzer_state();

        // Atualiza a matriz de LEDs
        matrizWrite(leds);

        // adiciona um atraso de 60ms para criar tempo para vizualização dos dados no monitor serial
        sleep_ms(60);
    }

    return 0;
}
