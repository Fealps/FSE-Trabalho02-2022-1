#include "esp32_control.h"
#include <stdlib.h>
#include <string.h>
/*
0x01  0x23 0xC1 matr  Solicita Temperatura Interna    0x00 0x23 0xC1 + float (4 bytes)
0x01  0x23 0xC2 matr  Solicita Temperatura de Referência  0x00 0x23 0xC2 + float (4 bytes)
0x01  0x23 0xC3 matr  Lê comandos do usuário  0x00 0x23 0xC3 + int (4 bytes de comando)

0x01  0x16 0xD1 matr  Envia sinal de controle Int (4 bytes)
0x01  0x16 0xD2 matr  Envia sinal de Referência Float (4 bytes)
0x01  0x16 0xD3 matr  Envia Estado do Sistema (Ligado = 1 / Desligado = 0)    0x00 0x16 0xD3 + int (4 bytes de estado)
0x01  0x16 0xD4 matr  Modo de Controle da Temperatura de referência(Dashboard=0/Terminal=1)(1 byte)   0x00 0x16 0xD4 + int (4 bytes de modo de controle)
0x01  0x16 0xD5 matr  Envia Estado de Funcionamento (Funcionando = 1 / Parado = 0)    0x00 0x16 0xD5 + int (4 bytes de estado)
0x01  0x16 0xD6 matr  Envia valor do Temporizador (Inteiro)   0x00 0x16 0xD6 + int (4 bytes de estado)
*/

#define MESSAGE_MAX_SIZE        128
#define ESP_ADDRESS_DEVICE      0x01
#define ESP_REQUEST             0x23
#define ESP_SEND                0x16

#define TEMPERATURE_ERROR       0.5f
#define TEMPERATURE_MIN         0.0f
#define TEMPERATURE_MAX         70.0f

#define RETURN_MESSAGE_SIZE     4
#define ESP_TIMEOUT             3


static Byte message_buffer[MESSAGE_MAX_SIZE];
static float ultima_leitura_temperatura_interna = 0;
static float ulrima_leitura_temperatura_referencia = 0;

void esp32_control_open(){
    modbus_open();
}

int request_esp(void *return_message, Byte sub_code){

    modbus_read(&message_buffer[0], MESSAGE_MAX_SIZE);//Limpar leitura
    modbus_init(ESP_ADDRESS_DEVICE, ESP_REQUEST, sub_code);
    int error = 0;
    for(int i = 0; error <= 0 && i < ESP_TIMEOUT; i++){
        error = modbus_write(NULL, 0);
        if(error <= 0) continue;
        error = modbus_read(&message_buffer[0], MESSAGE_MAX_SIZE);
    }

    memcpy(return_message, &message_buffer[3], RETURN_MESSAGE_SIZE);

    return ESP_R_SUCCESS;
}

int send_esp(void *data, Byte data_size, void *return_message, Byte sub_code){

    modbus_read(&message_buffer[0], MESSAGE_MAX_SIZE);//Limpar leitura
    modbus_init(ESP_ADDRESS_DEVICE, ESP_SEND, sub_code);
    int error = modbus_write(data, data_size);
    if(error == -1) return ESP_R_FAIL;

    if(return_message != NULL){
        error = modbus_read(&message_buffer[0], MESSAGE_MAX_SIZE);
        if(error == -1) return ESP_R_FAIL;

        memcpy(return_message, &message_buffer[3], RETURN_MESSAGE_SIZE);
    }
    
    return ESP_R_SUCCESS;
}

int temperatura_interna(float *temperatura){
    
    int error = request_esp(temperatura, 0xC1);
    for(int i = 0; error <= 0 && i < ESP_TIMEOUT; i++){
        error = request_esp(temperatura, 0xC1);
    }

    if(*temperatura > TEMPERATURE_MAX || *temperatura <= TEMPERATURE_MIN){
        *temperatura = ultima_leitura_temperatura_interna;
    }else{
        ultima_leitura_temperatura_interna = *temperatura;
    }

    return error;
}

int temperatura_referencia(float *temperatura){
    
    int error = request_esp(temperatura, 0xC2);
    for(int i = 0; error <= 0 && i < ESP_TIMEOUT; i++){
        error = request_esp(temperatura, 0xC2);
    }

    if(*temperatura > TEMPERATURE_MAX || *temperatura <= TEMPERATURE_MIN){
        *temperatura = ulrima_leitura_temperatura_referencia;
    }else{
        ulrima_leitura_temperatura_referencia = *temperatura;
    }

    return error;
}

int comando_usuario(Byte *comando){
    Byte bytes[4];
    int error = request_esp(&bytes[0], 0xC3);
    *comando = bytes[0];
    return error;
}

int envia_temperatura_controle(int controle){
    return send_esp(&controle, sizeof(int), NULL, 0xD1);
}
int envia_temperatura_referencia(float temperatura){
    return send_esp(&temperatura, sizeof(float), NULL, 0xD2);
}

int liga_desliga(Byte sinal){
    Byte bytes[4];
    send_esp(&sinal, sizeof(Byte), bytes, 0xD3);
    for(int i = 0; bytes[0] != sinal && i < ESP_TIMEOUT; i++){
        send_esp(&sinal, sizeof(Byte), bytes, 0xD3);
    }
    if(bytes[0] != sinal) return ESP_R_FAIL;
    return ESP_R_SUCCESS;
}

int modo_controle(Byte modo){
    Byte bytes[4];
    send_esp(&modo, sizeof(Byte), bytes, 0xD4);
    for(int i = 0; bytes[0] != modo && i < ESP_TIMEOUT; i++){
        send_esp(&modo, sizeof(Byte), bytes, 0xD4);
    }
    if(bytes[0] != modo) return ESP_R_FAIL;
    return ESP_R_SUCCESS;
}

int estado_funcionamento(Byte estado){
    Byte bytes[4];
    send_esp(&estado, sizeof(Byte), bytes, 0xD5);
    for(int i = 0; bytes[0] != estado && i < ESP_TIMEOUT; i++){
        send_esp(&estado, sizeof(Byte), bytes, 0xD4);
    }
    if(bytes[0] != estado) return ESP_R_FAIL;
    return ESP_R_SUCCESS;
}

int envia_temporizador(Byte tempo){
    Byte bytes[4];
    send_esp(&tempo, sizeof(Byte), bytes, 0xD6);
    for(int i = 0; bytes[0] != tempo && i < ESP_TIMEOUT; i++){
        send_esp(&tempo, sizeof(Byte), bytes, 0xD4);
    }
    if(bytes[0] != tempo) return ESP_R_FAIL;
    return ESP_R_SUCCESS;
}

void esp_control_close(){
    modbus_close();
}