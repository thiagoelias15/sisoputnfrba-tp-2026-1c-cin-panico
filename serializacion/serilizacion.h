#ifndef SERIALIZACION_H_
#define SERIALIZACION_H_

#include "pcb.h" // Para usar t_pcb
#include <stdint.h>

typedef enum {
    MENSAJE,
    PAQUETE,
    HANDSHAKE_CPU_A_MS,
    SOLICITUD_INSTRUCCION,
    CONTEXTO_PCB,
    INTERRUPCION,
    SYSCALL_SLEEP,
    SYSCALL_STDIN,
    SYSCALL_STDOUT,
    SYSCALL_MUTEX_CREATE,
    SYSCALL_MUTEX_LOCK,
    SYSCALL_MUTEX_UNLOCK,
    SYSCALL_EXIT
} op_code;

// Funciones para enviar y recibir mensajes y PCBs

void enviar_mensaje(char* mensaje, op_code codigo_operacion, int socket_cliente);
char* recibir_mensaje(int socket_cliente);

void enviar_pcb(t_pcb* pcb, int socket, op_code cod_op);
t_pcb* recibir_pcb(int socket);

int recibir_operacion(int socket_cliente);

#endif // SERIALIZACION_H_
