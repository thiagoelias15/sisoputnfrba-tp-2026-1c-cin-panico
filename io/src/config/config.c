#include "config.h"

t_io_config io_config;

void cargar_configuracion_io(char* path) {
    // Lo guardamos en config_base
    io_config.config_base = iniciar_config(path);

    // Lo usamos para extraer los valores
    io_config.ip_sched = config_get_string_value(io_config.config_base, "IP_SCHEDULER");
    io_config.puerto_sched = config_get_string_value(io_config.config_base, "PUERTO_SCHEDULER");
    io_config.id_modulo = config_get_string_value(io_config.config_base, "ID_MODULO");
}

void destruir_configuracion_io() {
    config_destroy(io_config.config_base);
}