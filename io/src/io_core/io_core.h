#ifndef IO_CORE_H
#define IO_CORE_H

#include <commons/log.h>
#include "../utils/utils.h" 
#include "../utils/serializacion/serializacion.h"

// Hacemos el puente con el logger que vive en main.c
extern t_log* logger;

// Firmas de las funciones publicas
void atender_peticiones_io(int fd_scheduler);

#endif