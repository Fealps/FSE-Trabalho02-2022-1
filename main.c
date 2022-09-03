#include "inc/bme280_driver.h"
#include "inc/pid.h"
#include "inc/csv.h"
#include "inc/esp32_control.h"
#include "inc/lcd_16x2_driver.h"

#include <stdlib.h>
#include <sys/time.h>
#include <signal.h>
#include <wiringPi.h>
#include <stdio.h>    
#include <softPwm.h>
#include <unistd.h>
#include <pthread.h>


#define MODE_TERMINAL       0x00
#define MODE_UART			0x01

#define CSV_LINES_MAX       50 
#define CSV_START_LINE      1      


struct csv_log_data log_data;
float var_temperatura_referencia = 0;

int mode = 0;
int uart = 0;

const int RESISTOR =    4;  /* GPIO23 */
const int VENTOINHA =   5;	/* GPIO24 */

long curve_lines = 0;
float curve_temperature[CSV_LINES_MAX];
long curve_time[CSV_LINES_MAX];
long curve_line = CSV_START_LINE;
long long curve_clock_last = 0;

char alimento[15];
pthread_t thread_id_log_csv;


void exit_thread(){
    pthread_exit(NULL);
}

long long timeInMicroseconds(void){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((((long long)tv.tv_sec)*1000000)+(tv.tv_usec));
}

void desliga(){
    printf(">>> DESLIGANDO! <<<\n");
    pthread_kill(thread_id_log_csv, SIGUSR1);

    softPwmWrite(RESISTOR, 0);
    softPwmWrite(VENTOINHA, 0);
    bme280_driver_close();

    lcd_clear();
    lcd_type_line("    SISTEMA");
    lcd_set_line(LCD_LINE2);
    lcd_type_line("   DESLIGADO!");

    liga_desliga(ESP_OFF);
    exit(0);
}

void temperatura_controle(){

    double var_temperatura_controle = pid_controle(log_data.var_temperatura_interna);
    if(var_temperatura_controle > 0){
        log_data.resistor_power = var_temperatura_controle;
        softPwmWrite(RESISTOR, var_temperatura_controle);
        log_data.fan_speed = 0;
        softPwmWrite(VENTOINHA, 0);
    }else if(var_temperatura_controle >= -40){
        log_data.fan_speed = 40;
        softPwmWrite(VENTOINHA, 40);
        log_data.resistor_power = 0;
        softPwmWrite(RESISTOR, 0);
    }else{
        log_data.fan_speed = -var_temperatura_controle;
        softPwmWrite(VENTOINHA, (int)-var_temperatura_controle);
        log_data.resistor_power = 0;
        softPwmWrite(RESISTOR, 0);
    }
    envia_temperatura_controle(var_temperatura_controle);

    printf("Temperatura de referencia: %2.1lf C'\n", var_temperatura_referencia);
    printf("Temperatura interna:       %2.1lf C'\n", log_data.var_temperatura_interna);
    printf("Potencia do resistor:      %2.1lf%%\n",  log_data.resistor_power);
    printf("Potencia da ventoinha:     %2.1lf%%\n",  log_data.fan_speed);
}

void lcd_routine(){
    lcd_clear();
    if(mode == MODE_TERMINAL)
        lcd_type_line(">> MODO TERMINAL ");
        lcd_set_line(LCD_LINE2);
        lcd_type_line(">>TI: ");
        lcd_type_float(log_data.var_temperatura_referencia);
        lcd_type_line(">>TR: ");
        lcd_type_float(log_data.var_temperatura_interna);
    else if(uart == MODE_TERMINAL){
            lcd_type_line("TI: ");
            lcd_type_float(log_data.var_temperatura_interna);
            lcd_type_line(" ");
            //lcd_type_line(alimento);
            lcd_set_line(LCD_LINE2);
            lcd_type_line("TIME: ");
            //lcd_type_line(temporestante);

        }
    lcd_type_float(log_data.var_temperatura_referencia);

    lcd_set_line(LCD_LINE2);
    lcd_type_line("TT:");
    lcd_type_float(timeInMicroseconds());

}

