#include <utils/utils.h>

int main(int argc, char* argv[]) {
    
    if(argc<2) {

        printf("[ERROR] Mal ejecutado");
        return EXIT_FAILURE;
    }

    // Se inicializan las herramientas de las commons y se extraen los parámetros correspondientes del config
    t_config* config = iniciar_config(argv[1]);
    
    char* ip_mem = config_get_string_value(config, "IP_MEMORIA");
    char* port_mem = config_get_string_value(config, "PUERTO_MEMORIA");
    char* mi_id = config_get_string_value(config,"ID_MODULO");

    // Se inicia el Logger
    t_log* logger = log_create("swap.log","SWAP",1,LOG_LEVEL_INFO);
    log_info(logger, "Iniciando modulo de SWAP  ");

    // ------------------------------ CONEXIONES ------------------------------ //

    int fd_memoria = crear_conexion(ip_mem, port_mem);

    if(fd_memoria != -1) {

        enviar_mensaje(mi_id, MENSAJE, fd_memoria);
        log_info(logger, "SWAP conectada al Kernel Memory.");
    }

    // ------------------------------ LIMPIEZA DE LA MEMORIA ------------------------------ //

    close(fd_memoria);
    config_destroy(config);
    log_destroy(logger);
    
    return 0;
}

