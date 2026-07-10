#include "main.h"
#include "instrucciones/instrucciones.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
t_log* logger;
int cpu_corriendo = 1;
int tam_max_segmento = 0;
t_list* sticks_cpu; // lista de sticks a las que estamos conectados

//funcion para conectarse a un stick y guardarlo en la lista
void conectar_a_stick(char* ip, char* puerto, uint32_t base_global, uint32_t tamanio) {
    
    int fd = crear_conexion(ip, puerto);
    if(fd != -1) {
        t_stick_cpu* stick = malloc(sizeof(t_stick_cpu));
        stick->fd_socket = fd;
        stick->base_global = base_global;
        stick->tamanio = tamanio;
        list_add(sticks_cpu, stick);
        log_info(logger, "Conectado a Memory Stick en %s:%s (base: %d, tam: %d)", ip, puerto, base_global, tamanio);
    } else {
        log_error(logger, "No pude conectarme al Memory Stick en %s:%s", ip, puerto);
    }
}

int main(int argc, char* argv[]) { 
    // La variable "argc" almacena la cantidad de palabras en la terminal, mientras que "argv[numero]" almacena la palabra en dicha posición.
    
    // Retorna ERROR si la cantidad de palabras en la terminal es menor a dos
    if(argc < 3) {
        printf("[ERROR] Debe utilizar el siguiente formato: ./bin/cpu ./config\n [identificado(numero)]" );
        return EXIT_FAILURE;
    }
    
    cargar_configuracion_cpu(argv[1]);
    logger = log_create("cpu.log", argv[2], 1, LOG_LEVEL_INFO);

    // ------------------------------ CONEXIONES ------------------------------ //
    
    int fd_memoria = crear_conexion(cpu_config.ip_memoria, cpu_config.puerto_memoria);
    if(fd_memoria != -1) {
        enviar_mensaje(cpu_config.id_modulo, MENSAJE, fd_memoria);
        log_info(logger, "Handshake enviado con ID: %s", cpu_config.id_modulo);
        //recibie el tamaño maximo de segmento de la memoria
        recv(fd_memoria, &tam_max_segmento, sizeof(int),MSG_WAITALL);
        log_info(logger, "Tamaño máximo de segmento recibido: %d", tam_max_segmento);
       // recibimos los sticks que ya existen y nos conectamos a cada uno
        int cant_sticks;
        recv(fd_memoria, &cant_sticks, sizeof(int), MSG_WAITALL);
        for(int i = 0; i < cant_sticks; i++) {
            int len_ip;
            recv(fd_memoria,&len_ip, sizeof(int), MSG_WAITALL);
            char* ip = malloc(len_ip);
            recv(fd_memoria, ip, len_ip, MSG_WAITALL);
                        
            int len_puerto;
            recv(fd_memoria, &len_puerto, sizeof(int), MSG_WAITALL);
            char* puerto = malloc(len_puerto);
            recv(fd_memoria, puerto, len_puerto, MSG_WAITALL);
            
            uint32_t base, tam;
            recv(fd_memoria, &base, sizeof(uint32_t), MSG_WAITALL);
            recv(fd_memoria, &tam, sizeof(uint32_t), MSG_WAITALL);
            conectar_a_stick(ip,puerto, base, tam);
            free(ip);
            free(puerto);
        }
    }

    int fd_scheduler = crear_conexion(cpu_config.ip_sched, cpu_config.puerto_sched);
    if(fd_scheduler != -1) {
        enviar_mensaje(cpu_config.id_modulo, MENSAJE, fd_scheduler);
        log_info(logger, "Handshake enviado con ID: %s", cpu_config.id_modulo);
    }
    
    // si las conexiones a Memoria y Scheduler fueron exitosas, inicia el CPU
    if(fd_memoria != -1 && fd_scheduler != -1) {
        log_info(logger, "CPU conectada a todos los modulos correctamente");
    }
// bucle principal( escucha al scheduler)
    while(cpu_corriendo) {

        // se bloquea la cpu esperando a que el scheduler mande una orden
        op_code cod_op_sched = recibir_operacion(fd_scheduler);

        if(cod_op_sched == -1) { // si el socket da -1 murio/desconecto el scheduler
            break;
        }
          //si KM nos avisa de un stick nuevo, nos conectamos
        if(cod_op_sched == NUEVO_STICK) {
            int len_ip;
            recv(fd_scheduler,&len_ip, sizeof(int), MSG_WAITALL);
            char* ip = malloc(len_ip);
            recv(fd_scheduler, ip, len_ip, MSG_WAITALL);

            int len_puerto;
            recv(fd_scheduler, &len_puerto, sizeof(int), MSG_WAITALL);
            char* puerto = malloc(len_puerto);
            recv(fd_scheduler, puerto, len_puerto, MSG_WAITALL);
            
            uint32_t base, tam;
            recv(fd_scheduler, &base, sizeof(uint32_t), MSG_WAITALL);
            recv(fd_scheduler, &tam, sizeof(uint32_t), MSG_WAITALL);
            conectar_a_stick(ip,puerto, base, tam);
            free(ip);
            free(puerto);
            continue;
        }

    if(cod_op_sched == CONTEXTO_PCB) {
            log_info(logger, "¡Recibí CONTEXTO_PCB del Scheduler!");
            t_pcb* pcb_actual = recibir_pcb(fd_scheduler);
            
            if(pcb_actual == NULL) {
                log_error(logger, "ERROR: pcb_actual es NULL después de recibir.");
                continue;
            }
            
            log_info(logger, "¡PCB recibido! PID: %d", pcb_actual->pid);
            int desalojar = 0;
            op_code motivo_desalojo = CONTEXTO_PCB;

            log_info(logger, "Entrando al ciclo de instrucción...");
            
            while(!desalojar && cpu_corriendo) {
                log_info(logger, "DEBUG: Voy a llamar a realizar_fetch...");
                char* instruccion = realizar_fetch(pcb_actual, fd_memoria, logger);
                log_info(logger, "DEBUG: Volví de realizar_fetch con instrucción: %s", instruccion);
                
                if(instruccion == NULL) {
                    log_error(logger, "ERROR: FETCH devolvió NULL");
                    break;
                }
                
                log_info(logger, "DEBUG: Instrucción recibida: %s", instruccion);
                
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
                ejecutar_instrucciones(tokens, pcb_actual, &desalojar, &motivo_desalojo, &modifico_pc, logger,fd_memoria);
                
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
            list_destroy_and_destroy_elements(pcb_actual->tabla_segmentos, free);
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