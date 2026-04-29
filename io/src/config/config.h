#ifndef IO_CONFIG_H_
#define IO_CONFIG_H_

#include <commons/config.h>
#include <utils/utils.h>

typedef struct {
    char* ip_sched;
    char* puerto_sched;
    char* id_modulo;
    t_config* config_base;
} t_io_config;


extern t_io_config io_config;

void cargar_configuracion_io(char* path);
void destruir_configuracion_io();

#endif