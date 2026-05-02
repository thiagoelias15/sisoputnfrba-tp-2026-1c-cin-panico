#ifndef INSTRUCCIONES_H_
#define INSTRUCCIONES_H_

#include <utils/utils.h>
#include <commons/string.h>
#include <commons/log.h>
#include <stdlib.h>
#include <string.h>

// Definimos la función que va a procesar el Execute
void ejecutar_instrucciones(char** tokens, t_pcb* pcb, int* desalojar, op_code* motivo, int* modifico_pc, t_log* logger);

void setear_valor_registro(t_pcb* pcb, char* reg, uint32_t valor);
uint32_t obtener_valor_registro(t_pcb* pcb, char* reg);

#endif