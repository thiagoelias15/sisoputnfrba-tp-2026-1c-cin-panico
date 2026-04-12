#include <utils/utils.h>

int main(int argc, char* argv[]) {

    if(argc<2){
        printf("Error: Mal ejecutado");
        return EXIT_FAILURE;
    }

    // 1 - Inicializacion de herramientas de las Commons
    t_config* config = iniciar_config(argv[1]);

    // 2 - Extraccion de parametros del config
    char* ip_sched = config_get_string_value(config, "IP_SCHEDULER");
    char* puerto_sched = config_get_string_value(config, "PUERTO_SCHEDULER");
    char* mi_id = config_get_string_value(config,"ID_MODULO");
    // 3 - Iniciamos Logger
    t_log* logger = log_create("io.log","IO",1,LOG_LEVEL_INFO);
    log_info(logger, "Iniciando modulo de I/O ");

    // 4 - Conexiones
    int fd_scheduler = crear_conexion(ip_sched,puerto_sched);
    if(fd_scheduler != -1){
        enviar_mensaje(mi_id, MENSAJE,fd_scheduler);
        log_info(logger, "I/O conectada al Scheduler correctamente."); 
    }
    
    // 5 - Limpieza
    close(fd_scheduler);
    config_destroy(config);
    log_destroy(logger);

    return 0;
}
