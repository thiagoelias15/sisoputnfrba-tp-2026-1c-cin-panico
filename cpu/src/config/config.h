#ifndef CPU_CONFIG_H_
#define CPU_CONFIG_H_

#include <commons/config.h>
#include <utils/utils.h>


typedef struct {
    char* ip_memoria;
    char* puerto_memoria;
    char* ip_sched;
    char* puerto_sched;
    char* id_modulo;
    t_config* config_base;
} t_cpu_config;


extern t_cpu_config cpu_config;

void cargar_configuracion_cpu(char* path);
void destruir_configuracion_cpu();

#endif