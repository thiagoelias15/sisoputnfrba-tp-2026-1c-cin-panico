#include "planificador.h"
// variable global para saber a quien desalojar
t_pcb* pcb_en_ejecucion = NULL;


// Busca una CPU libre y devuelve su fd (-1 si no hay)
int obtener_cpu_libre() {
    int fd = -1;
    pthread_mutex_lock(&m_cpus_sched);
    for(int i = 0; i < list_size(lista_cpus_sched); i++) {
        t_cpu_info* ci = list_get(lista_cpus_sched, i);
        if(ci->pid_ejecutando == -1) {
            fd = ci->fd_cpu;
            break;
        }
    }
    pthread_mutex_unlock(&m_cpus_sched);
    return fd;
}

// Marca una CPU como ocupada por un PID
void marcar_cpu_ocupada(int fd, int pid) {
    pthread_mutex_lock(&m_cpus_sched);
    for(int i = 0; i < list_size(lista_cpus_sched); i++) {
        t_cpu_info* ci = list_get(lista_cpus_sched, i);
        if(ci->fd_cpu == fd) {
            ci->pid_ejecutando = pid;
            break;
        }
    }
    pthread_mutex_unlock(&m_cpus_sched);
}

// Busca en qué CPU corre un PID y devuelve el fd (-1 si no lo encuentra)
int buscar_fd_por_pid(int pid) {
    int fd = -1;
    pthread_mutex_lock(&m_cpus_sched);
    for(int i = 0; i < list_size(lista_cpus_sched); i++) {
        t_cpu_info* ci = list_get(lista_cpus_sched, i);
        if(ci->pid_ejecutando == pid) {
            fd = ci->fd_cpu;
            break;
        }
    }
    pthread_mutex_unlock(&m_cpus_sched);
    return fd;
}

//----------------------------------Planificacion de corto plazo--------------------------/

void* planificador_corto_plazo(void* arg) {

    while(scheduler_corriendo) {
        sem_wait(&sem_procesos_ready);
        sem_wait(&sem_cpu_libre);
         int fd_cpu_actual = obtener_cpu_libre();
        if (fd_cpu_actual == -1) {
            sem_post(&sem_procesos_ready); // devolvemos el proceso a la cola
            continue; // sem_wait(sem_cpu_libre) en la próxima iteración bloqueará naturalmente
        }
        t_pcb* pcb_a_ejecutar = NULL;

        //buscamos el proceso mas prioritario disponible
        pthread_mutex_lock(&m_ready);
        for(int i = 0; i < cantidad_colas; i++){
            if(!list_is_empty(colas_ready[i])){
                pcb_a_ejecutar = list_remove(colas_ready[i],0);
                break; // encontramos uno paramos la busqueda
            }
        }
        pthread_mutex_unlock(&m_ready);
        
        // Lo mandamos a ejecutar
        if(pcb_a_ejecutar != NULL) {
            pcb_a_ejecutar->estado = EXEC;
            pcb_en_ejecucion = pcb_a_ejecutar; // Registramos que este ocupó la CPU
            
            log_info(logger, "## (%d) Pasa del estado READY al estado EXEC", pcb_a_ejecutar->pid);

            enviar_pcb(pcb_a_ejecutar, fd_cpu_actual, CONTEXTO_PCB);
            marcar_cpu_ocupada(fd_cpu_actual, pcb_a_ejecutar->pid);
           // Evaluamos si corresponde lanzar el temporizador de Round Robin
            int usa_rr = 0; // Por defecto es 0 (Falso)
            
                if(strcmp(kernel_config.algoritmo_planificacion, "CMN") == 0) {
                // En CMN, nos fijamos qué dice el array para la cola de este proceso
                if(kernel_config.algoritmos_colas != NULL && 
                   strcmp(kernel_config.algoritmos_colas[pcb_a_ejecutar->prioridad], "RR") == 0) {
                    usa_rr = 1;
                }
            } else if(strcmp(kernel_config.algoritmo_planificacion, "RR") == 0) {
                // Si el algoritmo global es RR, lo usamos siempre
                usa_rr = 1;
            }

            // Si dió verdadero, lanzamos el hilo del Quantum
            if(usa_rr == 1) {
                pthread_t hilo_quantum;
                pthread_create(&hilo_quantum, NULL, temporizador_quantum, pcb_a_ejecutar);
                pthread_detach(hilo_quantum);
            }
        }else {
            sem_post(&sem_cpu_libre);
        }
    }
    return NULL;
}

