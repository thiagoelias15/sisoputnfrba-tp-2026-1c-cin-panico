#include "conexiones.h"
#include "../core/memoria_core.h"
#include "../memoria_administrador/memoria_administrador.h"
extern t_list* tabla_segmentos_global;
extern pthread_mutex_t m_memoria;
void* atender_cliente(void* arg) {

    int fd_cliente = *(int*)arg;
    free(arg); // Liberamos el puntero que usamos para pasar el FD

    // Recibir Handshake inicial
    int cod_op = recibir_operacion(fd_cliente);
    if (cod_op == MENSAJE) {

        char* id_modulo = recibir_mensaje(fd_cliente);
        log_info(logger, "Se conecto el modulo: %s", id_modulo);
        if (strcmp(id_modulo, "MEMORY_STICK") == 0) { // O como sea que se llame tu ID
        int tamaño_ms;
        recv(fd_cliente, &tamaño_ms, sizeof(int), MSG_WAITALL);
        log_info(logger, "Recibí tamaño de MS: %d", tamaño_ms);
        }
        free(id_modulo);
    }
    //Bucle infinito para escuchar a cliente
    int conectado = 1;

    while(conectado) {

        int operacion = recibir_operacion(fd_cliente);
       log_warning(logger, "DEBUG: Recibí op_code: %d", operacion);
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
            case SYSCALL_MEM_ALLOC:
                atender_mem_alloc(fd_cliente);
                break;

            case SYSCALL_MEM_FREE:
                atender_mem_free(fd_cliente);
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

void atender_mem_alloc(int fd_cliente) {
    uint32_t pid;
    int id_segmento;
    int tam_segmento;

    recv(fd_cliente, &pid, sizeof(uint32_t), MSG_WAITALL);
    recv(fd_cliente, &id_segmento, sizeof(int), MSG_WAITALL);
    recv(fd_cliente, &tam_segmento, sizeof(int), MSG_WAITALL);

    log_info(logger, "## PID: %d - Crear Segmento - ID: %d - Tamaño: %d", pid, id_segmento, tam_segmento);

    // 1. Usamos función asignar_memoria (que ya hace el Best Fit)
    int id_asignado = asignar_memoria(pid, tam_segmento);
    
    uint32_t direccion_base = 0;

    if (id_asignado != -1) {
        // 2. Si encontró hueco, buscamos cuál es la dirección base que le asignó
        pthread_mutex_lock(&m_memoria);
        for(int i = 0; i < list_size(tabla_segmentos_global); i++) {
            t_segmento_memoria* seg = list_get(tabla_segmentos_global, i);
            // Buscamos el segmento de este PID que tenga el tamaño que acabamos de asignar
            if(seg->pid == pid && seg->tamanio == tam_segmento) {
                // Le forzamos el ID que nos pidió el Scheduler
                seg->id = id_segmento; 
                direccion_base = seg->base;
                break;
            }
        }
        pthread_mutex_unlock(&m_memoria);
    } else {
        // Si no hay hueco, su enunciado probablemente les pida compactar
        log_warning(logger, "No hay espacio. Iniciando compactación...");
        compactar_memoria();
        
        // Intentamos asignar de nuevo después de compactar
        id_asignado = asignar_memoria(pid, tam_segmento);
        if (id_asignado != -1) {
            pthread_mutex_lock(&m_memoria);
            for(int i = 0; i < list_size(tabla_segmentos_global); i++) {
                t_segmento_memoria* seg = list_get(tabla_segmentos_global, i);
                if(seg->pid == pid && seg->tamanio == tam_segmento) {
                    seg->id = id_segmento; 
                    direccion_base = seg->base;
                    break;
                }
            }
            pthread_mutex_unlock(&m_memoria);
        } else {
            // Si después de compactar tampoco entra, el sistema explotó por falta de RAM
            log_error(logger, "Out of Memory. Imposible crear segmento.");
            // Mandamos una dirección "imposible" para que el Scheduler sepa que falló
            direccion_base = 999999; 
        }
    }

    // 3. Devolvemos la dirección base real al Scheduler
    send(fd_cliente, &direccion_base, sizeof(uint32_t), 0);
}


void atender_mem_free(int fd_cliente) {
    uint32_t pid;
    int id_segmento;

    recv(fd_cliente, &pid, sizeof(uint32_t), MSG_WAITALL);
    recv(fd_cliente, &id_segmento, sizeof(int), MSG_WAITALL);

    log_info(logger, "## PID: %d - Destruir Segmento - ID: %d", pid, id_segmento);

    // 1. Usamos función que ya sabe marcar el espacio como libre
    liberar_memoria(pid, id_segmento);

    // 2. Le mandamos el OK al Scheduler
    int confirmacion = 1;
    send(fd_cliente, &confirmacion, sizeof(int), 0);
}