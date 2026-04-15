#include <utils/utils.h>
#include <stdio.h>

int main(int argc,char* argv[]) // La variable "argc" almacena la cantidad de palabras en la terminal, mientras que "argv[numero]" almacena la palabra en dicha posición.
{
    // Retorna ERROR si la cantidad de palabras en la terminal es menor a tres
    if(argc < 2) {

        printf("[ERROR] Debe utilizar el siguiente formato: ./bin/cpu ./config\n");
        return EXIT_FAILURE;
    }

    t_config* config = iniciar_config(argv[1]);

    // Se carga el "config" con la información del parámetro argv[1].
    char* ip_mem = config_get_string_value(config, "IP_MEMORIA");
    char* port_mem = config_get_string_value(config, "PUERTO_MEMORIA");
    char* ip_sched = config_get_string_value(config, "IP_SCHEDULER");
    char* port_sched = config_get_string_value(config, "PUERTO_SCHEDULER"); // Se obtiene información del ".config" para la conexión con el resto de módulos
    char* mi_id = config_get_string_value(config,"ID_MODULO");

    t_log* logger = log_create("cpu.log", "CPU", 1, LOG_LEVEL_INFO);
    log_info(logger, "Iniciando CPU");

    // ------------------------------ CONEXIONES ------------------------------ //

    // Utilizamos las variables obtenidas a partir del config (ip_mem, port_mem, etc.)

    int fd_memoria = crear_conexion(ip_mem,port_mem);
    if(fd_memoria !=-1) {

       enviar_mensaje(mi_id, MENSAJE, fd_memoria);
       log_info(logger,"Handshake enviado con ID: %s",mi_id);  

    }

    int fd_scheduler = crear_conexion(ip_sched,port_sched);
    if(fd_scheduler != -1) {

       enviar_mensaje(mi_id, MENSAJE, fd_scheduler);
       log_info(logger,"Handshake enviado con ID: %s",mi_id);

    }

    if(fd_memoria != -1 && fd_scheduler != -1) {

        log_info(logger,"CPU conectada a todos los modulos correctamente");
        log_info(logger, "Enviando saludo a la Memory Stick a traves de la Memoria...");
        enviar_mensaje("¡Hola Stick! Soy la CPU mandando un saludo.", HANDSHAKE_CPU_A_MS, fd_memoria);

        // Esperamos la respuesta del Memory Stick, el cual llega a través de la memoria.
        int cod_op = recibir_operacion(fd_memoria);
        
        if(cod_op == HANDSHAKE_CPU_A_MS) {

            char* respuesta_ms = recibir_mensaje(fd_memoria);
            log_info(logger, "La Stick me respondio correctamente: %s", respuesta_ms);
            free(respuesta_ms);

        }
        else {
            
            log_error(logger, "Fallo el handshake con la Stick. Codigo recibido: %d", cod_op);
        }
    }

    // ------------------------------ LIMPIEZA DE LA MEMORIA ------------------------------ //

    close(fd_memoria); 
    close(fd_scheduler);
    config_destroy(config); // Liberamos la memoria del config
    log_destroy(logger);
    
    return 0; 
}
