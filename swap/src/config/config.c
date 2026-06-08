#include "config.h"

t_swap_config swap_config;

void cargar_configuracion_swap(char* path) {
    swap_config.config_base = config_create(path);

    // Extraemos los valores del archivo
    swap_config.ip_memoria = config_get_string_value(swap_config.config_base, "IP_MEMORIA");
    swap_config.puerto_memoria = config_get_string_value(swap_config.config_base, "PUERTO_MEMORIA");
    swap_config.id_modulo = config_get_string_value(swap_config.config_base, "ID_MODULO");
    swap_config.block_size = config_get_string_value(swap_config.config_base, "BLOCK_SIZE");
    swap_config.swap_file_size = config_get_string_value(swap_config.config_base, "SWAP_FILE_SIZE");
    swap_config.swap_file_path = config_get_string_value(swap_config.config_base, "SWAP_FILE_PATH");
}

void destruir_configuracion_swap() {
    config_destroy(swap_config.config_base);
}