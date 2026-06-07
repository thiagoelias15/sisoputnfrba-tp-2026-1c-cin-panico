#ifndef CICLO_INSTRUCCION_H_
#define CICLO_INSTRUCCION_H_

#include "main.h"
#include "../instrucciones/instrucciones.h"
#include <utils/utils.h>

// Pide la instrucción a Memoria enviando PID y PC
char* realizar_fetch(t_pcb* pcb, int fd_memoria, t_log* logger);


int hay_interrupcion(int fd_scheduler, t_log* logger);

// Empaqueta datos de Syscalls y devuelve el PCB actualizado al Scheduler
void gestionar_desalojo(t_pcb* pcb, op_code motivo, char** tokens, int fd_scheduler);

#endif