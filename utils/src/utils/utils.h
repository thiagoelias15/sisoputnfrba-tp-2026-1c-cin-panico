#ifndef UTILS_HELLO_H_
#define UTILS_HELLO_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/config.h> // Libreria para leer archivos .config

// Códigos para avisarle al que recibe que tipo de paquete estamos mandando

/*
    Creamos una variable typedef(op_code) de tipo enum, la cual asigna números enteros a palabras al compilar.

    MENSAJE = 0
    PAQUETE = 1
    PETICION_LECTURA = 2
    DATO_MEMORIA = 3
*/

typedef enum { 
    MENSAJE,
    PAQUETE,
    HANDSHAKE_CPU_A_MS,
} op_code;


// Se definen las funciones a utilizar en los módulos

int iniciar_servidor(char* puerto);
int esperar_cliente(int socket_servido);
int crear_conexion(char* ip,char* puerto);
t_config* iniciar_config(char*path_config);

void enviar_mensaje(char* mensaje, op_code codigo_operacion, int socket_cliente);
int recibir_operacion(int socket_cliente);
char* recibir_mensaje(int socket_cliente);

#endif