void* temporizador_quantum(void* arg) {
    
    t_pcb* pcb = (t_pcb*)arg; //aca le decimos al compilador que trate a ese arg como un puntero a un pcb para leer el PID
    usleep(kernel_config.quantum_rr * 1000); //la funcion usleep espera una x cantidad de microsegundos y por mil para pasar esos microsegundos a milisegundos
     int fd = buscar_fd_por_pid(pcb->pid);
    if(fd != -1) {
        log_info(logger, "## (%d) Desalojo de quantum", pcb->pid);
        op_code interrupcion = INTERRUPCION;
        send(fd, &interrupcion, sizeof(op_code), 0);
    }
    return NULL;
}

//--------------------------------- Auxiliares de planificacion---------------------------/

void mover_a_ready(int pid_buscado) {
    t_pcb* pcb_a_mover = NULL;

    pthread_mutex_lock(&m_block);
    for(int i = 0; i < list_size(cola_block); i++) {
        t_pcb* p = list_get(cola_block, i);
        if(p->pid == pid_buscado) {
            pcb_a_mover = list_remove(cola_block, i);
            break;
        }
    }
    pthread_mutex_unlock(&m_block);

    if(pcb_a_mover != NULL) {
        pcb_a_mover->estado = READY;

        // Recalcular prioridad heredada: si este proceso es dueño de algún
        // mutex con procesos esperando, hereda la prioridad más alta de ellos
        int prio_heredada = pcb_a_mover->prioridad_original;
       t_list* claves = dictionary_keys(dic_mutex);
        for(int k = 0; k < list_size(claves); k++) {
            char* clave = list_get(claves, k);
            t_mutex* mtx = dictionary_get(dic_mutex, clave);
            if(mtx->owner != NULL && mtx->owner->pid == pid_buscado) {
                // Este proceso es dueño de este mutex; revisamos quién espera
                t_list* esperando = mtx->bloqueados->elements;
                for(int e = 0; e < list_size(esperando); e++) {
                    t_pcb* esperador = list_get(esperando, e);
                    if(esperador->prioridad < prio_heredada) {
                        prio_heredada = esperador->prioridad;
                    }
                }
            }
        }
        list_destroy(claves);
        pcb_a_mover->prioridad = prio_heredada;

        int prio;
        if(strcmp(kernel_config.algoritmo_planificacion, "CMN") == 0) {
            prio = pcb_a_mover->prioridad;
        } else {
            prio = 0;
        }
        pthread_mutex_lock(&m_ready);
        list_add(colas_ready[prio], pcb_a_mover);
        pthread_mutex_unlock(&m_ready);

        if(kernel_config.queue_preemption == 1) {
            if(pcb_en_ejecucion != NULL &&
               pcb_a_mover->prioridad < pcb_en_ejecucion->prioridad) {
                int fd_desalojo = buscar_fd_por_pid(pcb_en_ejecucion->pid);
                if(fd_desalojo != -1) {
                    log_info(logger, "## (%d) Prioridad: %d Desalojado por cola mas prioritaria por el proceso %d con prioridad %d",
                        pcb_en_ejecucion->pid, pcb_en_ejecucion->prioridad, pcb_a_mover->pid, pcb_a_mover->prioridad);
                    op_code interrupcion = INTERRUPCION;
                    send(fd_desalojo, &interrupcion, sizeof(op_code), 0);
                }
            }
        }
        sem_post(&sem_procesos_ready);
    }
}