#ifndef MAIN_H_
#define MAIN_H_

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
extern t_list* cola_ready;
extern t_list* cola_exec;
extern t_list* cola_block;
extern t_list* cola_exit;

extern pthread_mutex_t m_ready, m_new, m_block, m_exit;
extern sem_t sem_procesos_ready;
extern t_dictionary* dic_mutex;
extern t_dictionary* dic_interfaces;

extern int fd_cpu;
extern int fd_memoria;

extern char* algoritmo_planificacion;
extern int quantum_rr;
extern t_log* logger;
extern int scheduler_corriendo;

// Firmas de funciones
void* planificador_corto_plazo(void* arg);
void* temporizador_quantum(void* arg);
void* atender_cliente(void* arg);
void mover_a_ready(int pid_buscado);

#endif
