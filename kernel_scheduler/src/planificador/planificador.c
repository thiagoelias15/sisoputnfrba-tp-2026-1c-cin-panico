#include "planificador.h"
// variable global para saber a quien desalojar
t_pcb* pcb_en_ejecucion = NULL;
//----------------------------------Planificacion de corto plazo--------------------------/

void* planificador_corto_plazo(void* arg) {

    while(scheduler_corriendo) {
        sem_wait(&sem_procesos_ready);
        sem_wait(&sem_cpu_libre);
        if (fd_cpu == -1) {
            log_warning(logger, "Esperando conexión de CPU...");
            sem_post(&sem_cpu_libre);
            sem_post(&sem_procesos_ready); // Volvemos a poner el semáforo para no trabarnos
            usleep(500000); // Esperamos medio segundo
            continue;
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

            enviar_pcb(pcb_a_ejecutar, fd_cpu, CONTEXTO_PCB);
            
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
    log_info(logger,"## (%d) Desalojo de quantum",pcb->pid);
    op_code interrupcion = INTERRUPCION;
    send(fd_cpu, &interrupcion, sizeof(op_code), 0);
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
        //metemos a la cola de READY especifica de su prioridad
        int prio;
        if(strcmp(kernel_config.algoritmo_planificacion, "CMN") == 0) {
            prio = pcb_a_mover->prioridad;
        } else {
            prio = 0; // Para FIFO y RR ignoramos la prioridad, todos van a la cola 0
        }
        pthread_mutex_lock(&m_ready);
        list_add(colas_ready[prio], pcb_a_mover);
        pthread_mutex_unlock(&m_ready);
        //desalojo o preemption
        if(kernel_config.queue_preemption == 1 && pcb_en_ejecucion != NULL){
            //si el nuevo proceso es mas importante (numero menor) que el que esta en ejecucion
         if(pcb_a_mover -> prioridad < pcb_en_ejecucion -> prioridad){
            log_info(logger, "## (%d) Prioridad: %d Desalojado por cola mas prioritaria por el proceso %d con prioridad %d",
            pcb_en_ejecucion -> pid, pcb_en_ejecucion -> prioridad, pcb_a_mover -> pid, pcb_a_mover -> prioridad);
        //le avisamos a la CPU que frene lo que esta haciendo
        op_code interrupcion = INTERRUPCION;
        send(fd_cpu, &interrupcion, sizeof(op_code), 0);
        }   
        }
    sem_post(&sem_procesos_ready); //avisamos que hay un proceso nuevo para mandar al cpu
    }
}