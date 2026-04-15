#include <utils/utils.h>

int main(int argc,char* argv[]) {
    
    // Se verifica la información en el config

    if(argc<2) {

        printf("[ERROR] Mal ejecutado");
        return EXIT_FAILURE; 
    }

    // Se carga el config, extrayendo la información necesaria para la conexión de módulos.
    t_config* config = iniciar_config(argv[1]);

    char* ip_mem = config_get_string_value(config, "IP_MEMORIA");
    char* port_mem = config_get_string_value(config, "PUERTO_MEMORIA");
    char* puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
    char* mi_id = config_get_string_value(config, "ID_MODULO");

    // Se inicia el Logger
    t_log* logger = log_create("Scheduler.log","SCHEDULER",1,LOG_LEVEL_INFO);
    log_info(logger,"Iniciando Scheduler");

    // ------------------------------ SCHEDULER COMO CLIENTE ------------------------------ //

    int fd_memoria = crear_conexion(ip_mem,port_mem);

    if(fd_memoria !=-1) {

        enviar_mensaje(mi_id, MENSAJE, fd_memoria);
        log_info(logger, "Scheduler conectado a la memoria con ID: %s",mi_id);

    }
    else {

        log_error(logger,"Fallo en la conexión a la memoria: Kernel Memory no se encuentra activo");
        return EXIT_FAILURE; // Retorna error si la memoria no esta prendida.
    }

    // ------------------------------ SCHEDULER COMO SERVIDOR ------------------------------ //

    int fd_escucha = iniciar_servidor(puerto_escucha);

    if(fd_escucha == -1) {

        log_error(logger, "Fallo al iniciar el servidor del Scheduler");
        return EXIT_FAILURE;
    }

    log_info(logger, "Servidor del Scheduler encendido");

    // Se guardan los sockets de los clientes
    int clientes_sched = 0;
    int fd_cpu =-1, fd_io =-1;

    // Estructura "while" como espera al CPU y al I/O
    while(clientes_sched<2) {

        int socket_cliente = esperar_cliente(fd_escucha);
        int cod_op = recibir_operacion(socket_cliente);

        if(cod_op == MENSAJE) {

            char* id_recibida = recibir_mensaje(socket_cliente);

            if(strcmp(id_recibida, "CPU") == 0) {

                log_info(logger, "CPU Conectada al Scheduler");
                fd_cpu = socket_cliente;
                clientes_sched++;
            } 
            else if(strcmp(id_recibida, "IO") == 0) {

                log_info(logger, "Interfaz de IO Conectada al Scheduler: %s",id_recibida);
                fd_io = socket_cliente;
                clientes_sched++;
            } 
            else {

                log_warning(logger, "Modulo desconocido en el Scheduler: %s", id_recibida);
                close(socket_cliente);
            }

            free(id_recibida); // Liberar la memoria del ID que recibimos

        }
        else {
            
            log_warning(logger, "Operacion desconocida. Se esperaba un MENSAJE.");
            close(socket_cliente);
        }
    }

    // Si salio del while, es porque ya se conectaron los dos módulos.
    log_info(logger, "Todos los clientes conectados. Scheduler listo para planificar.");

    // [IMPORTANTE] En este archivo se debe armar la lógica de planificación en el siguiente Checkpoint (sea FIFO, RR o VRR)
    
    // ------------------------------ LIMPIEZA DE LA MEMORIA ------------------------------ //

    close(fd_cpu);
    close(fd_io);
    close(fd_memoria);
    close(fd_escucha);
    config_destroy(config);
    log_destroy(logger);

    return 0;
}
        
    























