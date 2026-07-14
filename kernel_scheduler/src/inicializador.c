#include "main.h"
#include <utils/pcb/pcb.h>
extern int PID_GLOBAL;
void inicializar_estructuras(void) {
    cola_new = list_create();
    
    // Si es Colas Multinivel, contamos cuántas pide el array
    if(strcmp(kernel_config.algoritmo_planificacion, "CMN") == 0) {
        cantidad_colas = 0;
        // Agregamos chequeo de NULL por si el array no vino en el config
        while(kernel_config.algoritmos_colas != NULL && kernel_config.algoritmos_colas[cantidad_colas] != NULL) {
            cantidad_colas++;
        }
    } else {
        // Si es FIFO o RR, solo necesitamos 1 cola global
        cantidad_colas = 1; 
    }

    colas_ready = malloc(sizeof(t_list*) * cantidad_colas);
    for(int i = 0; i < cantidad_colas; i++) {
        colas_ready[i] = list_create();
    }
    sem_init(&sem_cpu_libre, 0 ,1);
    cola_exec = list_create();
    cola_block = list_create();
    cola_exit = list_create();
    cola_susp_block = list_create();
    cola_susp_ready = list_create();
    pthread_mutex_init(&m_susp, NULL);
    dic_mutex = dictionary_create();
    dic_interfaces = dictionary_create();

    pthread_mutex_init(&m_ready, NULL);
    pthread_mutex_init(&m_new, NULL);
    pthread_mutex_init(&m_block, NULL);
    pthread_mutex_init(&m_exit, NULL);
    sem_init(&sem_procesos_ready, 0, 0);
    lista_cpus_sched = list_create();
    pthread_mutex_init(&m_cpus_sched, NULL);
}
void crear_proceso(char* nombre_archivo, int prioridad) {
    int pid_nuevo = PID_GLOBAL;
    PID_GLOBAL++;

    // ------------------- ENVIAR MENSAJE DE CREACIÓN A LA MEMORIA ------------------- //
    op_code cop = SYSCALL_INIT_PROC;
    uint32_t tam_nombre = strlen(nombre_archivo) + 1;

    // Enviamos: Código de operación, el PID y el tamaño del string seguido del string del archivo
    send(fd_memoria, &cop, sizeof(op_code), 0);
    send(fd_memoria, &pid_nuevo, sizeof(int), 0);
    send(fd_memoria, &tam_nombre, sizeof(uint32_t), 0);
    send(fd_memoria, nombre_archivo, tam_nombre, 0);

    // Esperamos la confirmación (OK) de la Memoria para avanzar seguros
    int respuesta_memoria;
    recv(fd_memoria, &respuesta_memoria, sizeof(int), MSG_WAITALL);
    // ------------------------------------------------------------------------------- //
    t_pcb* pcb_nuevo = pcb_create();
    pcb_nuevo->pid = pid_nuevo;
    pcb_nuevo->pc = 0;
    pcb_nuevo->prioridad = prioridad;
    pcb_nuevo->prioridad_original = prioridad;
    
    log_info(logger, "## (%d) Se crea el proceso - Estado: NEW", pcb_nuevo->pid);
    
    pthread_mutex_lock(&m_new);
    list_add(cola_new, pcb_nuevo);
   
    int index_ultimo = list_size(cola_new) -1;
    t_pcb* pcb_a_ready = list_remove(cola_new, index_ultimo);
    pthread_mutex_unlock(&m_new);

    log_info(logger, "## (%d) Pasa del estado NEW a READY", pcb_a_ready->pid); 

    int prio = 0;
    if(strcmp(kernel_config.algoritmo_planificacion, "CMN")== 0){
        prio = pcb_a_ready -> prioridad;
    }
    
    pthread_mutex_lock(&m_ready);
    list_add(colas_ready[prio], pcb_a_ready);
    pthread_mutex_unlock(&m_ready);
    
    sem_post(&sem_procesos_ready);
}