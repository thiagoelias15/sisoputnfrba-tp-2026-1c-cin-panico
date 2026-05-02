#include "config.h"


t_cpu_config cpu_config;

void cargar_configuracion_cpu(char* path) { // Se carga la información del parámetro argv[1] de main.c

    cpu_config.config_base = iniciar_config(path);
    cpu_config.ip_memoria = config_get_string_value(cpu_config.config_base, "IP_MEMORIA");
    cpu_config.puerto_memoria = config_get_string_value(cpu_config.config_base, "PUERTO_MEMORIA");
    cpu_config.ip_sched = config_get_string_value(cpu_config.config_base, "IP_SCHEDULER");
    cpu_config.puerto_sched = config_get_string_value(cpu_config.config_base, "PUERTO_SCHEDULER");
    cpu_config.id_modulo = config_get_string_value(cpu_config.config_base, "ID_MODULO");
}

void destruir_configuracion_cpu() {
    config_destroy(cpu_config.config_base);
}