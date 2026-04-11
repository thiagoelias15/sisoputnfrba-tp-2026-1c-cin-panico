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
#include <commons/config.h> //libreria para leer archivos .config
//Codigos para avisarle al que recibe que tipo de paquete estamos mandando
typedef enum { /* creamos un tipo de variable con typedef(op_code) y enum asigna numeros enteros a palabras al compilar
    Mensaje=0 y Paquete=1 */
    MENSAJE,
    PAQUETE
} op_code;


//Definimos las funciones que van a usar nuestros modulos 
int iniciar_servidor(char* puerto);
int esperar_cliente(int socket_servido);
int crear_conexion(char* ip,char* puerto);
t_config* iniciar_config(char*path_config);

void enviar_mensaje(char* mensaje, int socket_cliente);
int recibir_operacion(int socket_cliente);
char* recibir_mensaje(int socket_cliente);

#endif