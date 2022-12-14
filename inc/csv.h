#ifndef CSV_H_
#define CSV_H_


struct csv_log_data{
    double var_temperatura_interna;
    double var_temperatura_externa;
    double var_temperatura_referencia;
    double fan_speed;
    double resistor_power;
};


void csv_create_log(void);
void csv_append_log(struct csv_log_data log);
long csv_read_csv_curve(float *temperature_, long* time_);


#endif /* CSV_H_ */