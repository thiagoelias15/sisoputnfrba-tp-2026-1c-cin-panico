#include "conexiones.h"
#include <stdbool.h>
extern t_pcb* pcb_en_ejecucion;

bool comparar_prioridades(void* pcb1, void* pcb2){
    t_pcb* p1 = (t_pcb*)pcb1;
    t_pcb* p2 = (t_pcb*)pcb2;

    //numero menor = mayor prioridad
    //retorna True si p1 debe ir antes que p2 en la cola ready
    return p1 -> prioridad < p2 -> prioridad;
}

void* atender_cliente(void* arg) {
    
    int socket_cliente = *(int*)arg; // arg es un puntero que apunta a la direccion de memoria donde el main guardo el numero de socket y (int*)*arg le dice al compilador que trate a ese puntero como un entero
    free(arg);

    recibir_operacion(socket_cliente);
    char* id_recibida = recibir_mensaje(socket_cliente);

    if(strcmp(id_recibida, "CPU")== 0) {
        
        fd_cpu = socket_cliente;
        log_info(logger,"## CPU <ID CPU> conectada");
        
        while (scheduler_corriendo) {
            
            op_code cod_op = recibir_operacion(socket_cliente);
            if(cod_op == -1)break;
            t_pcb* pcb_upd = recibir_pcb(fd_cpu);
            pcb_en_ejecucion = NULL;

            switch (cod_op) {

                case SYSCALL_EXIT: {

                    log_info(logger, "## (%d) Solicito syscall: EXIT",pcb_upd->pid);
                    log_info(logger, "## (%d) Pasa del estado EXEC al estado EXIT", pcb_upd->pid);
                    log_info(logger, "## (%d) finalizó su ejecución", pcb_upd->pid);
                    list_add(cola_exit, pcb_upd);
                    sem_post(&sem_procesos_ready); // La CPU queda libre
                    break;
                }

                case SYSCALL_SLEEP: {

                    int tiempo_ms;
                    recv(fd_cpu,&tiempo_ms,sizeof(int),MSG_WAITALL);
                    log_info(logger, "## (%d) Solicito syscall: SLEEP", pcb_upd->pid);
                    log_info(logger, "## (%d) Pasa del estado EXEC al estado BLOCK", pcb_upd->pid);
                    pthread_mutex_lock(&m_block);
                    list_add(cola_block, pcb_upd);
                    pthread_mutex_unlock(&m_block);

                    if(dictionary_has_key(dic_interfaces, "SLEEP")) {
                        
                        // Buscamos el enchufe específico del SLEEP
                        int socket_sleep = (int)(intptr_t)dictionary_get(dic_interfaces, "SLEEP");
                        enviar_mensaje("SLEEP", SYSCALL_SLEEP, socket_sleep);
                        send(socket_sleep, &tiempo_ms, sizeof(int), 0);
                        send(socket_sleep, &(pcb_upd->pid), sizeof(int), 0);
                    } else {
                        log_error(logger, "La interfaz SLEEP no está conectada.");
                    }
                    sem_post(&sem_procesos_ready);
                    break;
                }

                case SYSCALL_MUTEX_CREATE: { // mutex create crea una cola con un nombre determinado y guarda en dictionary todas esas colas que va creando para no repetir
                
                    char* m_name = recibir_mensaje(fd_cpu); // recibe el nombre que mande la cpu
                    if(!dictionary_has_key(dic_mutex, m_name)) { // se fija si ya existe ese nombre
                       // Creamos la nueva estructura del pcb del main.h
                       t_mutex* nuevo_mutex = malloc (sizeof(t_mutex));
                       nuevo_mutex -> owner = NULL; // nadie lo tiene todavia
                       nuevo_mutex -> bloqueados = queue_create(); 

                       dictionary_put(dic_mutex, m_name, nuevo_mutex);
                    }
                    enviar_pcb(pcb_upd, fd_cpu, CONTEXTO_PCB); 
                    free(m_name);
                    break;
                }

           case SYSCALL_MUTEX_LOCK: { 

    char* m_name = recibir_mensaje(fd_cpu);
    t_mutex* mutex_actual = dictionary_get(dic_mutex, m_name);

    if(mutex_actual->owner == NULL) { 
        // Si el mutex está libre
        log_info(logger, "## (%d) Toma el mutex %s", pcb_upd->pid, m_name);
        mutex_actual->owner = pcb_upd; 
        enviar_pcb(pcb_upd, fd_cpu, CONTEXTO_PCB); 
        
    } else {
        // Mutex bloqueado: bloquea proceso
        log_info(logger, "## (%d) Pasa del estado de EXEC al estado BLOCK", pcb_upd->pid);
        
        // Se mueve a cola bloqueados
        pthread_mutex_lock(&m_block);
        list_add(cola_block, pcb_upd); 
        pthread_mutex_unlock(&m_block);
        
        queue_push(mutex_actual->bloqueados, pcb_upd); 
        
        // Herencia de prioridades
        if(pcb_upd->prioridad < mutex_actual->owner->prioridad) {
            log_info(logger, "## (%d) hereda prioridad %d a (%d)", pcb_upd->pid, pcb_upd->prioridad, mutex_actual->owner->pid);

            // Cambiamos la prioridad del dueño
            mutex_actual->owner->prioridad = pcb_upd->prioridad;
           
            pthread_mutex_lock(&m_ready);
            for(int i = 0; i < cantidad_colas; i++) {
                if(colas_ready[i] != NULL && !list_is_empty(colas_ready[i])) {
                    list_sort(colas_ready[i], comparar_prioridades);
                }
            }
            pthread_mutex_unlock(&m_ready);
        }
     
    }
    
    free(m_name);
    break;
}

                case SYSCALL_MUTEX_UNLOCK: {
                    
                    char* m_name = recibir_mensaje(fd_cpu);
                    log_info(logger,"## (%d) Libera el mutex %s",pcb_upd->pid,m_name);
                    t_mutex* mutex_actual = dictionary_get(dic_mutex,m_name);
                    // restauro la prioridad original por si se la habian cambiado
                    pcb_upd -> prioridad = pcb_upd -> prioridad_original;
                    
                    // le paso al mutex al siguiente de la fila(si hay alguno)
                    if(!queue_is_empty(mutex_actual-> bloqueados)) { 
                        
                        t_pcb* proximo = queue_pop(mutex_actual -> bloqueados); // queue_pop saca al primero elemento de bloqueados y lo devuelve para que se pueda usar y el segundo pasa a la primera posicion
                        mutex_actual -> owner = proximo; // el nuevo owner es el que estaba esperando
                        mover_a_ready(proximo->pid); // busca ese siguiente proceso en la colaa de BLOCK y lo pasa a READY
                    }else{
                        mutex_actual -> owner = NULL; // Nadie lo estaba esperando, queda libre
                    }
                    enviar_pcb(pcb_upd,fd_cpu, CONTEXTO_PCB); // el proceso que solto el mutex vuelve a CPU para ejectuar la instruccion que sigue
                    free(m_name);
                    break;
                }

                case SYSCALL_STDIN: {

                    int tam, dir; // Recibimos de la CPU los parámetros necesarios: tamaño y dirección física
                    // Recibimos de forma bloqueante el tamaño del buffer que STDIN debe leer
                    // MSG_WAITALL asegura que no continúe hasta recibir los 4 bytes del int
                    recv(fd_cpu, &tam, sizeof(int), MSG_WAITALL);
                    // Recibimos la dirección física de memoria donde se debe escribir lo ingresado
                    // Esta información la envía la CPU tras traducir la dirección lógica
                    recv(fd_cpu, &dir, sizeof(int), MSG_WAITALL);
                    log_info(logger, "##(%d) Solicitó syscall: STDIN", pcb_upd->pid);
                    log_info(logger, "##(%d) Pasa del estado EXEC al estado BLOCK", pcb_upd->pid);
                    pthread_mutex_lock(&m_block); // Bloquea acceso a cola de bloqueados
                    list_add(cola_block, pcb_upd); // Agrega el proceso a la cola BLOCK
                    pthread_mutex_unlock(&m_block); // Libera el mutex de la cola
                
                    if(dictionary_has_key(dic_interfaces, "STDIN")) {

                        int socket_stdin = (int)(intptr_t)dictionary_get(dic_interfaces, "STDIN");
                        enviar_mensaje("STDIN", SYSCALL_STDIN, socket_stdin);
                        send(socket_stdin, &tam, sizeof(int), 0);
                        send(socket_stdin, &dir, sizeof(int), 0);
                        send(socket_stdin, &(pcb_upd->pid), sizeof(int), 0);
                    } else {
                        log_error(logger, "La interfaz STDIN no está conectada.");
                    }

                    sem_post(&sem_procesos_ready);
                    break;
                }

                case SYSCALL_STDOUT: {
                    
                    int tam, dir;
                    recv(fd_cpu, &tam, sizeof(int), MSG_WAITALL); // Recibe tamaño a mostrar desde CPU
                    recv(fd_cpu, &dir, sizeof(int), MSG_WAITALL); //Recibe dirección física de orígen
                    log_info(logger, "##(%d) Solicitó syscall: STDOUT", pcb_upd->pid);
                    log_info (logger,"##(%d) Pasa del estado EXEC al estado BLOCK", pcb_upd->pid); // Como toda operación de I/O es lenta, el proceso no puede seguir en la CPU. Se lo mueve de EXEC a BLOCK.
                    pthread_mutex_lock(&m_block); // Protege la cola de bloqueados
                    list_add(cola_block, pcb_upd); // Mueve el PCB a estado bloqueado
                    pthread_mutex_unlock(&m_block); // Libera la protección de la cola
                    
                    /*Si hay una interfaz conectada , el Kernel le envía un mensaje avisando que hay una tarea de STDOUT. 
                    Le pasa el tamaño, la dirección y el PID del proceso para que la interfaz sepa a quién pertenece la operación*/
                    
                    if(dictionary_has_key(dic_interfaces, "STDOUT")) {

                        int socket_stdout = (int)(intptr_t)dictionary_get(dic_interfaces, "STDOUT"); // intptr_t es de una biblioteca stdin.h y lo que hace es una variable que se asegura que el dato tenga el mismo tamaño en bytes que el puntero para que no tire error
                        enviar_mensaje("STDOUT", SYSCALL_STDOUT, socket_stdout);
                        send(socket_stdout, &tam, sizeof(int), 0);
                        send(socket_stdout, &dir, sizeof(int), 0);
                        send(socket_stdout, &(pcb_upd->pid), sizeof(int), 0);
                    } else {
                        log_error(logger, "La interfaz STDOUT no está conectada.");
                    }
                    
                    sem_post(&sem_procesos_ready);
                    break;
                }
                
                default: break; // si no es ninguno de los anteriores casos sale del switch y sigue con el codigo que esta abajo

            }
        }

    } else {

        // si no es la CPU es una de las IOs
        log_info(logger,"## Interfaz de IO %s conectada", id_recibida); 
        
        // la anotamos en el diccionario usando su nombre como Clave y su socket como valor
        dictionary_put(dic_interfaces, id_recibida, (void*)(intptr_t)socket_cliente); 

        while(scheduler_corriendo) {

            // Usamos socket_cliente y no fd_io como antes 
            op_code cod_op = recibir_operacion(socket_cliente);
            
            if(cod_op == -1) {
                
                log_warning(logger, "Se desconecto la IO %s", id_recibida);
                dictionary_remove(dic_interfaces, id_recibida); //como hubo error borramos esa IO del dicccionario
                break;
            }

            if(cod_op == MENSAJE) {
                
                char* resp = recibir_mensaje(socket_cliente);
                
                if(strcmp(resp, "FIN_IO")== 0) {
                    
                    int pid_fin;
                    recv(socket_cliente, &pid_fin, sizeof(int), MSG_WAITALL);
                    log_info(logger, "## (%d) finalizo IO y paso a READY",pid_fin);
                    mover_a_ready(pid_fin);
                }

                free(resp);  
            }
        }
    }
    
    free(id_recibida);
    return NULL;
}