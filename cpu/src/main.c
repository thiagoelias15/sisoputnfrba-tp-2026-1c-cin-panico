#include "main.h"
#include "instrucciones/instrucciones.h"
#include "Ciclo_Instrucciones/ciclo_instrucciones.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>

t_log* logger;
int cpu_corriendo = 1;

int main(int argc, char* argv[]) { 
    // La variable "argc" almacena la cantidad de palabras en la terminal, mientras que "argv[numero]" almacena la palabra en dicha posición.
    
    // Retorna ERROR si la cantidad de palabras en la terminal es menor a dos
    if(argc < 2) {
        printf("[ERROR] Debe utilizar el siguiente formato: ./bin/cpu ./config\n");
        return EXIT_FAILURE;
    }
    
    cargar_configuracion_cpu(argv[1]);
    logger = log_create("cpu.log", "CPU", 1, LOG_LEVEL_INFO);

    // ------------------------------ CONEXIONES ------------------------------ //
    
    int fd_memoria = crear_conexion(cpu_config.ip_memoria, cpu_config.puerto_memoria);
    if(fd_memoria != -1) {
        enviar_mensaje(cpu_config.id_modulo, MENSAJE, fd_memoria);
        log_info(logger, "Handshake enviado con ID: %s", cpu_config.id_modulo);
    }

    int fd_scheduler = crear_conexion(cpu_config.ip_sched, cpu_config.puerto_sched);
    if(fd_scheduler != -1) {
        enviar_mensaje(cpu_config.id_modulo, MENSAJE, fd_scheduler);
        log_info(logger, "Handshake enviado con ID: %s", cpu_config.id_modulo);
    }
    
    // si las conexiones a Memoria y Scheduler fueron exitosas, inicia a andar el CPU
    if(fd_memoria != -1 && fd_scheduler != -1) {
        log_info(logger, "CPU conectada a todos los modulos correctamente");
        log_info(logger, "Enviando saludo a la Memory Stick a traves de la Memoria...");
        enviar_mensaje("¡Hola Stick! Soy la CPU mandando un saludo.", HANDSHAKE_CPU_A_MS, fd_memoria);

        // Esperamos la respuesta del Memory Stick, el cual llega a través de la memoria.
        int cod_op = recibir_operacion(fd_memoria);
        
        if(cod_op == HANDSHAKE_CPU_A_MS) {
            char* respuesta_ms = recibir_mensaje(fd_memoria);
            log_info(logger, "La Stick me respondio correctamente: %s", respuesta_ms);
            free(respuesta_ms);
        } else {
            log_error(logger, "Fallo el handshake con la Stick. Codigo recibido: %d", cod_op);
        }
    }

    // bucle principal( escucha al scheduler)
    while(cpu_corriendo) {

        // se bloquea la cpu esperando a que el scheduler mande una orden
        op_code cod_op_sched = recibir_operacion(fd_scheduler);

        if(cod_op_sched == -1) { // si el socket da -1 murio/desconecto el scheduler
            break;
        }

        if(cod_op_sched == CONTEXTO_PCB) { // si la orden es ejecutar un proceso, recibimos su "contexto(pcb)"
            
            t_pcb* pcb_actual = recibir_pcb(fd_scheduler);
            int desalojar = 0; // flag para saber cuando devolver el proceso al scheduler
            op_code motivo_desalojo = CONTEXTO_PCB;

            //-------------- CICLO DE INSTRUCCION (FETCH -> DECODE -> EXECUTE -> INTERRUPT STAGE)
            while(!desalojar && cpu_corriendo) {

                //--------- ETAPA 1° FETCH (buscar instruccion en Memoria)------------
                // esperamos la respuesta(el texto de instruccion)
                char* instruccion = realizar_fetch(pcb_actual, fd_memoria, logger);
                
                //------ ETAPA 2° DECODE ---------------
                // cortamos el texto por los espacios. token[0]= Comando, token[1]= parametro1 y asi sucesivamente
                char** tokens = string_split(instruccion, " ");
                char* comando = tokens[0];
                
                // logs dinamicos adaptados a la cantidad de parametros
                if(tokens[1] != NULL && tokens[2] != NULL) { 
                    log_info(logger, "## PID: %d - Ejecutando: %s - %s %s", pcb_actual->pid, comando, tokens[1], tokens[2]);
                } else if(tokens[1] != NULL) {
                    log_info(logger, "## PID: %d - Ejecutando: %s - %s", pcb_actual->pid, comando, tokens[1]);
                } else {
                    log_info(logger, "## PID: %d - Ejecutando: %s -", pcb_actual->pid, comando);
                }

                int modifico_pc = 0; // flag para saber si la instruccion altero el pc(ej un salto JNZ)

                //--------- ETAPA 3° EXECUTE(la funcion esta en la carpeta instrucciones) ---------------------------
                ejecutar_instrucciones(tokens, pcb_actual, &desalojar, &motivo_desalojo, &modifico_pc, logger);
                
                // si la instruccion no fue un JNZ, incrementamos el pc para la proxima instruccion
                if(!modifico_pc) {
                    pcb_actual->pc++;
                }
        
                // ETAPA 4° INTERRUPT STAGE( revisa si ocurrio alguna interrupcion, es decir el scheduler para la cpu)
                if(!desalojar && hay_interrupcion(fd_scheduler, logger)) {
                    desalojar = 1;
                    motivo_desalojo = INTERRUPCION; // ej: puede pasar por fin de quantum
                }
                
                //----------------- DESALOJO----------------------
                // si se rompio el ciclo se le devuelve todo al scheduler
                if(desalojar) {
                    gestionar_desalojo(pcb_actual, motivo_desalojo, tokens, fd_scheduler);
                }

                // Liberamos la memoria del texto y el array de palabras de este ciclo
                string_array_destroy(tokens);
                free(instruccion);
            }
                
            // Liberamos el PCB cuando ya lo devolvimos y terminamos de trabajar con él
            free(pcb_actual);
        }
    }

    // ------------------------------ LIMPIEZA DE LA MEMORIA ------------------------------ //
    
    close(fd_memoria);
    close(fd_scheduler);
    destruir_configuracion_cpu();
    log_destroy(logger);
    
    return 0; 
}