#include "config.h"

t_ms_config ms_config;

void cargar_configuracion_ms(char* path) {
    ms_config.config_base = config_create(path);

    ms_config.ip_memoria = config_get_string_value(ms_config.config_base, "IP_MEMORIA");
    ms_config.puerto_memoria = config_get_string_value(ms_config.config_base, "PUERTO_MEMORIA");
    ms_config.id_modulo = config_get_string_value(ms_config.config_base, "ID_MODULO");
}

void destruir_configuracion_ms() {
    config_destroy(ms_config.config_base);
}