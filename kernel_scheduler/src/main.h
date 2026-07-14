#ifndef SCHEDULER_MAIN_H_
#define SCHEDULER_MAIN_H_

#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/queue.h>
#include <pthread.h>
#include <semaphore.h>
#include <commons/log.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <utils/utils.h>
#include "utils/serializacion/serializacion.h"
#include "config/config.h"
#include "utils/pcb/pcb.h"

// Variables globales(extern le dice al compilador que en algun lado del codigo van a estar definidas y que no les reserve ahora espacion en memoria)
extern t_list* cola_new;
extern t_list** colas_ready;
extern int cantidad_colas;
extern t_list* cola_exec;
extern t_list* cola_block;
extern t_list* cola_exit;
extern t_list* cola_susp_block;
extern t_list* cola_susp_ready;
extern pthread_mutex_t m_susp;

extern pthread_mutex_t m_ready, m_new, m_block, m_exit;
extern sem_t sem_procesos_ready;
extern t_dictionary* dic_mutex;
extern t_dictionary* dic_interfaces;

typedef struct {
    int fd_cpu;
    int pid_ejcutando;
} t_cpu_info;
extern t_lista_cpus_sched;
extern pthread_mutex_t m_cpus_sched;
extern int fd_memoria;



extern t_log* logger;
extern int scheduler_corriendo;

typedef struct {
    t_pcb* owner; // el proceso que hizo el lock y tiene el recurso
    t_queue* bloqueados; // la cola de los que estan esperando
} t_mutex;
void inicializar_estructuras(void);
void crear_proceso(char* nombre_archivo, int prioridad);
extern sem_t sem_cpu_libre;

#endif