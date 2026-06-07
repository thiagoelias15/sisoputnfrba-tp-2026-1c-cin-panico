#include "main.h"

void inicializar_estructuras(void) {
    cola_new = list_create();
    cola_ready = list_create();
    cola_exec = list_create();
    cola_block = list_create();
    cola_exit = list_create();
    dic_mutex = dictionary_create();
    dic_interfaces = dictionary_create();

    pthread_mutex_init(&m_ready, NULL);
    pthread_mutex_init(&m_new, NULL);
    pthread_mutex_init(&m_block, NULL);
    pthread_mutex_init(&m_exit, NULL);
    sem_init(&sem_procesos_ready, 0, 0);
}

void crear_proceso_inicial(void) {
    t_pcb* pcb_inicial = malloc(sizeof(t_pcb));
    pcb_inicial->pid = 0;
    pcb_inicial->pc = 0;
    pcb_inicial->prioridad = 0; // Prioridad máxima por enunciado
    pcb_inicial->prioridad_original = 0;

    // Inicializamos los registros en 0
    pcb_inicial->ax = 0; pcb_inicial->bx = 0; pcb_inicial->cx = 0; pcb_inicial->dx = 0;
    pcb_inicial->eax = 0; pcb_inicial->ebx = 0; pcb_inicial->ecx = 0; pcb_inicial->edx = 0;
    pcb_inicial->si = 0; pcb_inicial->di = 0;

    log_info(logger, "## (%d) Se crea el proceso - Estado: NEW", pcb_inicial->pid);
    
    pthread_mutex_lock(&m_new);
    list_add(cola_new, pcb_inicial);
    pthread_mutex_unlock(&m_new);

    pthread_mutex_lock(&m_new);
    t_pcb* pcb_a_ready = list_remove(cola_new, 0);
    pthread_mutex_unlock(&m_new);

    log_info(logger, "## (%d) Pasa del estado NEW a READY", pcb_a_ready->pid); 

    pthread_mutex_lock(&m_ready);
    list_add(cola_ready, pcb_a_ready);
    pthread_mutex_unlock(&m_ready);
    
    sem_post(&sem_procesos_ready);
}