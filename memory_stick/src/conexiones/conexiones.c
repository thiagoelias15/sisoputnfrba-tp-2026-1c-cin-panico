#include "conexiones.h"

void* atender_cpu(void* arg){
    int socket_cpu = *(int*)arg;
    free(arg);
    log_info(logger, "## CPU %d Conectada", socket_cpu);
    while(ms_corriendo) {
        op_code cod_op = recibir_operacion(socket_cpu);
        if(cod_op == -1) {
            log_warning(logger, "Se desconecto la CPU");
            break;
        }
        switch(cod_op) {
            
            case ESCRIBIR_MEMORIA: {
                int dir_fisica, tamanio;
                //recibimos la direccion fisica y el tamaño
                recv(socket_cpu, &dir_fisica, sizeof(int), MSG_WAITALL);
                recv(socket_cpu, &tamanio, sizeof(int), MSG_WAITALL);

                //recibimos el contendio a escribir
                void* datos_a_escribir = malloc(tamanio);
                recv(socket_cpu, datos_a_escribir, tamanio, MSG_WAITALL);

                //llamamos a la operacion 
                escribir_en_memoria(dir_fisica, datos_a_escribir, tamanio);

                //como resultado devolvemos una confirmacion (lo pide el enunciado)
                enviar_mensaje("OK", MENSAJE, socket_cpu);
                free(datos_a_escribir);
                break;
            }
            
            case LEER_MEMORIA: {
                int dir_fisica, tamanio;

                //recibimos la direccion y el tamaño a leer
                recv(socket_cpu, &dir_fisica, sizeof(int), MSG_WAITALL);
                recv(socket_cpu, &tamanio, sizeof(int), MSG_WAITALL);

                // llamamos a la operacion que nos devuelve los bytes
                void* datos_leidos = leer_de_memoria(dir_fisica, tamanio);

                //enviamos los bytes devueltos a la cpu
                send(socket_cpu, datos_leidos, tamanio, 0);
                free(datos_leidos);
                break;
            }
            
            default:
            log_warning(logger, "Operacion desconocida");
            break;
        }
    }
    return NULL;
}