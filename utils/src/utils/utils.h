#ifndef UTILS_HELLO_H_
#define UTILS_HELLO_H_

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>

//Definimos las funciones que van a usar nuestros modulos 
t_config* iniciar_config(char*path_config);
int iniciar_servido(char* puerto);
int esperar_cliente(int socket_servido);
int crear_conexion(char* ip,char* puerto);

#endif