void comandos_usuario(){
    Byte command;
    comando_usuario(&command);

    switch(command){
    case ESP_USER_LIGA:
        printf(">>> LIGAR <<<n");
        liga_desliga(ESP_ON);
        break;
    case ESP_USER_DESLIGA:
        printf(">>> DESLIGAR <<<\n");
        desliga();
        break;
    case ESP_USER_AQUECE:
        printf(">>> AQUECENDO <<<\n");
        uart = ESP_USER_AQUECE;
        modo_controle(ESP_USER_AQUECE);
        break;
    case ESP_USER_CANCELA:
        printf(">>> CANCELANDO <<<\n");
        uart = ESP_USER_CANCELA;
        modo_controle(ESP_USER_CANCELA);
        break;
    case ESP_USER_ADICIONA:
        printf(">>> ADICIONANDO 1 MINUTO <<<\n");
        uart = ESP_USER_ADICIONA;
        modo_controle(ESP_USER_ADICIONA);
        break;
    case ESP_USER_REMOVE:
        printf(">>> REMOVENDO 1 MINUTO <<<\n");
        uart = ESP_USER_REMOVE;
        modo_controle(ESP_USER_REMOVE);
        break;
    case ESP_USER_MENU:
        printf(">>> ABRINDO MENU <<<\n");
        uart = ESP_USER_MENU;
        modo_controle(ESP_USER_MENU);
        break;
    }
}

void temperatura_controle_routine(){

    if(mode == MODE_UART){
        if(uart == MODE_TERMINAL){
            temperatura_referencia(&var_temperatura_referencia);
        }else if(curve_line < curve_lines){
            if((timeInMicroseconds() - curve_clock_last)/1000000 >= curve_time[curve_line]){
                var_temperatura_referencia = curve_temperature[curve_line++];
            }
        }
    }
    
    log_data.var_temperatura_referencia = var_temperatura_referencia;
    pid_atualiza_referencia(var_temperatura_referencia);
	envia_temperatura_referencia(var_temperatura_referencia);

    bme280_get_temperature(&log_data.var_temperatura_externa);

    temperatura_controle();

    float var_temperatura_interna;
    temperatura_interna(&var_temperatura_interna);
    log_data.var_temperatura_interna = var_temperatura_interna;
}

void loop_routine(){
    printf("\n>>> Executando <<<\n");
    modbus_open();
    comandos_usuario();
    temperatura_controle_routine();
    modbus_close();
    lcd_routine();
}

void* loop(){

    long long wait = 0;
    long long currentTime = 0;
    long long lastTime = timeInMicroseconds();
    pthread_t id = pthread_self();

    while(1){
        currentTime = timeInMicroseconds();
        
        if(pthread_equal(id, thread_id_log_csv)){
            csv_append_log(log_data);
            wait += 1000000 - (currentTime - lastTime);
        }else{
            loop_routine();
            wait += 500000 - (currentTime - lastTime);
        }

        if(wait > 0){
            usleep(wait);
		    wait = 0;
        }
        lastTime = timeInMicroseconds();
    }
}

void modo_leitura(){

    char option;
    printf("\nSelecione o modo de temperatura de referencia:\n\n");
    do{
        printf("\n\t(1) Terminal.\n");
        printf("\t(2) UART.\n");
        printf("=======> ");
        scanf(" %c", &option);

        switch(option){
        case '1':
			printf("\nDigite a temperatura de referencia desejada:\n\n");
			printf("=======> ");
			scanf(" %f", &var_temperatura_referencia);
			mode = MODE_TERMINAL;
            break;

        case '2':
			mode = MODE_UART;
            break;

        default:
            printf("\nEntrada invalida! Selecione novamente:\n");
            option = '\0';
        }
    }while(option == '\0');
    modo_controle(mode);
}

int main(void){

    signal(SIGINT, desliga);
    signal(SIGUSR1, exit_thread);
    //setando estado inicial do sistema
    liga_desliga(ESP_OFF);
    modo_controle(ESP_USER_MENU);
    printf(">>> INICIALIZANDO! <<<\n");

    if(wiringPiSetup() == -1) exit(1);
    if(lcd_init() == -1) exit(1);
    if(bme280_driver_init() != BME280_OK) exit(1);  
    
    lcd_clear();
    lcd_type_line("    SISTEMA");
    lcd_set_line(LCD_LINE2);
    lcd_type_line(" INICIALIZANDO");

    pinMode(RESISTOR, OUTPUT);
	pinMode(VENTOINHA, OUTPUT);
	softPwmCreate(RESISTOR, 1, 100);
    softPwmCreate(VENTOINHA, 1, 100);
	pid_configura_constantes(30.0, 0.2, 400.0);

    modo_leitura();
    csv_create_log();
    
    modbus_open();
    if(mode == MODE_TERMINAL){
        modo_controle(ESP_MODO_DASHBOARD);
    }else{
        modo_controle(ESP_MODO_TERMINAL);
        curve_lines = csv_read_csv_curve(curve_temperature, curve_time);    
    }
    liga_desliga(ESP_ON);
    modbus_close();
    sleep(1);
    printf(">>> SISTEMA LIGADO! <<<\n");
    pthread_create(&thread_id_log_csv, NULL, loop, NULL);
    loop();

    return 0;
}