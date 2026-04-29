#ifndef SCHEDULER_CONFIG_H_
#define SCHEDULER_CONFIG_H_

#include <commons/config.h>
#include <utils/utils.h> 

// Creamos la struct con toda la info que se va a necesitar sacar del config
typedef struct {
    char* ip_memoria;
    char* puerto_memoria;
    char* puerto_escucha;
    char* id_modulo;
    char* algoritmo_planificacion;
    int quantum_rr;
    t_config* config_base; 
} t_kernel_config;


extern t_kernel_config kernel_config;

// Firmas de las funciones que cargan y destruyen la info del config
void cargar_configuracion_kernel(char* path);
void destruir_configuracion_kernel();

#endif