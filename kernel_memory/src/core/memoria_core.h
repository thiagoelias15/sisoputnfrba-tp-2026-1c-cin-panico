#ifndef MEMORIA_CORE_H_
#define MEMORIA_CORE_H_

#include <utils/utils.h>
#include <commons/log.h>
#include <unistd.h>
#include "../config/config.h"

extern t_log* logger;

void atender_fetch_cpu(int fd_cpu);
void atender_consulta_espacio(int fd_kernel);
void atender_lectura_memoria(int fd_modulo);
void atender_escritura_memoria(int fd_modulo);

#endif