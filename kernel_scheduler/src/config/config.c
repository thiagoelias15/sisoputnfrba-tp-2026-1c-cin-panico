#include "config.h"

// Inicializamos la struct global que creamos en config.h
t_kernel_config kernel_config;

void cargar_configuracion_kernel(char* path) {
    // Usamos la función iniciar_config que viene de utils/str/config/config.h
    kernel_config.config_base = iniciar_config(path);

    // Llenamos la struct con la informacion
    kernel_config.ip_memoria = config_get_string_value(kernel_config.config_base, "IP_MEMORIA");
    kernel_config.puerto_memoria = config_get_string_value(kernel_config.config_base, "PUERTO_MEMORIA");
    kernel_config.puerto_escucha = config_get_string_value(kernel_config.config_base, "PUERTO_ESCUCHA");
    kernel_config.id_modulo = config_get_string_value(kernel_config.config_base, "ID_MODULO");
    kernel_config.algoritmo_planificacion = config_get_string_value(kernel_config.config_base, "PLANIFICATION_ALGORITHM");
    kernel_config.quantum_rr = config_get_int_value(kernel_config.config_base, "RR_QUANTUM");
    kernel_config.algoritmos_colas = config_get_array_value(kernel_config.config_base, "QUEUES_ALGORITHMS");
    char* preempt = config_get_string_value(kernel_config.config_base, "QUEUE_PREEMPTION");
    kernel_config.suspension_timeout = config_get_int_value(kernel_config.config_base, "SUSPENSION_TIMEOUT");
    kernel_config.queue_preemption = (strcmp(preempt, "TRUE")== 0) ? 1 : 0;  
}

void destruir_configuracion_kernel() {
    config_destroy(kernel_config.config_base);//borramos todo para evitar memory leaks
}