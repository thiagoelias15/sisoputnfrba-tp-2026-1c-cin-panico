#include "pcb.h"
#include <stdlib.h> // Necesario para malloc
#include <commons/collections/list.h> // Necesario para list_create
t_pcb* pcb_create() {
    t_pcb* pcb = malloc(sizeof(t_pcb));
    
    // Inicialización obligatoria para evitar el Segfault
    pcb->tabla_segmentos = list_create(); 
    
    // Inicialización de valores base
    pcb->pid = 0;
    pcb->pc = 0;
    pcb->estado = NEW;
    pcb->prioridad = 0;
    pcb->prioridad_original = 0;

    // Inicialización de registros
    pcb->ax = 0; pcb->bx = 0; pcb->cx = 0; pcb->dx = 0;
    pcb->eax = 0; pcb->ebx = 0; pcb->ecx = 0; pcb->edx = 0;
    pcb->si = 0; pcb->di = 0;
    
    return pcb;
}