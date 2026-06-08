#ifndef UTILS_SERIALIZACION_H_
#define UTILS_SERIALIZACION_H_

#include "utils/pcb/pcb.h"// Para usar t_pcb
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>

// Códigos para avisarle al que recibe que tipo de paquete estamos mandando

/*
Creamos una variable typedef(op_code) de tipo enum, la cual asigna números enteros a palabras al compilar.

MENSAJE = 0
PAQUETE = 1
PETICION_LECTURA = 2
DATO_MEMORIA = 3
...
*/

typedef enum {
    MENSAJE,
    PAQUETE,
    HANDSHAKE_CPU_A_MS,
    SOLICITUD_INSTRUCCION,
    CONTEXTO_PCB,
    INTERRUPCION,

    //Syscalls(peticiones de CPU a scheduler)
    SYSCALL_SLEEP,
    SYSCALL_STDIN,
    SYSCALL_STDOUT,
    SYSCALL_MUTEX_CREATE,
    SYSCALL_MUTEX_LOCK,
    SYSCALL_MUTEX_UNLOCK,
    SYSCALL_EXIT,
    SEG_FAULT,

    //Syscalls(cpu)
    FETCH_INSTRUCCION,
    SYSCALL_MEM_ALLOC,
    SYSCALL_MEM_FREE,
    SYSCALL_INIT_PROC,
    //Memoria
    CONSULTAR_ESPACIO,
    LEER_MEMORIA,
    ESCRIBIR_MEMORIA,
    SWAP_LECTURA,
    SWAP_ESCRITURA
} op_code;

// Funciones para enviar y recibir mensajes y PCBs
void enviar_mensaje(char* mensaje, op_code codigo_operacion, int socket_cliente);
char* recibir_mensaje(int socket_cliente);

//funciones para el intercambio de contextos
void enviar_pcb(t_pcb* pcb, int socket, op_code cod_op);
t_pcb* recibir_pcb(int socket);

void enviar_operacion(op_code operacion, int socket_cliente);

int recibir_operacion(int socket_cliente);

#endif 
