#include "config.h"

t_memoria_config memoria_config;

void cargar_configuracion_memoria(char* path){
    memoria_config.config_base = iniciar_config(path);
    memoria_config.id_recibida = config_get_string_value(memoria_config.config_base, "ID_RECIBIDA");
    memoria_config.puerto = config_get_string_value(memoria_config.config_base, "PUERTO_ESCUCHA");
    memoria_config.memoria_operando = config_get_int_value(memoria_config.config_base, "MEMORIA_OPERANDO");
    memoria_config.segment_max_size = config_get_int_value(memoria_config.config_base, "SEGMENT_MAX_SIZE");
    memoria_config.allocation_strategy = config_get_string_value(memoria_config.config_base, "ALLOCATION_STRATEGY");
    memoria_config.instruction_delay = config_get_int_value(memoria_config.config_base, "INSTRUCTION_DELAY");
    memoria_config.compaction_delay = config_get_int_value(memoria_config.config_base, "COMPACTION_DELAY");
    memoria_config.scripts_basepath = config_get_string_value(memoria_config.config_base, "SCRIPTS_BASEPATH");
}

void destruir_configuracion_memoria() {
    config_destroy(memoria_config.config_base);
}