#include "conexiones.h"
#include <stdbool.h>
extern t_pcb *pcb_en_ejecucion;

bool comparar_prioridades(void *pcb1, void *pcb2)
{
    t_pcb *p1 = (t_pcb *)pcb1;
    t_pcb *p2 = (t_pcb *)pcb2;

    // numero menor = mayor prioridad
    // retorna True si p1 debe ir antes que p2 en la cola ready
    return p1->prioridad < p2->prioridad;
}

void* timer_suspension(void* arg) {
    int pid = *(int*)arg;
    free(arg);

    usleep(kernel_config.suspension_timeout * 1000);

    pthread_mutex_lock(&m_block);
    t_pcb* encontrado = NULL;
    int indice = -1;
    for(int i = 0; i < list_size(cola_block); i++) {
        t_pcb* pcb = list_get(cola_block, i);
        if(pcb->pid == pid) {
            encontrado = pcb;
            indice = i;
            break;
        }
    }

    if(encontrado != NULL) {
        list_remove(cola_block, indice);
        pthread_mutex_unlock(&m_block);

        encontrado->estado = SUSP_BLOCK;
        log_info(logger, "## (%d) Pasa del estado BLOCK al estado SUSP_BLOCK", pid);

        op_code op = SWAP_ESCRITURA;
        send(fd_memoria, &op, sizeof(op_code), 0);
        send(fd_memoria, &pid, sizeof(int), 0);
        int confirmacion;
        recv(fd_memoria, &confirmacion, sizeof(int), MSG_WAITALL);
        log_info(logger, "## (%d) Segmentos movidos a SWAP", pid);

        pthread_mutex_lock(&m_susp);
        list_add(cola_susp_block, encontrado);
        pthread_mutex_unlock(&m_susp);
    } else {
        pthread_mutex_unlock(&m_block);
    }

    return NULL;
}
void intentar_desuspender() {
    pthread_mutex_lock(&m_susp);

    list_sort(cola_susp_ready, comparar_prioridades);

    for(int i = 0; i < list_size(cola_susp_ready); i++) {
        t_pcb* pcb = list_get(cola_susp_ready, i);

        op_code op = SWAP_LECTURA;
        send(fd_memoria, &op, sizeof(op_code), 0);
        send(fd_memoria, &(pcb->pid), sizeof(int), 0);

        int resultado;
        recv(fd_memoria, &resultado, sizeof(int), MSG_WAITALL);

        if(resultado == 1) {
            list_remove(cola_susp_ready, i);
            i--;

            pcb->estado = READY;
            log_info(logger, "## (%d) Pasa del estado SUSP_READY al estado READY", pcb->pid);

            int prio = 0;
            if(strcmp(kernel_config.algoritmo_planificacion, "CMN") == 0) {
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

        fd_cpu = socket_cliente;
        log_info(logger, "## CPU <ID CPU> conectada");

        while (scheduler_corriendo)
        {

            op_code cod_op = recibir_operacion(socket_cliente);
            if (cod_op == -1)
                break;
            t_pcb *pcb_upd = recibir_pcb(fd_cpu);
            pcb_en_ejecucion = NULL;
            sem_post(&sem_cpu_libre);

            switch (cod_op)
            {

            case SYSCALL_EXIT:
            {

                log_info(logger, "## (%d) Solicito syscall: EXIT", pcb_upd->pid);
                log_info(logger, "## (%d) Pasa del estado EXEC al estado EXIT", pcb_upd->pid);
                
                //avisarle a la memory que libere los segmentos de ese proceso
                op_code op_exit = SYSCALL_EXIT;
                send(fd_memoria, &op_exit, sizeof(op_code), 0);
                send(fd_memoria, &(pcb_upd->pid), sizeof(int), 0);
                int confirmacion;
                recv(fd_memoria, &confirmacion, sizeof(int), MSG_WAITALL);
                log_info(logger, "## (%d) finalizó su ejecución", pcb_upd->pid);
                list_add(cola_exit, pcb_upd);
                sem_post(&sem_procesos_ready); // La CPU queda libre
                intentar_desuspender(); // Intentamos desuspender procesos si es posible
                break;
            }

            case SEG_FAULT:
            {
                log_info(logger, "## (%d) Pasa del estado EXEC al estado EXIT", pcb_upd->pid);

                //avisamos a la memory que libere los segmentos de ese proceso
                op_code op_exit = SYSCALL_EXIT;
                send(fd_memoria, &op_exit, sizeof(op_code), 0);
                send(fd_memoria, &(pcb_upd->pid), sizeof(int), 0);
                int confirmacion;
                recv(fd_memoria, &confirmacion, sizeof(int), MSG_WAITALL);
                log_info(logger, "## (%d) finalizó su ejecución por SEG_FAULT", pcb_upd->pid);

                // lo matamos
                list_add(cola_exit, pcb_upd);

                // Liberamos la CPU
                sem_post(&sem_procesos_ready);
                intentar_desuspender();
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
                recv(fd_cpu, &tiempo_ms, sizeof(int), MSG_WAITALL);
                log_info(logger, "## (%d) Solicito syscall: SLEEP", pcb_upd->pid);
                log_info(logger, "## (%d) Pasa del estado EXEC al estado BLOCK", pcb_upd->pid);
                pthread_mutex_lock(&m_block);
                list_add(cola_block, pcb_upd);
                pthread_mutex_unlock(&m_block);
                // Lanzar timer de suspensión
                int* pid_timer = malloc(sizeof(int));
                *pid_timer = pcb_upd->pid;
                pthread_t hilo_susp;
                pthread_create(&hilo_susp, NULL, timer_suspension, pid_timer);
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

                char *m_name = recibir_mensaje(fd_cpu); // recibe el nombre que mande la cpu
                if (!dictionary_has_key(dic_mutex, m_name))
                { // se fija si ya existe ese nombre
                    // Creamos la nueva estructura del pcb del main.h
                    t_mutex *nuevo_mutex = malloc(sizeof(t_mutex));
                    nuevo_mutex->owner = NULL; // nadie lo tiene todavia
                    nuevo_mutex->bloqueados = queue_create();

                    dictionary_put(dic_mutex, m_name, nuevo_mutex);
                }
                enviar_pcb(pcb_upd, fd_cpu, CONTEXTO_PCB);
                free(m_name);
                break;
            }

            case SYSCALL_MUTEX_LOCK:
            {

                char *m_name = recibir_mensaje(fd_cpu);
                t_mutex *mutex_actual = dictionary_get(dic_mutex, m_name);

                if (mutex_actual->owner == NULL)
                {
                    // Si el mutex está libre
                    log_info(logger, "## (%d) Toma el mutex %s", pcb_upd->pid, m_name);
                    mutex_actual->owner = pcb_upd;
                    enviar_pcb(pcb_upd, fd_cpu, CONTEXTO_PCB);
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
                int* pid_timer = malloc(sizeof(int));
                *pid_timer = pcb_upd->pid;
                pthread_t hilo_susp;
                pthread_create(&hilo_susp, NULL, timer_suspension, pid_timer);
                pthread_detach(hilo_susp);
                    queue_push(mutex_actual->bloqueados, pcb_upd);

                    // Herencia de prioridades
                    if (pcb_upd->prioridad < mutex_actual->owner->prioridad)
                    {
                        log_info(logger, "## (%d) hereda prioridad %d a (%d)", pcb_upd->pid, pcb_upd->prioridad, mutex_actual->owner->pid);

                        // Cambiamos la prioridad del dueño
                        mutex_actual->owner->prioridad = pcb_upd->prioridad;

                        pthread_mutex_lock(&m_ready);
                        for (int i = 0; i < cantidad_colas; i++)
                        {
                            if (colas_ready[i] != NULL && !list_is_empty(colas_ready[i]))
                            {
                                list_sort(colas_ready[i], comparar_prioridades);
                            }
                        }
                        pthread_mutex_unlock(&m_ready);
                    }
                }

                free(m_name);
                break;
            }

            case SYSCALL_MUTEX_UNLOCK:
            {

                char *m_name = recibir_mensaje(fd_cpu);
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
                enviar_pcb(pcb_upd, fd_cpu, CONTEXTO_PCB); // el proceso que solto el mutex vuelve a CPU para ejectuar la instruccion que sigue
                free(m_name);
                break;
            }

            case SYSCALL_STDIN:
            {

                int tam, dir, pid_sobrante; // Recibimos de la CPU los parámetros necesarios: tamaño y dirección física
                // Recibimos de forma bloqueante el tamaño del buffer que STDIN debe leer
                // MSG_WAITALL asegura que no continúe hasta recibir los 4 bytes del int
                recv(fd_cpu, &tam, sizeof(int), MSG_WAITALL);
                // Recibimos la dirección física de memoria donde se debe escribir lo ingresado
                // Esta información la envía la CPU tras traducir la dirección lógica
                recv(fd_cpu, &dir, sizeof(int), MSG_WAITALL);
                recv(fd_cpu, &pid_sobrante, sizeof(int), MSG_WAITALL);
                log_info(logger, "##(%d) Solicitó syscall: STDIN", pcb_upd->pid);
                log_info(logger, "##(%d) Pasa del estado EXEC al estado BLOCK", pcb_upd->pid);
                pthread_mutex_lock(&m_block);   // Bloquea acceso a cola de bloqueados
                list_add(cola_block, pcb_upd);  // Agrega el proceso a la cola BLOCK
                pthread_mutex_unlock(&m_block); // Libera el mutex de la cola
                // Lanzar timer de suspensión
                int* pid_timer = malloc(sizeof(int));
                *pid_timer = pcb_upd->pid;
                pthread_t hilo_susp;
                pthread_create(&hilo_susp, NULL, timer_suspension, pid_timer);
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
                recv(fd_cpu, &tam, sizeof(int), MSG_WAITALL); // Recibe tamaño a mostrar desde CPU
                recv(fd_cpu, &dir, sizeof(int), MSG_WAITALL); // Recibe dirección física de orígen
                recv(fd_cpu, &pid_sobrante, sizeof(int), MSG_WAITALL);
                log_info(logger, "##(%d) Solicitó syscall: STDOUT", pcb_upd->pid);
                log_info(logger, "##(%d) Pasa del estado EXEC al estado BLOCK", pcb_upd->pid); // Como toda operación de I/O es lenta, el proceso no puede seguir en la CPU. Se lo mueve de EXEC a BLOCK.
                pthread_mutex_lock(&m_block);                                                  // Protege la cola de bloqueados
                list_add(cola_block, pcb_upd);                                                 // Mueve el PCB a estado bloqueado
                pthread_mutex_unlock(&m_block);                                                // Libera la protección de la cola
                // Lanzar timer de suspensión
                int* pid_timer = malloc(sizeof(int));
                *pid_timer = pcb_upd->pid;
                pthread_t hilo_susp;
                pthread_create(&hilo_susp, NULL, timer_suspension, pid_timer);
                pthread_detach(hilo_susp);
                // 1. EL SCHEDULER LE PIDE LA INFO A MEMORIA
                op_code op_leer = LEER_MEMORIA;
                send(fd_memoria, &op_leer, sizeof(op_code), 0);
                send(fd_memoria, &dir, sizeof(int), 0);
                send(fd_memoria, &tam, sizeof(int), 0);

                // Preparamos un buffer y recibimos el texto
                char *texto_de_memoria = calloc(tam + 1, sizeof(char)); // +1 para el '\0'
                recv(fd_memoria, texto_de_memoria, tam, MSG_WAITALL);

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
                recv(fd_cpu, &id_segmento, sizeof(int), MSG_WAITALL);
                recv(fd_cpu, &tam_segmento, sizeof(int), MSG_WAITALL);

                log_info(logger, "## (%d) Solicitó syscall: MEM_ALLOC - ID: %d - Tam: %d", pcb_upd->pid, id_segmento, tam_segmento);

                // 1. Le mandamos la solicitud real a Kernel Memory
                op_code op_memoria = SYSCALL_MEM_ALLOC;
                send(fd_memoria, &op_memoria, sizeof(op_code), 0);
                send(fd_memoria, &(pcb_upd->pid), sizeof(uint32_t), 0);
                send(fd_memoria, &id_segmento, sizeof(int), 0);
                send(fd_memoria, &tam_segmento, sizeof(int), 0);

                //KM contesta si es -1 -> necesita compactar
                // si es >= a 0 -> no necesita
                int primer_respuesta;
                recv(fd_memoria, &primer_respuesta, sizeof(int), MSG_WAITALL);
                if(primer_respuesta == -1){
                    //KM necesita que desalojemos todas las CPUs
                    int ok = 1;
                    send(fd_memoria, &ok, sizeof(int), 0);
                    //esperamos que KM compacte
                    int fin_compactacion;
                    recv(fd_memoria, &fin_compactacion, sizeof(int), MSG_WAITALL);
                    log_info(logger," ## Fin de compactacion");
                }
                // 2. Esperamos que la Memoria haga su magia y nos devuelva la Dirección Base
                uint32_t direccion_base;
                recv(fd_memoria, &direccion_base, sizeof(uint32_t), MSG_WAITALL);

                // 3. Creamos la estructura del segmento
                if(direccion_base == 999999) {
                    log_error(logger, "## (%d) MEM_ALLOC falló - Out of Memory", pcb_upd->pid);
                } else {
                    t_segmento *nuevo_segmento = malloc(sizeof(t_segmento));
                    nuevo_segmento->id = id_segmento;
                    nuevo_segmento->tamanio = (uint32_t)tam_segmento;
                    nuevo_segmento->direccion_base = direccion_base;
                    list_add(pcb_upd->tabla_segmentos, nuevo_segmento);
                }

                // 5. El proceso vuelve a CPU directo
                 enviar_pcb(pcb_upd, fd_cpu, CONTEXTO_PCB);
                break;
            }

            case SYSCALL_MEM_FREE:
            {
                int id_segmento;
                recv(fd_cpu, &id_segmento, sizeof(int), MSG_WAITALL);

                log_info(logger, "## (%d) Solicitó syscall: MEM_FREE - ID: %d", pcb_upd->pid, id_segmento);

                // 1. Le avisamos a la Memoria que destruya el segmento
                op_code op_memoria = SYSCALL_MEM_FREE;
                send(fd_memoria, &op_memoria, sizeof(op_code), 0);
                send(fd_memoria, &(pcb_upd->pid), sizeof(uint32_t), 0);
                send(fd_memoria, &id_segmento, sizeof(int), 0);

                // 2. Esperamos la confirmación (OK) de la memoria
                int confirmacion;
                recv(fd_memoria, &confirmacion, sizeof(int), MSG_WAITALL);

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

                // 4. El proceso vuelve a CPU directo
                 enviar_pcb(pcb_upd, fd_cpu, CONTEXTO_PCB);
                intentar_desuspender();
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

                    int en_block = 0;
                    pthread_mutex_lock(&m_block);
                    for(int i = 0; i < list_size(cola_block); i++) {
                        t_pcb* p = list_get(cola_block, i);
                        if(p->pid == pid_fin) {
                            en_block = 1;
                            break;
                        }
                    }
                    pthread_mutex_unlock(&m_block);

                    if(en_block) {
                        log_info(logger, "## (%d) finalizo IO y paso a READY", pid_fin);
                        mover_a_ready(pid_fin);
                    } else {
                        pthread_mutex_lock(&m_susp);
                        for(int i = 0; i < list_size(cola_susp_block); i++) {
                            t_pcb* p = list_get(cola_susp_block, i);
                            if(p->pid == pid_fin) {
                                list_remove(cola_susp_block, i);
                                p->estado = SUSP_READY;
                                list_add(cola_susp_ready, p);
                                log_info(logger, "## (%d) finalizo IO y paso a SUSP_READY", pid_fin);
                                break;
                            }
                        }
                        pthread_mutex_unlock(&m_susp);
                    }
                }

                free(resp);
            }
            if (cod_op == SYSCALL_STDIN) // bloque para recibir la lectrua de teclado
            {
                int pid_fin, dir_fisica, tam_buffer;

                // Leemos exactamente lo que la IO nos mandó
                recv(socket_cliente, &pid_fin, sizeof(int), MSG_WAITALL);
                recv(socket_cliente, &dir_fisica, sizeof(int), MSG_WAITALL);
                recv(socket_cliente, &tam_buffer, sizeof(int), MSG_WAITALL);

                void *buffer_leido = malloc(tam_buffer);
                recv(socket_cliente, buffer_leido, tam_buffer, MSG_WAITALL);

                // 1. El Scheduler hace de cadete y guarda el texto en Memoria
                op_code op_mem = ESCRIBIR_MEMORIA;
                send(fd_memoria, &op_mem, sizeof(op_code), 0);
                send(fd_memoria, &dir_fisica, sizeof(int), 0);
                send(fd_memoria, &tam_buffer, sizeof(int), 0);
                send(fd_memoria, buffer_leido, tam_buffer, 0);

                // 2. Esperamos la confirmación (OK) de la memoria
                int confirmacion;
                recv(fd_memoria, &confirmacion, sizeof(int), MSG_WAITALL);

                free(buffer_leido);

                // 3. Despertamos al proceso para que vuelva a la CPU
                int en_block_stdin = 0;
                pthread_mutex_lock(&m_block);
                for(int i = 0; i < list_size(cola_block); i++) {
                    t_pcb* p = list_get(cola_block, i);
                    if(p->pid == pid_fin) {
                        en_block_stdin = 1;
                        break;
                    }
                }
                pthread_mutex_unlock(&m_block);

                if(en_block_stdin) {
                    log_info(logger, "## (%d) finalizo IO (STDIN) y paso a READY", pid_fin);
                    mover_a_ready(pid_fin);
                } else {
                    pthread_mutex_lock(&m_susp);
                    for(int i = 0; i < list_size(cola_susp_block); i++) {
                        t_pcb* p = list_get(cola_susp_block, i);
                        if(p->pid == pid_fin) {
                            list_remove(cola_susp_block, i);
                            p->estado = SUSP_READY;
                            list_add(cola_susp_ready, p);
                            log_info(logger, "## (%d) finalizo IO (STDIN) y paso a SUSP_READY", pid_fin);
                            break;
                        }
                    }
                    pthread_mutex_unlock(&m_susp);
                }
            }
        }
    }

    free(id_recibida);
    return NULL;
}