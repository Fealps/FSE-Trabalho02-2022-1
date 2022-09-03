#ifndef ESP32_CONTROL_H_
#define ESP32_CONTROL_H_

/*  
Comando                                                 Código 
Liga o Forno                                            0x01 
Desliga o Forno                                         0x02  
Inicia aquecimento                                      0x03    
Cancela processo                                        0x04    
Tempo + : adiciona 1 minuto ao timer                    0x05    
Tempo - : adiciona 1 minuto ao timer                    0x06
Menu : aciona o modo de alimentos pré-programados       0x07
*/
#include "modbus.h"

#define ESP_ON                      0x01
#define ESP_OFF                     0x00

#define ESP_MODO_DASHBOARD          0x00
#define ESP_MODO_TERMINAL           0x01

#define ESP_R_SUCCESS               0
#define ESP_R_FAIL                  -1

#define ESP_USER_LIGA               0x01
#define ESP_USER_DESLIGA            0x02
#define ESP_USER_AQUECE             0x03
#define ESP_USER_CANCELA            0x04
#define ESP_USER_ADICIONA           0x05
#define ESP_USER_REMOVE             0x06
#define ESP_USER_MENU               0x07

int temperatura_interna(float *temperatura);
int temperatura_referencia(float *temperature);
int comando_usuario(Byte *comando);
int envia_temperatura_controle(int controle);
int envia_temperatura_referencia(float temperatura);
int liga_desliga(Byte sinal);
int modo_controle(Byte modo);
int estado_funcionamento(Byte estado);
int envia_temporizador(Byte tempo);


#endif /* ESP32_CONTROL_H_ */