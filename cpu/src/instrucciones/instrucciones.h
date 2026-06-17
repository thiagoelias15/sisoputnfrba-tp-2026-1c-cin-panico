#ifndef INSTRUCCIONES_H_
#define INSTRUCCIONES_H_

#include <utils/utils.h>
#include <commons/string.h>
#include <commons/log.h>
#include <stdlib.h>
#include <string.h>

// Definimos la función que va a procesar el Execute
void ejecutar_instrucciones(char** tokens, t_pcb* pcb, int* desalojar, op_code* motivo, int* modifico_pc, t_log* logger, int fd_memoria);

void setear_valor_registro(t_pcb* pcb, char* reg, uint32_t valor);
uint32_t obtener_valor_registro(t_pcb* pcb, char* reg);
int traducir_direccion_mmu(uint32_t dir_logica, uint32_t tam_a_leer_escribir, t_pcb* pcb);

#endif