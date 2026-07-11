#ifndef MEMORIA_CONFIG_H_
#define MEMORIA_CONFIG_H_

#include <commons/config.h>
#include <utils/utils.h>

typedef struct{
    char* id_recibida;
    char* puerto;
    int memoria_operando;
    int segment_max_size;
    char* allocation_strategy;
    int instruction_delay;
    int compaction_delay;
    char* scripts_basepath;
    char* ip_swap;
    char* puerto_swap;
    t_config* config_base;
} t_memoria_config;

extern t_memoria_config memoria_config;

void cargar_configuracion_memoria(char* path);
void destruir_configuracion_memoria();

#endif