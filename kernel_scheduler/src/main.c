#include <utils/utils.h>
int main(int argc,char* argv[]){
    //verifiacion del archivo config
    if(agrc<2){
        printf("Error: Mal ejecutado");
        return EXIT_FAILURE; 
    }
    //cargamos config y extraemos la informacion necesaria
t_config* config = iniciar_config(argv[1]);
char* ip_mem = config_get_string_value(config, "IP_MEMORIA");
char* port_mem = config_get_string_value(config, "PUERTO_MEMORIA");
char* puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");

//iniciamos logger
t_log* logger = log_create("Scheduler.log","SCHEDULER",1,LOG_LEVEL_INFO);
log_info(logger,"Iniciando Scheduler");

// 1° Scheduler actua como cliente
int fd_memoria = crear_conexion(ip_mem,port_mem);
if(fd_memoria !=-1){
    enviar_mensaje("HANDSHAKE_SCHEDULER",fd_memoria);
    log_info(logger, "Scheduler conectado a la memoria");
}else{
    log_error(logger,"Fallo la conexion a la memoria");
    return EXIT_FAILURE;// si la memoria no esta prendida error
}

//2° Scheduler como servidor
int fd_escucha = iniciar_servidor(puerto_escucha);
if(fd_escucha == -1){
    log_error(logger, "Fallo al iniciar el servidor del Scheduler");
        return EXIT_FAILURE;
    }

    log_info(logger, "Servidor del Scheduler encendido");

//guardamos los sockets de los clientes
int clientes_sched = 0;
int fd_cpu =-1, fd_io =-1;
//while del scheduler para esperar a cpu e io
while(clientes_sched<2){
    int socket_cliente = esperar_cliente(fd_escucha);
    int cod_op = recibir_operacion(socket_cliente);
    if(cod_op==MENSAJE){
        char* mensaje = recibir_mensaje(socket_cliente);
        if(strcmp(mensaje,"HANDSHAKE_CPU")==0){
            og_info(logger, "CPU Conectada al Scheduler");
                fd_cpu = socket_cliente;
                clientes_sched++;
            } 
            else if (strcmp(mensaje, "HANDSHAKE_IO") == 0) {
                log_info(logger, "IO Conectada al Scheduler");
                fd_io = socket_cliente;
                clientes_sched++;
            } 
            else {
                log_warning(logger, "Modulo desconocido en el Scheduler: %s", mensaje);
                close(socket_cliente);
            }
            
            // Liberar la memoria del texto que recibimos (el buffer del tren)
            free(mensaje);

        } else {
            log_warning(logger, "Operacion desconocida. Se esperaba un MENSAJE.");
            close(socket_cliente);
        }
    }

    // Si salio del while, es porque ya se conectaron los 2 modulos
    log_info(logger, "Todos los clientes conectados. Scheduler listo para planificar.");

    // (Acá irá toda la lógica de FIFO, Round Robin o VRR para el checkpoint 2 creo)

    // Limpieza final antes de cerrar el programa
    close(fd_cpu);
    close(fd_io);
    close(fd_memoria);
    close(fd_escucha);
    config_destroy(config);
    log_destroy(logger);

    return 0;
}
        
    





















}

