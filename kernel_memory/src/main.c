#include <utils/utils.h>

int main (int argc, char*argv[]){
    t_log* logger = log_create("memoria.log","MEMORIA",1,LOG_LEVEL_INFO);
    if(argc < 2){
        printf("Error: Mal ejecutado");
        return EXIT_FAILURE;
    }
 
    // cargamos el config para sacar la informacion
    t_config* config = iniciar_config(argv[1]);
    char* puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
    if(puerto == NULL){
        log_error(logger,"No se encontro el PUERTO_ESCUCHA en el config");
        return EXIT_FAILURE;
    }
    
    log_info(logger, "Iniciando Servidor de Memoria en el puerto %s", puerto);

    // Abrimos el puerto para empezar a escuchar
    int fd_escucha = iniciar_servidor(puerto);
    if(fd_escucha == -1){
        log_error(logger,"Fallo al iniciar el servidor de memoria");
        return EXIT_FAILURE;
    }
    log_info(logger,"Servidor de memoria encendida. Esperando a los modulos");

    // ciclo while para esperar a los clientes 
    int clientes_conectados = 0;
    int fd_cpu = -1, fd_ms = -1, fd_scheduler = -1, fd_swap = -1; 
    
    while (clientes_conectados < 4){
        int socket_cliente = esperar_cliente(fd_escucha);
        int cod_op = recibir_operacion(socket_cliente);
        
        if(cod_op == MENSAJE){ 
            // 1. Recibimos la ID 
            char* id_recibida = recibir_mensaje(socket_cliente);
            
            log_info(logger, "Modulo conectado: %s", id_recibida);

            if(strcmp(id_recibida, "CPU") == 0) {
                fd_cpu = socket_cliente;
                clientes_conectados++;
            }
            else if(strcmp(id_recibida, "MEMORY_STICK") == 0) {
                fd_ms = socket_cliente;
                clientes_conectados++;
            }
            else if(strcmp(id_recibida, "SCHEDULER") == 0) {
                fd_scheduler = socket_cliente;
                clientes_conectados++;
            }
            else if(strcmp(id_recibida, "SWAP") == 0) {
                fd_swap = socket_cliente;
                clientes_conectados++;
            }
            else {
                log_warning(logger, "ID desconocida: %s", id_recibida);
                close(socket_cliente);
            }
            free(id_recibida); 
        } else {
            log_warning(logger, "Operacion desconocida. Se espera un MENSAJE");
            close(socket_cliente); 
        }
    }

    log_info(logger,"Todos los clientes conectados. Memoria lista para operar");

    int memoria_operando = 1;
    while(memoria_operando) {
        int cod_op_cpu = recibir_operacion(fd_cpu);

        if (cod_op_cpu == -1) {
            log_error(logger, "La CPU se desconecto abruptamente.");
            memoria_operando = 0; 
        } 
        else if (cod_op_cpu == HANDSHAKE_CPU_A_MS) {
            char* saludo_cpu = recibir_mensaje(fd_cpu);
            log_info(logger, "CPU manda saludo: %s", saludo_cpu);

            enviar_mensaje(saludo_cpu, HANDSHAKE_CPU_A_MS, fd_ms);
            free(saludo_cpu);

            int cod_op_ms = recibir_operacion(fd_ms);
            if (cod_op_ms == HANDSHAKE_CPU_A_MS) {
                char* respuesta_ms = recibir_mensaje(fd_ms);
                log_info(logger, "Stick responde: %s", respuesta_ms);
                enviar_mensaje(respuesta_ms, HANDSHAKE_CPU_A_MS, fd_cpu);
                free(respuesta_ms);
            }
        }
    }

    // Limpieza final
    close(fd_cpu);
    close(fd_ms);
    close(fd_scheduler);
    close(fd_swap);
    close(fd_escucha);
    config_destroy(config);
    log_destroy(logger);
    return 0;
}






