#ifndef SWAP_CONFIG_H_
#define SWAP_CONFIG_H_

#include <commons/config.h>
#include <utils/utils.h>


typedef struct {
    char* ip_memoria;
    char* puerto_memoria;
    char* id_modulo;
    t_config* config_base;
} t_swap_config;


extern t_swap_config swap_config;

void cargar_configuracion_swap(char* path);
void destruir_configuracion_swap();

#endif