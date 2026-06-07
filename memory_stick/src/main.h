#ifndef MS_MAIN_H_
#define MS_MAIN_H_

#include <utils/utils.h>
#include "config/config.h"
#include <commons/log.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "conexiones.h"
#include "operaciones.h"

// El puntero que va a representar nuestra memoria de hardware real
extern void* espacio_memoria; 

extern t_log* logger;
extern int tamanio_memoria;
extern int ms_corriendo;

#endif