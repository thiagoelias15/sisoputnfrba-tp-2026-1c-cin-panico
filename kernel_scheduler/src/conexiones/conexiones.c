#include "conexiones.h"
#include <stdbool.h>
#include <sys/socket.h>
extern t_pcb *pcb_en_ejecucion;
void intentar_desuspender();
void reintentar_esperando_memoria();
void* monitor_bsod(void* arg);

typedef struct
{
    int pid;
    int epoch;
} t_timer_args;
void bsod_exit() {
    log_error(logger, "## BLUE SCREEN OF DEATH - Memoria corrupta detectada");
    log_error(logger, "## Finalizando Kernel Scheduler");
    scheduler_corriendo = 0;
    exit(EXIT_FAILURE);
}

void* monitor_bsod(void* arg) {
    int error = 0;
    socklen_t len = sizeof(error);
    while(1) {
        usleep(500000);
        if(getsockopt(fd_memoria, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error != 0) {
            bsod_exit();
        }
        // Intentar leer 0 bytes para detectar cierre
        char peek;
        int r = recv(fd_memoria, &peek, 1, MSG_PEEK | MSG_DONTWAIT);
        if(r == 0) {
            bsod_exit();
        }
    }
    return NULL;
}



int recv_memoria(void* buf, size_t size) {
    int bytes = recv(fd_memoria, buf, size, MSG_WAITALL);
    if(bytes <= 0) bsod_exit();
    return bytes;
}
void reintentar_esperando_memoria()
{
    pthread_mutex_lock(&m_esperando_memoria);

    for (int i = 0; i < list_size(cola_esperando_memoria); i++)
    {
        t_pcb *pcb = list_get(cola_esperando_memoria, i);

        pthread_mutex_lock(&m_fd_memoria);
        op_code op = SYSCALL_MEM_ALLOC;
        send(fd_memoria, &op, sizeof(op_code), 0);
        send(fd_memoria, &(pcb->pid), sizeof(uint32_t), 0);
        send(fd_memoria, &(pcb->mem_pendiente_id), sizeof(int), 0);
        send(fd_memoria, &(pcb->mem_pendiente_tam), sizeof(int), 0);

        int primer_respuesta;
        recv_memoria(&primer_respuesta, sizeof(int));
        if (primer_respuesta == -1)
        {
            int ok = 1;
            send(fd_memoria, &ok, sizeof(int), 0);
            int fin_compactacion;
            recv_memoria(&fin_compactacion, sizeof(int));
        }
        uint32_t direccion_base;
        recv_memoria(&direccion_base, sizeof(uint32_t));
        pthread_mutex_unlock(&m_fd_memoria);

        if (direccion_base != 999999)
        {
            t_segmento *nuevo = malloc(sizeof(t_segmento));
            nuevo->id = pcb->mem_pendiente_id;
            nuevo->tamanio = (uint32_t)pcb->mem_pendiente_tam;
            nuevo->direccion_base = direccion_base;
            list_add(pcb->tabla_segmentos, nuevo);

            list_remove(cola_esperando_memoria, i);
            i--;

            log_info(logger, "## (%d) Consiguió memoria - Vuelve a READY", pcb->pid);
            pcb->estado = READY;
            int prio = 0;
            if (strcmp(kernel_config.algoritmo_planificacion, "CMN") == 0)
            {
                prio = pcb->prioridad;
            }
            pthread_mutex_lock(&m_ready);
            list_add(colas_ready[prio], pcb);
            pthread_mutex_unlock(&m_ready);
            sem_post(&sem_procesos_ready);
        }
    }

    pthread_mutex_unlock(&m_esperando_memoria);
}

bool comparar_prioridades(void *pcb1, void *pcb2)
{
    t_pcb *p1 = (t_pcb *)pcb1;
    t_pcb *p2 = (t_pcb *)pcb2;

    // numero menor = mayor prioridad
    // retorna True si p1 debe ir antes que p2 en la cola ready
    return p1->prioridad < p2->prioridad;
}

void* timer_suspension(void* arg) {
    t_timer_args* args = (t_timer_args*)arg;
    int pid = args->pid;
    int epoch_al_lanzar = args->epoch;
    free(args);

    usleep(kernel_config.suspension_timeout * 1000);

    if(block_epoch[pid] != epoch_al_lanzar) {
        return NULL;
    }

    t_pcb* encontrado = sacar_de_block(pid);

    if(encontrado != NULL) {
        encontrado->estado = SUSP_BLOCK;
        log_info(logger, "## (%d) Pasa del estado BLOCK al estado SUSP_BLOCK", pid);

        // Primero escribimos a SWAP (puede tardar)
        pthread_mutex_lock(&m_fd_memoria);
        op_code op = SWAP_ESCRITURA;
        send(fd_memoria, &op, sizeof(op_code), 0);
        send(fd_memoria, &pid, sizeof(int), 0);
        int confirmacion;
        recv_memoria(&confirmacion, sizeof(int));
        pthread_mutex_unlock(&m_fd_memoria);
        log_info(logger, "## (%d) Segmentos movidos a SWAP", pid);

        // DESPUÉS del SWAP write, chequeamos si FIN_IO llegó mientras tanto
        pthread_mutex_lock(&m_susp);
        if(pending_wakeup[pid]) {
            // FIN_IO llegó durante el SWAP write: va directo a SUSP_READY
            pending_wakeup[pid] = 0;
            encontrado->estado = SUSP_READY;
            list_add(cola_susp_ready, encontrado);
            log_info(logger, "## (%d) Wakeup pendiente, pasa a SUSP_READY", pid);
            pthread_mutex_unlock(&m_susp);
            intentar_desuspender();
            reintentar_esperando_memoria();
        } else {
            list_add(cola_susp_block, encontrado);
            pthread_mutex_unlock(&m_susp);
        }
    }

    return NULL;
}
void intentar_desuspender()
{
    pthread_mutex_lock(&m_susp);

    list_sort(cola_susp_ready, comparar_prioridades);

    for (int i = 0; i < list_size(cola_susp_ready); i++)
    {
        t_pcb *pcb = list_get(cola_susp_ready, i);
        pthread_mutex_lock(&m_fd_memoria);
        op_code op = SWAP_LECTURA;
        send(fd_memoria, &op, sizeof(op_code), 0);
        send(fd_memoria, &(pcb->pid), sizeof(int), 0);

        int resultado;
        recv_memoria(&resultado, sizeof(int));
        pthread_mutex_unlock(&m_fd_memoria);
        if (resultado == 1)
        {
            list_remove(cola_susp_ready, i);
            i--;

            pcb->estado = READY;
            log_info(logger, "## (%d) Pasa del estado SUSP_READY al estado READY", pcb->pid);

            int prio = 0;
            if (strcmp(kernel_config.algoritmo_planificacion, "CMN") == 0)
            {
                prio = pcb->prioridad;
            }

            pthread_mutex_lock(&m_ready);
            list_add(colas_ready[prio], pcb);
            pthread_mutex_unlock(&m_ready);

            sem_post(&sem_procesos_ready);
        }
    }

    pthread_mutex_unlock(&m_susp);
}
void *atender_cliente(void *arg)
{

    int socket_cliente = *(int *)arg; // arg es un puntero que apunta a la direccion de memoria donde el main guardo el numero de socket y (int*)*arg le dice al compilador que trate a ese puntero como un entero
    free(arg);

    recibir_operacion(socket_cliente);
    char *id_recibida = recibir_mensaje(socket_cliente);

    if (strcmp(id_recibida, "CPU") == 0)
    {

        t_cpu_info *cpu_nueva = malloc(sizeof(t_cpu_info));
        cpu_nueva->fd_cpu = socket_cliente;
        cpu_nueva->pid_ejecutando = -1;
        pthread_mutex_lock(&m_cpus_sched);
        list_add(lista_cpus_sched, cpu_nueva);
        sem_post(&sem_cpu_libre);
        pthread_mutex_unlock(&m_cpus_sched);
        log_info(logger, "## CPU %d Conectada", socket_cliente);

        while (scheduler_corriendo)
        {

            op_code cod_op = recibir_operacion(socket_cliente);
            if (cod_op == -1)
                break;
            t_pcb *pcb_upd = recibir_pcb(socket_cliente);
            pcb_en_ejecucion = NULL;
            pthread_mutex_lock(&m_cpus_sched);
            for (int i = 0; i < list_size(lista_cpus_sched); i++)
            {
                t_cpu_info *ci = list_get(lista_cpus_sched, i);
                if (ci->fd_cpu == socket_cliente)
                {
                    ci->pid_ejecutando = -1;
                    break;
                }
            }
            pthread_mutex_unlock(&m_cpus_sched);
            sem_post(&sem_cpu_libre);

            switch (cod_op)
            {

            case SYSCALL_EXIT:
            {

                log_info(logger, "## (%d) Solicito syscall: EXIT", pcb_upd->pid);
                log_info(logger, "## (%d) Pasa del estado EXEC al estado EXIT", pcb_upd->pid);
                pthread_mutex_lock(&m_fd_memoria);
                // avisarle a la memory que libere los segmentos de ese proceso
                op_code op_exit = SYSCALL_EXIT;
                send(fd_memoria, &op_exit, sizeof(op_code), 0);
                send(fd_memoria, &(pcb_upd->pid), sizeof(int), 0);
                int confirmacion;
                recv_memoria(&confirmacion, sizeof(int));
                pthread_mutex_unlock(&m_fd_memoria);
                log_info(logger, "## (%d) finalizó su ejecución", pcb_upd->pid);
                list_add(cola_exit, pcb_upd);
                sem_post(&sem_procesos_ready); // La CPU queda libre
                intentar_desuspender();        // Intentamos desuspender procesos si es posible
                reintentar_esperando_memoria();
                break;
            }

            case SEG_FAULT:
            {
                log_info(logger, "## (%d) Pasa del estado EXEC al estado EXIT", pcb_upd->pid);
                pthread_mutex_lock(&m_fd_memoria);
                // avisamos a la memory que libere los segmentos de ese proceso
                op_code op_exit = SYSCALL_EXIT;
                send(fd_memoria, &op_exit, sizeof(op_code), 0);
                send(fd_memoria, &(pcb_upd->pid), sizeof(int), 0);
                int confirmacion;
               recv_memoria(&confirmacion, sizeof(int));
                pthread_mutex_unlock(&m_fd_memoria);
                log_info(logger, "## (%d) finalizó su ejecución por SEG_FAULT", pcb_upd->pid);

                // lo matamos
                list_add(cola_exit, pcb_upd);

                // Liberamos la CPU
                sem_post(&sem_procesos_ready);
                intentar_desuspender();
                reintentar_esperando_memoria();
                break;
            }

            case INTERRUPCION:
            {
                log_info(logger, "## (%d) Pasa del estado EXEC al estado READY", pcb_upd->pid);

                // Fin de Quantum: vuelve a la fila
                pcb_upd->estado = READY;

                int prio = 0;
                if (strcmp(kernel_config.algoritmo_planificacion, "CMN") == 0)
                {
                    prio = pcb_upd->prioridad;
                }

                pthread_mutex_lock(&m_ready);
                list_add(colas_ready[prio], pcb_upd);
                pthread_mutex_unlock(&m_ready);

                // Avisamos que hay alguien esperando
                sem_post(&sem_procesos_ready);
                break;
            }
            case SYSCALL_SLEEP:
            {

                int tiempo_ms;
                recv(socket_cliente, &tiempo_ms, sizeof(int), MSG_WAITALL);
                log_info(logger, "## (%d) Solicito syscall: SLEEP", pcb_upd->pid);
                log_info(logger, "## (%d) Pasa del estado EXEC al estado BLOCK", pcb_upd->pid);
                pthread_mutex_lock(&m_block);
                list_add(cola_block, pcb_upd);
                pthread_mutex_unlock(&m_block);
                // Lanzar timer de suspensión
                block_epoch[pcb_upd->pid]++;
                t_timer_args* timer_args = malloc(sizeof(t_timer_args));
                timer_args->pid = pcb_upd->pid;
                timer_args->epoch = block_epoch[pcb_upd->pid];
                pthread_t hilo_susp;
                pthread_create(&hilo_susp, NULL, timer_suspension, timer_args);
                pthread_detach(hilo_susp);

                if (dictionary_has_key(dic_interfaces, "SLEEP"))
                {

                    // Buscamos el enchufe específico del SLEEP
                    int socket_sleep = (int)(intptr_t)dictionary_get(dic_interfaces, "SLEEP");
                    enviar_mensaje("SLEEP", SYSCALL_SLEEP, socket_sleep);
                    send(socket_sleep, &tiempo_ms, sizeof(int), 0);
                    send(socket_sleep, &(pcb_upd->pid), sizeof(int), 0);
                }
                else
                {
                    log_error(logger, "La interfaz SLEEP no está conectada.");
                }
                break;
            }

            case SYSCALL_MUTEX_CREATE:
            { // mutex create crea una cola con un nombre determinado y guarda en dictionary todas esas colas que va creando para no repetir

                char *m_name = recibir_mensaje(socket_cliente); // recibe el nombre que mande la cpu
                if (!dictionary_has_key(dic_mutex, m_name))
                { // se fija si ya existe ese nombre
                    // Creamos la nueva estructura del pcb del main.h
                    t_mutex *nuevo_mutex = malloc(sizeof(t_mutex));
                    nuevo_mutex->owner = NULL; // nadie lo tiene todavia
                    nuevo_mutex->bloqueados = queue_create();

                    dictionary_put(dic_mutex, m_name, nuevo_mutex);
                }
                // El proceso vuelve a READY
                pcb_upd->estado = READY;
                int prio_create = 0;
                if (strcmp(kernel_config.algoritmo_planificacion, "CMN") == 0)
                {
                    prio_create = pcb_upd->prioridad;
                }
                pthread_mutex_lock(&m_ready);
                list_add(colas_ready[prio_create], pcb_upd);
                pthread_mutex_unlock(&m_ready);
                sem_post(&sem_procesos_ready);

                free(m_name);
                break;
            }

            case SYSCALL_MUTEX_LOCK:
            {

                char *m_name = recibir_mensaje(socket_cliente);
                t_mutex *mutex_actual = dictionary_get(dic_mutex, m_name);

                if (mutex_actual->owner == NULL)
                {
                    // Si el mutex está libre
                    log_info(logger, "## (%d) Toma el mutex %s", pcb_upd->pid, m_name);
                    mutex_actual->owner = pcb_upd;
                    pcb_upd->estado = READY;
                    int prio_mutex = 0;
                    if (strcmp(kernel_config.algoritmo_planificacion, "CMN") == 0)
                    {
                        prio_mutex = pcb_upd->prioridad;
                    }
                    pthread_mutex_lock(&m_ready);
                    list_add(colas_ready[prio_mutex], pcb_upd);
                    pthread_mutex_unlock(&m_ready);
                    sem_post(&sem_procesos_ready);
                }
                else
                {
                    // Mutex bloqueado: bloquea proceso
                    log_info(logger, "## (%d) Pasa del estado de EXEC al estado BLOCK", pcb_upd->pid);

                    // Se mueve a cola bloqueados
                    pthread_mutex_lock(&m_block);
                    list_add(cola_block, pcb_upd);
                    pthread_mutex_unlock(&m_block);
                    // Lanzar timer de suspensión
                    block_epoch[pcb_upd->pid]++;
                    t_timer_args* timer_args = malloc(sizeof(t_timer_args));
                    timer_args->pid = pcb_upd->pid;
                    timer_args->epoch = block_epoch[pcb_upd->pid];
                    pthread_t hilo_susp;
                    pthread_create(&hilo_susp, NULL, timer_suspension, timer_args);
                    pthread_detach(hilo_susp);
                    queue_push(mutex_actual->bloqueados, pcb_upd);

                    // Herencia de prioridades
                    if (pcb_upd->prioridad < mutex_actual->owner->prioridad)
                    {
                        int owner_pid = mutex_actual->owner->pid;
                        int nueva_prio = pcb_upd->prioridad;

                        log_info(logger, "## (%d) hereda prioridad %d a (%d)", pcb_upd->pid, nueva_prio, owner_pid);
                        mutex_actual->owner->prioridad = nueva_prio;

                        // Actualizar en cola_block
                        pthread_mutex_lock(&m_block);
                        for (int i = 0; i < list_size(cola_block); i++)
                        {
                            t_pcb *p = list_get(cola_block, i);
                            if (p->pid == owner_pid)
                            {
                                p->prioridad = nueva_prio;
                                break;
                            }
                        }
                        pthread_mutex_unlock(&m_block);

                        // Actualizar si está ejecutando
                        if (pcb_en_ejecucion != NULL && pcb_en_ejecucion->pid == owner_pid)
                        {
                            pcb_en_ejecucion->prioridad = nueva_prio;
                        }

                        // Actualizar en colas_ready y moverlo a la cola correcta
                        pthread_mutex_lock(&m_ready);
                        for (int q = 0; q < cantidad_colas; q++)
                        {
                            int encontrado = 0;
                            for (int j = 0; j < list_size(colas_ready[q]); j++)
                            {
                                t_pcb *p = list_get(colas_ready[q], j);
                                if (p->pid == owner_pid)
                                {
                                    p->prioridad = nueva_prio;
                                    list_remove(colas_ready[q], j);
                                    list_add(colas_ready[nueva_prio], p);
                                    encontrado = 1;
                                    break;
                                }
                            }
                            if (encontrado)
                                break;
                        }
                        pthread_mutex_unlock(&m_ready);
                    }
                }

                free(m_name);
                break;
            }

            case SYSCALL_MUTEX_UNLOCK:
            {

                char *m_name = recibir_mensaje(socket_cliente);
                log_info(logger, "## (%d) Libera el mutex %s", pcb_upd->pid, m_name);
                t_mutex *mutex_actual = dictionary_get(dic_mutex, m_name);
                // restauro la prioridad original por si se la habian cambiado
                pcb_upd->prioridad = pcb_upd->prioridad_original;

                // le paso al mutex al siguiente de la fila(si hay alguno)
                if (!queue_is_empty(mutex_actual->bloqueados))
                {

                    t_pcb *proximo = queue_pop(mutex_actual->bloqueados); // queue_pop saca al primero elemento de bloqueados y lo devuelve para que se pueda usar y el segundo pasa a la primera posicion
                    mutex_actual->owner = proximo;                        // el nuevo owner es el que estaba esperando
                    mover_a_ready(proximo->pid);                          // busca ese siguiente proceso en la colaa de BLOCK y lo pasa a READY
                }
                else
                {
                    mutex_actual->owner = NULL; // Nadie lo estaba esperando, queda libre
                }
                // El proceso que soltó el mutex vuelve a READY
                pcb_upd->estado = READY;
                int prio_unlock = 0;
                if (strcmp(kernel_config.algoritmo_planificacion, "CMN") == 0)
                {
                    prio_unlock = pcb_upd->prioridad;
                }
                pthread_mutex_lock(&m_ready);
                list_add(colas_ready[prio_unlock], pcb_upd);
                pthread_mutex_unlock(&m_ready);
                sem_post(&sem_procesos_ready);

                free(m_name);
                break;
            }

            case SYSCALL_STDIN:
            {

                int tam, dir, pid_sobrante; // Recibimos de la CPU los parámetros necesarios: tamaño y dirección física
                // Recibimos de forma bloqueante el tamaño del buffer que STDIN debe leer
                // MSG_WAITALL asegura que no continúe hasta recibir los 4 bytes del int
                recv(socket_cliente, &tam, sizeof(int), MSG_WAITALL);
                // Recibimos la dirección física de memoria donde se debe escribir lo ingresado
                // Esta información la envía la CPU tras traducir la dirección lógica
                recv(socket_cliente, &dir, sizeof(int), MSG_WAITALL);
                recv(socket_cliente, &pid_sobrante, sizeof(int), MSG_WAITALL);
                log_info(logger, "##(%d) Solicitó syscall: STDIN", pcb_upd->pid);
                log_info(logger, "##(%d) Pasa del estado EXEC al estado BLOCK", pcb_upd->pid);
                pthread_mutex_lock(&m_block);   // Bloquea acceso a cola de bloqueados
                list_add(cola_block, pcb_upd);  // Agrega el proceso a la cola BLOCK
                pthread_mutex_unlock(&m_block); // Libera el mutex de la cola
                // Lanzar timer de suspensión
               block_epoch[pcb_upd->pid]++;
                t_timer_args* timer_args = malloc(sizeof(t_timer_args));
                timer_args->pid = pcb_upd->pid;
                timer_args->epoch = block_epoch[pcb_upd->pid];
                pthread_t hilo_susp;
                pthread_create(&hilo_susp, NULL, timer_suspension, timer_args);
                pthread_detach(hilo_susp);
                if (dictionary_has_key(dic_interfaces, "STDIN"))
                {

                    int socket_stdin = (int)(intptr_t)dictionary_get(dic_interfaces, "STDIN");
                    enviar_mensaje("STDIN", SYSCALL_STDIN, socket_stdin);
                    send(socket_stdin, &tam, sizeof(int), 0);
                    send(socket_stdin, &dir, sizeof(int), 0);
                    send(socket_stdin, &(pcb_upd->pid), sizeof(int), 0);
                }
                else
                {
                    log_error(logger, "La interfaz STDIN no está conectada.");
                }

                break;
            }

            case SYSCALL_STDOUT:
            {

                int tam, dir, pid_sobrante;
                recv(socket_cliente, &tam, sizeof(int), MSG_WAITALL); // Recibe tamaño a mostrar desde CPU
                recv(socket_cliente, &dir, sizeof(int), MSG_WAITALL); // Recibe dirección física de orígen
                recv(socket_cliente, &pid_sobrante, sizeof(int), MSG_WAITALL);
                log_info(logger, "##(%d) Solicitó syscall: STDOUT", pcb_upd->pid);
                log_info(logger, "##(%d) Pasa del estado EXEC al estado BLOCK", pcb_upd->pid); // Como toda operación de I/O es lenta, el proceso no puede seguir en la CPU. Se lo mueve de EXEC a BLOCK.
                pthread_mutex_lock(&m_block);                                                  // Protege la cola de bloqueados
                list_add(cola_block, pcb_upd);                                                 // Mueve el PCB a estado bloqueado
                pthread_mutex_unlock(&m_block);                                                // Libera la protección de la cola
                // Lanzar timer de suspensión
                block_epoch[pcb_upd->pid]++;
                t_timer_args* timer_args = malloc(sizeof(t_timer_args));
                timer_args->pid = pcb_upd->pid;
                timer_args->epoch = block_epoch[pcb_upd->pid];
                pthread_t hilo_susp;
                pthread_create(&hilo_susp, NULL, timer_suspension, timer_args);
                pthread_detach(hilo_susp);
                // 1. EL SCHEDULER LE PIDE LA INFO A MEMORIA
                pthread_mutex_lock(&m_fd_memoria);
                op_code op_leer = LEER_MEMORIA;
                send(fd_memoria, &op_leer, sizeof(op_code), 0);
                send(fd_memoria, &dir, sizeof(int), 0);
                send(fd_memoria, &tam, sizeof(int), 0);

                // Preparamos un buffer y recibimos el texto
                char *texto_de_memoria = calloc(tam + 1, sizeof(char)); // +1 para el '\0'
               recv_memoria(texto_de_memoria, tam);
                pthread_mutex_unlock(&m_fd_memoria);

                /*Si hay una interfaz conectada , el Kernel le envía un mensaje avisando que hay una tarea de STDOUT.
                Le pasa el tamaño, la dirección y el PID del proceso para que la interfaz sepa a quién pertenece la operación*/

                if (dictionary_has_key(dic_interfaces, "STDOUT"))
                {

                    int socket_stdout = (int)(intptr_t)dictionary_get(dic_interfaces, "STDOUT"); // intptr_t es de una biblioteca stdin.h y lo que hace es una variable que se asegura que el dato tenga el mismo tamaño en bytes que el puntero para que no tire error
                    enviar_mensaje("STDOUT", SYSCALL_STDOUT, socket_stdout);
                    send(socket_stdout, &(pcb_upd->pid), sizeof(int), 0);
                    // En lugar de enviar_mensaje, mandamos los paquetes crudos
                    // para que encajen perfecto en el recibir_mensaje de la I/O
                    int tam_texto = tam + 1; // +1 por el '\0' final
                    send(socket_stdout, &tam_texto, sizeof(int), 0);
                    send(socket_stdout, texto_de_memoria, tam_texto, 0);
                }
                else
                {
                    log_error(logger, "La interfaz STDOUT no está conectada.");
                }

                free(texto_de_memoria);
                break;
            }
            case SYSCALL_INIT_PROC:
            {

                int len_string;
                recv(socket_cliente, &len_string, sizeof(int), MSG_WAITALL);

                char *archivo_instrucciones = malloc(len_string);
                recv(socket_cliente, archivo_instrucciones, len_string, MSG_WAITALL);

                int prioridad;
                recv(socket_cliente, &prioridad, sizeof(int), MSG_WAITALL);

                log_info(logger, "## (%d) Solicitó syscall: INIT_PROC", pcb_upd->pid);

                crear_proceso(archivo_instrucciones, prioridad);

                pcb_upd->estado = READY;

                int prio = 0;
                if (strcmp(kernel_config.algoritmo_planificacion, "CMN") == 0)
                {
                    prio = pcb_upd->prioridad;
                }

                pthread_mutex_lock(&m_ready);
                list_add(colas_ready[prio], pcb_upd);
                pthread_mutex_unlock(&m_ready);

                sem_post(&sem_procesos_ready);

                free(archivo_instrucciones);
                break;
            }

            case SYSCALL_MEM_ALLOC:
            {
                int id_segmento;
                int tam_segmento;
                recv(socket_cliente, &id_segmento, sizeof(int), MSG_WAITALL);
                recv(socket_cliente, &tam_segmento, sizeof(int), MSG_WAITALL);

                log_info(logger, "## (%d) Solicitó syscall: MEM_ALLOC - ID: %d - Tam: %d", pcb_upd->pid, id_segmento, tam_segmento);
                pthread_mutex_lock(&m_fd_memoria);
                // 1. Le mandamos la solicitud real a Kernel Memory
                op_code op_memoria = SYSCALL_MEM_ALLOC;
                send(fd_memoria, &op_memoria, sizeof(op_code), 0);
                send(fd_memoria, &(pcb_upd->pid), sizeof(uint32_t), 0);
                send(fd_memoria, &id_segmento, sizeof(int), 0);
                send(fd_memoria, &tam_segmento, sizeof(int), 0);

                // KM contesta si es -1 -> necesita compactar
                //  si es >= a 0 -> no necesita
                int primer_respuesta;
               recv_memoria(&primer_respuesta, sizeof(int));
                if (primer_respuesta == -1)
                {
                    // KM necesita que desalojemos todas las CPUs
                    int ok = 1;
                    send(fd_memoria, &ok, sizeof(int), 0);
                    // esperamos que KM compacte
                    int fin_compactacion;
                    recv_memoria(&fin_compactacion, sizeof(int));
                    log_info(logger, " ## Fin de compactacion");
                }
                // 2. Esperamos que la Memoria haga su magia y nos devuelva la Dirección Base
                uint32_t direccion_base;
                recv_memoria(&direccion_base, sizeof(uint32_t));
                pthread_mutex_unlock(&m_fd_memoria);
                // 3. Creamos la estructura del segmento
                if (direccion_base == 999999)
                {
                    log_info(logger, "## (%d) MEM_ALLOC sin espacio - Proceso esperando memoria", pcb_upd->pid);
                    pcb_upd->mem_pendiente_id = id_segmento;
                    pcb_upd->mem_pendiente_tam = tam_segmento;

                    pthread_mutex_lock(&m_esperando_memoria);
                    list_add(cola_esperando_memoria, pcb_upd);
                    pthread_mutex_unlock(&m_esperando_memoria);
                }
                else
                {
                    t_segmento *nuevo_segmento = malloc(sizeof(t_segmento));
                    nuevo_segmento->id = id_segmento;
                    nuevo_segmento->tamanio = (uint32_t)tam_segmento;
                    nuevo_segmento->direccion_base = direccion_base;
                    list_add(pcb_upd->tabla_segmentos, nuevo_segmento);

                    pcb_upd->estado = READY;
                    int prio_alloc = 0;
                    if (strcmp(kernel_config.algoritmo_planificacion, "CMN") == 0)
                    {
                        prio_alloc = pcb_upd->prioridad;
                    }
                    pthread_mutex_lock(&m_ready);
                    list_add(colas_ready[prio_alloc], pcb_upd);
                    pthread_mutex_unlock(&m_ready);
                    sem_post(&sem_procesos_ready);
                }
                break;
            }

            case SYSCALL_MEM_FREE:
            {
                int id_segmento;
                recv(socket_cliente, &id_segmento, sizeof(int), MSG_WAITALL);

                log_info(logger, "## (%d) Solicitó syscall: MEM_FREE - ID: %d", pcb_upd->pid, id_segmento);
                pthread_mutex_lock(&m_fd_memoria);
                // 1. Le avisamos a la Memoria que destruya el segmento
                op_code op_memoria = SYSCALL_MEM_FREE;
                send(fd_memoria, &op_memoria, sizeof(op_code), 0);
                send(fd_memoria, &(pcb_upd->pid), sizeof(uint32_t), 0);
                send(fd_memoria, &id_segmento, sizeof(int), 0);

                // 2. Esperamos la confirmación (OK) de la memoria
                int confirmacion;
              recv_memoria(&confirmacion, sizeof(int));
                pthread_mutex_unlock(&m_fd_memoria);
                // 3. Buscamos el segmento en la tabla del PCB y lo borramos
                for (int i = 0; i < list_size(pcb_upd->tabla_segmentos); i++)
                {
                    t_segmento *seg = list_get(pcb_upd->tabla_segmentos, i);
                    if (seg->id == id_segmento)
                    {
                        list_remove(pcb_upd->tabla_segmentos, i);
                        free(seg); // Liberamos la memoria física del struct
                        break;
                    }
                }

                // El proceso vuelve a READY, el planificador lo despacha
                pcb_upd->estado = READY;
                int prio_alloc = 0;
                if (strcmp(kernel_config.algoritmo_planificacion, "CMN") == 0)
                {
                    prio_alloc = pcb_upd->prioridad;
                }
                pthread_mutex_lock(&m_ready);
                list_add(colas_ready[prio_alloc], pcb_upd);
                pthread_mutex_unlock(&m_ready);
                sem_post(&sem_procesos_ready);
                break;
            }

            default:

                log_error(logger, "CUIDADO: Llegó un motivo desconocido desde la CPU. Código: %d", cod_op);

                break; // si no es ninguno de los anteriores casos sale del switch y sigue con el codigo que esta abajo
            }
        }
    }
    else
    {

        // si no es la CPU es una de las IOs
        log_info(logger, "## Interfaz de IO %s conectada", id_recibida);

        // la anotamos en el diccionario usando su nombre como Clave y su socket como valor
        dictionary_put(dic_interfaces, id_recibida, (void *)(intptr_t)socket_cliente);

        while (scheduler_corriendo)
        {

            // Usamos socket_cliente y no fd_io como antes
            op_code cod_op = recibir_operacion(socket_cliente);

            if (cod_op == -1)
            {

                log_warning(logger, "Se desconecto la IO %s", id_recibida);
                dictionary_remove(dic_interfaces, id_recibida); // como hubo error borramos esa IO del dicccionario
                break;
            }
            if (cod_op == MENSAJE)
            {
                char *resp = recibir_mensaje(socket_cliente);

                if (strcmp(resp, "FIN_IO") == 0)
                {
                    int pid_fin;
                    recv(socket_cliente, &pid_fin, sizeof(int), MSG_WAITALL);

                    t_pcb* pcb_fin = sacar_de_block(pid_fin);

                    if(pcb_fin != NULL) {
                        log_info(logger, "## (%d) finalizo IO y paso a READY", pid_fin);
                        encolar_en_ready(pcb_fin);
                    } else {
                        pthread_mutex_lock(&m_susp);
                        int found = 0;
                        for(int i = 0; i < list_size(cola_susp_block); i++) {
                            t_pcb* p = list_get(cola_susp_block, i);
                            if(p->pid == pid_fin) {
                                list_remove(cola_susp_block, i);
                                p->estado = SUSP_READY;
                                list_add(cola_susp_ready, p);
                                log_info(logger, "## (%d) finalizo IO y paso a SUSP_READY", pid_fin);
                                found = 1;
                                break;
                            }
                        }
                        if(!found) {
                            // Proceso en limbo: timer lo sacó de block pero
                            // todavía está escribiendo a SWAP. Dejamos flag.
                            pending_wakeup[pid_fin] = 1;
                            log_info(logger, "## (%d) FIN_IO pendiente (suspensión en curso)", pid_fin);
                        }
                        pthread_mutex_unlock(&m_susp);
                        if(found) {
                            intentar_desuspender();
                            reintentar_esperando_memoria();
                        }
                    }
                }
                free(resp);
            }
            if (cod_op == SYSCALL_STDIN)
            {
                int pid_fin, dir_fisica, tam_buffer;

                recv(socket_cliente, &pid_fin, sizeof(int), MSG_WAITALL);
                recv(socket_cliente, &dir_fisica, sizeof(int), MSG_WAITALL);
                recv(socket_cliente, &tam_buffer, sizeof(int), MSG_WAITALL);

                void *buffer_leido = malloc(tam_buffer);
                recv(socket_cliente, buffer_leido, tam_buffer, MSG_WAITALL);

                // Remoción atómica ANTES de decidir: si lo sacamos de cola_block,
                // el proceso sigue en memoria y podemos escribir. Si el timer ganó
                // la carrera, sacar_de_block devuelve NULL y sus datos ya están en
                // SWAP, así que NO escribimos.
                t_pcb *pcb_stdin = sacar_de_block(pid_fin);

                if (pcb_stdin != NULL)
                {
                    // Proceso en memoria: escribimos directo y lo despertamos
                    pthread_mutex_lock(&m_fd_memoria);
                    op_code op_mem = ESCRIBIR_MEMORIA;
                    send(fd_memoria, &op_mem, sizeof(op_code), 0);
                    send(fd_memoria, &dir_fisica, sizeof(int), 0);
                    send(fd_memoria, &tam_buffer, sizeof(int), 0);
                    send(fd_memoria, buffer_leido, tam_buffer, 0);
                    int confirmacion;
                  recv_memoria(&confirmacion, sizeof(int));
                    pthread_mutex_unlock(&m_fd_memoria);
                    free(buffer_leido);

                    log_info(logger, "## (%d) finalizo IO (STDIN) y paso a READY", pid_fin);
                    encolar_en_ready(pcb_stdin);
           } else {
                    // Proceso suspendido o en limbo: NO escribimos
                    free(buffer_leido);

                    pthread_mutex_lock(&m_susp);
                    int found = 0;
                    for(int i = 0; i < list_size(cola_susp_block); i++) {
                        t_pcb* p = list_get(cola_susp_block, i);
                        if(p->pid == pid_fin) {
                            list_remove(cola_susp_block, i);
                            p->estado = SUSP_READY;
                            list_add(cola_susp_ready, p);
                            log_info(logger, "## (%d) finalizo IO (STDIN) y paso a SUSP_READY", pid_fin);
                            found = 1;
                            break;
                        }
                    }
                    if(!found) {
                        pending_wakeup[pid_fin] = 1;
                        log_info(logger, "## (%d) STDIN pendiente (suspensión en curso)", pid_fin);
                    }
                    pthread_mutex_unlock(&m_susp);
                    if(found) {
                        intentar_desuspender();
                        reintentar_esperando_memoria();
                    }
                }
        }
    }
    }
    free(id_recibida);
    return NULL;
}
