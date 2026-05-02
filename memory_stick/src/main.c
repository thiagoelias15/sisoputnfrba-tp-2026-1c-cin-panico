#include "main.h"
#include <stdio.h>
#include <stdlib.h>

t_log* logger;

int main(int argc, char* argv[]) {

    if(argc<2) {

        printf("[ERROR] Mal ejecutado");
        return EXIT_FAILURE;
    }
    
    // Se inicializan las herramientas de las commons y se extraen los parámetros correspondientes del config

    cargar_configuracion_ms(argv[1]);
    
    // Se inicia el Logger
    logger = log_create("ms.log","MEMORY_STICK",1,LOG_LEVEL_INFO);
    log_info(logger, "Iniciando modulo Memory Stick");

    // ------------------------------ CONEXIONES ------------------------------ //

    int fd_memoria = crear_conexion(ms_config.ip_memoria, ms_config.puerto_memoria);
    
    if(fd_memoria != -1) {
        enviar_mensaje(ms_config.id_modulo, MENSAJE, fd_memoria);
        log_info(logger, "Memory Stick conectado a Kernel Memory");
        log_info(logger, "Esperando saludo de la CPU a traves de la Memoria...");
        
        // Se espera la llegada de algún mensaje del socket de Kernel Memory
        int cod_op = recibir_operacion(fd_memoria);

        if(cod_op == HANDSHAKE_CPU_A_MS) {
            
            char* mensaje_cpu = recibir_mensaje(fd_memoria);
            log_info(logger, "Llego un saludo de la CPU: %s", mensaje_cpu);
            
            free(mensaje_cpu);

            // Respondemos el saludo a la CPU (usando el mismo código de operación)
        
            log_info(logger, "Enviando respuesta a la CPU...");
            enviar_mensaje("¡Hola CPU! Handshake recibido por la Stick.", HANDSHAKE_CPU_A_MS, fd_memoria);
        }
        else {
            
            log_error(logger, "Se esperaba un saludo de la CPU pero llego otra cosa.");
        }
    }
    else {
        
        log_error(logger, "No se pudo conectar con la Memoria.");
        destruir_configuracion_ms();
        log_destroy(logger);
        return EXIT_FAILURE;
    }

    // ------------------------------ LIMPIEZA DE LA MEMORIA ------------------------------ //

    close(fd_memoria);
    destruir_configuracion_ms();
    log_destroy(logger);

    return 0;
}
