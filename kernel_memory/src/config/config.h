#ifndef MEMORIA_CONFIG_H_
#define MEMORIA_CONFIG_H_

#include <commons/config.h>
#include <utils/utils.h>

typedef struct{
    char* id_recibida;
    char* puerto;
    int memoria_operando;
    t_config* config_base;
} t_memoria_config;

extern t_memoria_config memoria_config;

void cargar_configuracion_memoria(char* path);
void destruir_configuracion_memoria();

#endif