#include "config.h"

t_memoria_config memoria_config;

void cargar_configuracion_memoria(char* path){
    memoria_config.config_base = iniciar_config(path);
    memoria_config.id_recibida = config_get_string_value(memoria_config.config_base, "ID_RECIBIDA");
    memoria_config.puerto = config_get_string_value(memoria_config.config_base, "PUERTO");
    memoria_config.memoria_operando = config_get_int_value(memoria_config.config_base, "MEMORIA_OPERANDO");
}

void destruir_configuracion_memoria() {
    config_destroy(memoria_config.config_base);
}