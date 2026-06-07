#include "planificador.h"

//----------------------------------Planificacion de corto plazo--------------------------/

void* planificador_corto_plazo(void* arg) {

    while(scheduler_corriendo) {

        sem_wait(&sem_procesos_ready);
        pthread_mutex_lock(&m_ready);
        t_pcb* pcb_a_ejecutar = list_remove(cola_ready, 0);
        pthread_mutex_unlock(&m_ready);
        
        log_info(logger, "## (%d) Pasa del estado READY al estado EXEC",pcb_a_ejecutar->pid);

        //manda el pcb al cpu a ejecutar
        enviar_pcb(pcb_a_ejecutar, fd_cpu, CONTEXTO_PCB);
        
        if(strcmp(kernel_config.algoritmo_planificacion, "RR")== 0) {
            
            pthread_t hilo_quantum;
            pthread_create(&hilo_quantum,NULL,temporizador_quantum,pcb_a_ejecutar);
            pthread_detach(hilo_quantum);
        }
    }
    return NULL;
}

void* temporizador_quantum(void* arg) {
    
    t_pcb* pcb = (t_pcb*)arg; //aca le decimos al compilador que trate a ese arg como un puntero a un pcb para leer el PID
    usleep(kernel_config.quantum_rr * 1000); //la funcion usleep espera una x cantidad de microsegundos y por mil para pasar esos microsegundos a milisegundos
    log_info(logger,"## (%d) Desalojo de quantum",pcb->pid);
    enviar_mensaje("INTERRUPCION_RR",INTERRUPCION,fd_cpu); //aca el scheduler le pide a la cpu que frene la ejecucion del procesos y se lo devuelva
    return NULL;
}

//--------------------------------- Auxiliares de planificacion---------------------------/

void mover_a_ready(int pid_buscado) { //La función entra a la "sala de espera" de procesos bloqueados (cola_block)
    t_pcb* pcb_a_mover = NULL;

    //bloqueamos la cola BLOCK para evitar "choques" o que entren nuevos hilos a la cola
    pthread_mutex_lock(&m_block);

    for(int i = 0; i< list_size(cola_block);i++) { // va a recorrer la lista buscando el PID que necesitamos
        
        t_pcb* p= list_get(cola_block,i); //obtenemos el PCB en la posicion i
        if(p->pid == pid_buscado) {

            //si coincide el PID, lo extraemos de la lista de bloqueados
            pcb_a_mover = list_remove(cola_block,i);
            break;
        }
    }

    pthread_mutex_unlock(&m_block);

    if(pcb_a_mover != NULL) { //si encontro el proceso lo mete en la cola de listos
        
        pcb_a_mover->estado = READY; //actualizamods el estado interno del PCB
        //metemos a la cola de READY
        pthread_mutex_lock(&m_ready);
        list_add(cola_ready, pcb_a_mover);
        pthread_mutex_unlock(&m_ready);
        sem_post(&sem_procesos_ready); //avisamos que hay un proceso nuevo para mandar al cpu
    }
}