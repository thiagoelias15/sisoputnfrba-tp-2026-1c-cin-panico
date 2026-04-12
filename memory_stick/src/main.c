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
    char* puerto_mem = config_get_string_value(config, "PUERTO_MEMORIA");
    char*mi_id = config_get_string_value(config,"ID_MODULO");
    // 3 - Iniciamos Logger
    t_log* logger = log_create("ms.log","MEMORY_STICK",1,LOG_LEVEL_INFO);
    log_info(logger, "Iniciando modulo Memory Stick");

    // 4 - Conexiones
    int fd_memoria = crear_conexion(ip_mem,puerto_mem);
    if(fd_memoria != -1){
      enviar_mensaje(mi_id, MENSAJE, fd_memoria);
        log_info(logger,"Memory Stick conectado a Kernel Memory");
    log_info(logger, "Esperando saludo de la CPU a traves de la Memoria...");
        
        // Nos quedamos esperando que llegue algo por el socket de memoria
        int cod_op = recibir_operacion(fd_memoria);

        if(cod_op == HANDSHAKE_CPU_A_MS) {
            char* mensaje_cpu = recibir_mensaje(fd_memoria);
            log_info(logger, "Llego un saludo de la CPU: %s", mensaje_cpu);
            free(mensaje_cpu);

            // Respondemos el saludo a la CPU (usando el mismo código de operación)
            log_info(logger, "Enviando respuesta a la CPU...");
            enviar_mensaje("¡Hola CPU! Handshake recibido por la Stick.", HANDSHAKE_CPU_A_MS, fd_memoria);
        } else {
            log_error(logger, "Se esperaba un saludo de la CPU pero llego otra cosa.");
        }
    } else {
        log_error(logger, "No se pudo conectar con la Memoria.");
        config_destroy(config);
        log_destroy(logger);
        return EXIT_FAILURE;
    }

    // 5 - Limpieza
    close(fd_memoria);
    config_destroy(config);
    log_destroy(logger);
     return 0;
}
