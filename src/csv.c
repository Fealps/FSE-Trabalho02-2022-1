#include "csv.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>


#define LOG_FILE_PATH "log/log.csv"
#define CURVE_FILE_PATH "curva_predefinida.csv"
#define MAXSIZE_BUFFER 50
#define SEPARATOR ",;"


void csv_create_log(void){

    FILE *file = fopen(LOG_FILE_PATH, "r");

    if(file == NULL){
        FILE *file = fopen(LOG_FILE_PATH, "w");
        fprintf(file, "%s;%s;", "Data", "Hora");
        fprintf(file, "%s;%s;%s;", "Temperatura interna (C)", "Temperatura externa (C)", "Temperatura de referencia (C)");
        fprintf(file, "%s;%s\n", "Potencia do Resistor (%)", "Potencia da Ventoinha (%)");
        fclose(file);
    }else{
        fclose(file);
    }
}

void csv_append_log(struct csv_log_data log){
    
    FILE *file = fopen(LOG_FILE_PATH, "a");

    time_t rawtime;
    struct tm *info;
    time(&rawtime);
    info = localtime(&rawtime);

    fprintf(file, "%02d/%02d/%04d;", info->tm_mday, info->tm_mon, info->tm_year%100 + 2000);
    fprintf(file, "%02d:%02d:%02d;", info->tm_hour, info->tm_min, info->tm_sec);
    fprintf(file, "%02.1lf;%02.1lf;%02.1lf;", log.var_temperatura_interna, log.var_temperatura_externa, log.var_temperatura_referencia);
    fprintf(file, "%02.1lf;%02.1lf\n",log.resistor_power , log.fan_speed);

    fclose(file);
}

long csv_read_csv_curve(float *temperature_, long* time_){

    FILE *file = fopen(CURVE_FILE_PATH, "r");
    if(file == NULL) return -1;

    char *row = malloc(MAXSIZE_BUFFER);
    char *token;

    long index = 0;
    while(feof(file) == 0){
        fgets(row, MAXSIZE_BUFFER, file);
        token = strtok(row, SEPARATOR);
        time_[index] = atol(token);
        token = strtok(NULL, SEPARATOR);
        temperature_[index] = atof(token);
        index++;
    }

    fclose(file);
    return index;
}