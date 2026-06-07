#include "config.h"

t_swap_config swap_config;

void cargar_configuracion_swap(char* path) {
    swap_config.config_base = config_create(path);

    // Extraemos los valores del archivo
    swap_config.ip_memoria = config_get_string_value(swap_config.config_base, "IP_MEMORIA");
    swap_config.puerto_memoria = config_get_string_value(swap_config.config_base, "PUERTO_MEMORIA");
    swap_config.id_modulo = config_get_string_value(swap_config.config_base, "ID_MODULO");
}

void destruir_configuracion_swap() {
    config_destroy(swap_config.config_base);
}