#ifndef MS_CONFIG_H_
#define MS_CONFIG_H_

#include <commons/config.h>
#include <utils/utils.h>

typedef struct {
    char* ip_memoria;
    char* puerto_memoria;
    char* puerto_escucha;
    char* ip_escucha;
    int memory_delay;
    char* id_modulo;
    t_config* config_base;
} t_ms_config;

// Declaramos la variable global para que sea accesible desde el main
extern t_ms_config ms_config;

void cargar_configuracion_ms(char* path);
void destruir_configuracion_ms();

#endif