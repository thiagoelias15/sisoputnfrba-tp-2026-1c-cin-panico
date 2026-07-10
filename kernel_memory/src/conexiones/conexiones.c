#include "conexiones.h"
#include "../core/memoria_core.h"
#include "../memoria_administrador/memoria_administrador.h"
#include "../config/config.h"
extern t_list* tabla_segmentos_global;
extern pthread_mutex_t m_memoria;
int fd_scheduler_global = -1;
void* atender_cliente(void* arg) {

    int fd_cliente = *(int*)arg;
    free(arg); // Liberamos el puntero que usamos para pasar el FD

    // Recibir Handshake inicial
    int cod_op = recibir_operacion(fd_cliente);
    if (cod_op == MENSAJE) {

        char* id_modulo = recibir_mensaje(fd_cliente);
        log_info(logger, "Se conecto el modulo: %s", id_modulo);
     // se conecta la Stick
        if (strcmp(id_modulo, "MEMORY_STICK") == 0) {
            int tamaño_ms;
            recv(fd_cliente, &tamaño_ms, sizeof(int), MSG_WAITALL);
            
            //recibir ip del stick
            int len_ip;
            recv(fd_cliente, &len_ip, sizeof(int), MSG_WAITALL);
            char* ip_stick = malloc(len_ip);
            recv(fd_cliente, ip_stick, len_ip, MSG_WAITALL);
            //recibimos el puerto donde el stick escucha CPUs
            int len_puerto;
            recv(fd_cliente, &len_puerto, sizeof(int), MSG_WAITALL);
            char* puerto_stick = malloc(len_puerto);
            recv(fd_cliente, puerto_stick, len_puerto, MSG_WAITALL);
            log_info(logger, "## Memory Stick de %d bytes Conectada (puerto CPUs: %s)", tamaño_ms, puerto_stick);

            // Registrar el stick
            t_memory_stick_info* stick = malloc(sizeof(t_memory_stick_info));
            int fd_stick_rw = crear_conexion(ip_stick, puerto_stick);
            stick->fd_socket = fd_stick_rw;
            stick->ip_escucha = ip_stick;
            stick->puerto_escucha = puerto_stick;
            stick->tamanio = tamaño_ms;
            pthread_mutex_lock(&m_sticks);
            stick->base_global = memoria_total;
            memoria_total += tamaño_ms;
            list_add(lista_sticks, stick);
            // Crear/actualizar el hueco libre en la tabla de segmentos
            pthread_mutex_lock(&m_memoria);
            // Buscamos si ya hay un hueco libre al final para extenderlo
            int hueco_extendido = 0;
            if(list_size(tabla_segmentos_global) > 0) {
                t_segmento_memoria* ultimo = list_get(tabla_segmentos_global, 
                    list_size(tabla_segmentos_global) - 1);
                if(ultimo->ocupado == 0) {
                    ultimo->tamanio += tamaño_ms;
                    hueco_extendido = 1;
                }
            }
            if(!hueco_extendido) {
                t_segmento_memoria* nuevo_hueco = malloc(sizeof(t_segmento_memoria));
                nuevo_hueco->id = list_size(tabla_segmentos_global);
                nuevo_hueco->pid = -1;
                nuevo_hueco->base = stick->base_global;
                nuevo_hueco->tamanio = tamaño_ms;
                nuevo_hueco->ocupado = 0;
                list_add(tabla_segmentos_global, nuevo_hueco);
            }
            pthread_mutex_unlock(&m_memoria);

            // Avisarle a las CPUs conectadas que hay un stick nuevo
              pthread_mutex_lock(&m_cpus);
            for(int i = 0; i < list_size(lista_cpus_conectadas); i++) {
                int* fd_cpu = list_get(lista_cpus_conectadas, i);
                op_code op = NUEVO_STICK;
                send(*fd_cpu, &op, sizeof(op_code), 0);
                //mandamos ip
                int lip = strlen(stick ->ip_escucha) + 1;
                send(*fd_cpu, &lip, sizeof(int), 0);
                send(*fd_cpu, stick->ip_escucha, lip, 0);
                //mandamos puerto
                int lp = strlen(stick->puerto_escucha) + 1;
                send(*fd_cpu, &lp, sizeof(int), 0);
                send(*fd_cpu, stick->puerto_escucha, lp, 0);
                //mandamos base y tamaño
                send(*fd_cpu, &stick->base_global, sizeof(uint32_t), 0);
                send(*fd_cpu, &stick->tamanio, sizeof(uint32_t), 0);
            }
            pthread_mutex_unlock(&m_cpus);
            pthread_mutex_unlock(&m_sticks);

            free(id_modulo);
            // El stick no manda mas nada espontaneamente, KM le habla cuando necesita
            // NO cerramos fd_cliente porque lo guardamos en stick->fd_socket
            return NULL;
        }

        //  CPU se conecta
        if (strcmp(id_modulo, "CPU") == 0) {
            // Guardamos su fd para avisarle de sticks futuros
            int* fd_guardado = malloc(sizeof(int));
            *fd_guardado = fd_cliente;
            pthread_mutex_lock(&m_cpus);
            list_add(lista_cpus_conectadas, fd_guardado);
            pthread_mutex_unlock(&m_cpus);

            // Le mandamos el SEGMENT_MAX_SIZE para que la MMU funcione bien
            send(fd_cliente, &memoria_config.segment_max_size, sizeof(int), 0);

            // Le mandamos los sticks que ya existen para que se conecte
            pthread_mutex_lock(&m_sticks);
            int cant_sticks = list_size(lista_sticks);
            send(fd_cliente, &cant_sticks, sizeof(int), 0);
            for(int i = 0; i < cant_sticks; i++) {
                t_memory_stick_info* s = list_get(lista_sticks, i);
                //IP
                int lip = strlen(s->ip_escucha) + 1;
                send(fd_cliente, &lip, sizeof(int), 0);
                send(fd_cliente, s->ip_escucha, lip, 0);
                //PUERTO
                int lp = strlen(s->puerto_escucha) + 1;
                send(fd_cliente, &lp, sizeof(int), 0);
                send(fd_cliente, s->puerto_escucha, lp, 0);
                //BASE Y TAMAÑO
                send(fd_cliente, &s->base_global, sizeof(uint32_t), 0);
                send(fd_cliente, &s->tamanio, sizeof(uint32_t), 0);
            }
            pthread_mutex_unlock(&m_sticks);
        }
        if(strcmp(id_modulo, "SCHEDULER")== 0){
            log_info(logger, "## Kernel Scheduler Conectado - FD del socket: %d", fd_cliente);
            fd_scheduler_global = fd_cliente;
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
            case SYSCALL_MEM_ALLOC:
                atender_mem_alloc(fd_cliente);
                break;
            case SYSCALL_MEM_FREE:
                atender_mem_free(fd_cliente);
                break;
            case SYSCALL_EXIT: {
                int pid;
                recv(fd_cliente, &pid, sizeof(int), MSG_WAITALL);
                log_info(logger, "## PID: %d - Liberando todos los segmentos por EXIT", pid);
                liberar_todos_segmentos_pid(pid);
                int ok = 1;
                send(fd_cliente, &ok, sizeof(int), 0);
                break;
            }
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

    int id_asignado = asignar_memoria(pid, tam_segmento);
    int hubo_compactacion = 0;

    if(id_asignado == -1) {
        // Le avisamos al scheduler: -1 significa "necesito compactar"
        int necesita_compactar = -1;
        send(fd_cliente, &necesita_compactar, sizeof(int), 0);

        int confirmacion;
        recv(fd_cliente, &confirmacion, sizeof(int), MSG_WAITALL);

        log_info(logger, "## Inicio de compactación");
        compactar_memoria();
        log_info(logger, "## Fin de compactación");

        int fin_compactacion = 1;
        send(fd_cliente, &fin_compactacion, sizeof(int), 0);

        id_asignado = asignar_memoria(pid, tam_segmento);
        hubo_compactacion = 1;
    }

    if(!hubo_compactacion) {
        // Si NO compactamos, mandamos un 0 como primer respuesta
        // (el scheduler siempre espera este int primero)
        int ok = 0;
        send(fd_cliente, &ok, sizeof(int), 0);
    }

    uint32_t direccion_base = 999999;

    if(id_asignado != -1) {
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
        log_info(logger, "## PID: %d - Segmento Creado %d - Tamaño: %d - Base: %d", pid, id_segmento, tam_segmento, direccion_base);
    } else {
        log_error(logger, "Out of Memory incluso después de compactar.");
    }

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