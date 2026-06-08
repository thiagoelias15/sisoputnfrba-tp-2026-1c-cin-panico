#ifndef MEMORIA_CORE_H_
#define MEMORIA_CORE_H_



#include <stdint.h>
#include <commons/log.h>
#include <commons/config.h>

extern t_log* logger;

void atender_fetch_cpu(int fd_cpu);
void atender_consulta_espacio(int fd_kernel);
void atender_lectura_memoria(int fd_modulo);
void atender_escritura_memoria(int fd_modulo);
void atender_creacion_proceso(int fd_kernel);
#endif