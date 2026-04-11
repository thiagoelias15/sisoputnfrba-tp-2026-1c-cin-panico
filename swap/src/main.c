#include <utils/utils.h>

int main(int argc, char* argv[]) {
    
    if(argc<2){
        printf("Error: Mal ejecutado");
        return EXIT_FAILURE;
    }

    // 1 - Inicializacion de herramientas de las Commons
    t_config* config = iniciar_config(argv[1]);
    
    // 2 - Extraccion de parametros del config
    char* ip_mem = config_get_string_value(config, "IP_MEMORIA");
    char* port_mem = config_get_string_value(config, "PUERTO_MEMORIA");
    
    // 3 - Iniciamos logger
    t_log* logger = log_create("swap.log","SWAP",1,LOG_LEVEL_INFO);
    log_info(logger, "Iniciando modulo de SWAP  ");

    // 4 - Conexiones
    int fd_memoria = crear_conexion(ip_mem, port_mem);
    if(fd_memoria != -1) {
        enviar_mensaje("HANDSHAKE_SWAP", fd_memoria);
        log_info(logger, "SWAP conectada al Kernel Memory.");
    }

    // 5 - Limpieza
    close(fd_memoria);
    config_destroy(config);
    log_destroy(logger);
    
    return 0;
}

