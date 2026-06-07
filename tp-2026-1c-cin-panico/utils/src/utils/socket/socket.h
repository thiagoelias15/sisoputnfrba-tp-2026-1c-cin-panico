#ifndef UTILS_SOCKET_H_
#define UTILS_SOCKET_H_

// Declaraciones para manejo de sockets

int iniciar_servidor(char* puerto);
int esperar_cliente(int socket_servidor);
int crear_conexion(char* ip, char* puerto);

#endif 
