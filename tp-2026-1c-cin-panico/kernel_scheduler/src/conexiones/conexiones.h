#ifndef CONEXIONES_SCHEDULER_H_
#define CONEXIONES_SCHEDULER_H_

#include "../main.h"
#include "../planificador/planificador.h" // Para poder llamar a mover_a_ready()

void* atender_cliente(void* arg);

#endif