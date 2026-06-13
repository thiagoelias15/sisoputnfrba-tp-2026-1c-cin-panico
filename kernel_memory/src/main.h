#ifndef MEMORIA_MAIN_H_
#define MEMORIA_MAIN_H_

#include <utils/utils.h>
#include <commons/log.h>
#include <pthread.h>
#include "config/config.h"
#include "conexiones/conexiones.h"

extern t_log* logger;
extern t_list* tabla_segmentos_global;
extern pthread_mutex_t m_memoria;
extern void* espacio_memoria_real;

#endif