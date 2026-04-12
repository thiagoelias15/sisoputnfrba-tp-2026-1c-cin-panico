#include <utils/utils.h>
#include <stdio.h>
int main(int argc,char* argv[])/*
argc guarda la cantidad de palabras de la terminal 
y argv[numero]guarda la palabra en esa posicion exacta*/
{
    if(argc<2){ /*si la cantidad de palabras de la termina es menor a 3 error */
        printf("Error, se debe usar: ./bin/cpu ./config\n");
        return EXIT_FAILURE;
    }

    t_config* config = iniciar_config(argv[1]);

    //cargamos el archivo config usando el parametro argv[1]
    char* ip_mem = config_get_string_value(config, "IP_MEMORIA");
    char* port_mem = config_get_string_value(config, "PUERTO_MEMORIA");
    char* ip_sched = config_get_string_value(config, "IP_SCHEDULER");
    char* port_sched = config_get_string_value(config, "PUERTO_SCHEDULER"); /*aca estamos sacando la informacion,del .config, que necesita la cpu para conectarse a los demas modulos*/
    char* mi_id = config_get_string_value(config,"ID_MODULO");
    t_log* logger = log_create("cpu.log", "CPU", 1, LOG_LEVEL_INFO);
    log_info(logger, "Iniciando CPU" );
    
//conexiones
    //Ahora usamos las variables que sacamos del config(ip_mem,port_mem,etc)
    int fd_memoria = crear_conexion(ip_mem,port_mem);
    if(fd_memoria !=-1){
       enviar_mensaje(mi_id, MENSAJE, fd_memoria);
       log_info(logger,"Handshake enviado con ID: %s",mi_id);    
    }
    int fd_scheduler = crear_conexion(ip_sched,port_sched);
    if(fd_scheduler != -1){
       enviar_mensaje(mi_id, MENSAJE, fd_scheduler);
       log_info(logger,"Handshake enviado con ID: %s",mi_id);
    }

    if(fd_memoria != -1 && fd_scheduler != -1){
        log_info(logger,"CPU conectada a todos los modulos correctamente");
        log_info(logger, "Enviando saludo a la Memory Stick a traves de la Memoria...");
        enviar_mensaje("¡Hola Stick! Soy la CPU mandando un saludo.", HANDSHAKE_CPU_A_MS, fd_memoria);

        // Nos quedamos esperando que vuelva la respuesta de la Stick desde la Memoria
        int cod_op = recibir_operacion(fd_memoria);
        
        if(cod_op == HANDSHAKE_CPU_A_MS) {
            char* respuesta_ms = recibir_mensaje(fd_memoria);
            log_info(logger, "La Stick me respondio correctamente: %s", respuesta_ms);
            free(respuesta_ms);
        } else {
            log_error(logger, "Fallo el handshake con la Stick. Codigo recibido: %d", cod_op);
        }
    }
 //limpieza
    close(fd_memoria); close(fd_scheduler);
    config_destroy(config); //liberamos la memoria del config
    log_destroy(logger);

    return 0; 
}
