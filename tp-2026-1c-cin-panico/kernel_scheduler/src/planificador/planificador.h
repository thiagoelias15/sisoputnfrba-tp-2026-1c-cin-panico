#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_

#include "../main.h"

void* planificador_corto_plazo(void* arg);
void* temporizador_quantum(void* arg);
void mover_a_ready(int pid_buscado);

#endif