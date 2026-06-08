#include "conexiones.h"
#include "../core/memoria_core.h"
void* atender_cliente(void* arg) {

    int fd_cliente = *(int*)arg;
    free(arg); // Liberamos el puntero que usamos para pasar el FD

    // Recibir Handshake inicial
    int cod_op = recibir_operacion(fd_cliente);
    if (cod_op == MENSAJE) {

        char* id_modulo = recibir_mensaje(fd_cliente);
        log_info(logger, "Se conecto el modulo: %s", id_modulo);
        free(id_modulo);
    }

    //Bucle infinito para escuchar a cliente
    int conectado = 1;

    while(conectado) {

        int operacion = recibir_operacion(fd_cliente);
        
        switch(operacion) {
            
            //Atiende cuando el Kernel solicita inicializar un proceso y registrar su archivo .prc de forma dinámica
            case SYSCALL_INIT_PROC:
                atender_creacion_proceso(fd_cliente);
                break;

            case FETCH_INSTRUCCION:
                atender_fetch_cpu(fd_cliente);
                break;
            case CONSULTAR_ESPACIO: 
                atender_consulta_espacio(fd_cliente);
                break;
            case LEER_MEMORIA:      
                atender_lectura_memoria(fd_cliente);
                break;
            case ESCRIBIR_MEMORIA:  
                atender_escritura_memoria(fd_cliente);
                break;
            case HANDSHAKE_CPU_A_MS:
                log_info(logger, "Ruteando saludo de CPU a MS...");
                enviar_operacion(HANDSHAKE_CPU_A_MS, fd_cliente);
                enviar_mensaje("Hola CPU, soy la Stick respondiendo desde la Memoria", HANDSHAKE_CPU_A_MS, fd_cliente);
                break;
            case -1:
                log_error(logger, "Un cliente se desconecto.");
                conectado = 0;
                break;
            default:
                log_warning(logger, "Operacion desconocida: %d", operacion);
                break;
        }
    }

    close(fd_cliente);
    return NULL;
}