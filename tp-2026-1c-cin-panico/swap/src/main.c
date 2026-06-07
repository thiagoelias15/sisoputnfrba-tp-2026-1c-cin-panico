#include "main.h"
#include <stdio.h>
#include <stdlib.h>

t_log* logger;

int main(int argc, char* argv[]) {
    
    if(argc<2) {
        printf("[ERROR] Mal ejecutado");
        return EXIT_FAILURE;
    }

    cargar_configuracion_swap(argv[1]);

    // Se inicia el Logger
    logger = log_create("swap.log","SWAP",1,LOG_LEVEL_INFO);
    log_info(logger, "Iniciando modulo de SWAP  ");

    // ------------------------------ CONEXIONES ------------------------------ //

    int fd_memoria = crear_conexion(swap_config.ip_memoria, swap_config.puerto_memoria);

    if(fd_memoria != -1) {

        enviar_mensaje(swap_config.id_modulo, MENSAJE, fd_memoria);
        log_info(logger, "SWAP conectada al Kernel Memory.");
    }else {
        
        log_error(logger, "No se pudo conectar al Kernel Memory");
        destruir_configuracion_swap();
        log_destroy(logger);
        return EXIT_FAILURE;
    }

    // ------------------------------ LIMPIEZA DE LA MEMORIA ------------------------------ //

    close(fd_memoria);
    destruir_configuracion_swap();
    log_destroy(logger);
    return 0;
}

