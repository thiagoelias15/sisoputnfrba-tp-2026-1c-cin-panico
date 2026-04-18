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
    char* puerto_mem = config_string_value(config, "PUERTO_MEMORIA");
    
    // 3 - Iniciamos Logger
    t_log* logger = log_create("ms.log","MEMORY_STICK",1,LOG_LEVEL_INFO);
    log_info(logger, "Iniciando modulo Memory Stick");

    // 4 - Conexiones
    int fd_memoria = crear_conexion(ip_mem,puerto_mem);
    if(fd_memoria != -1){
        enviar_mensaje("HANDSHAKE_MS", fd_memoria);
        log_info(logger,"Memory Stick conectado a Kernel Memory");
    }

    // 5 - Limpieza
    close(fd_memoria);
    config_destroy(config);
    log_destroy(logger);
    return 0;
}
