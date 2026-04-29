#include "socket.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

int iniciar_servidor(char* puerto) {
    if (puerto != NULL) puerto[strcspn(puerto, "\r\n ;")] = 0;

    int socket_servidor;
    struct addrinfo hints, *servinfo;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, puerto, &hints, &servinfo) != 0) return -1;

    socket_servidor = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (socket_servidor == -1) {
        freeaddrinfo(servinfo);
        return -1;
    }

    int yes = 1;
    setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    if (bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        perror("Error en bind");
        close(socket_servidor);
        freeaddrinfo(servinfo);
        return -1;
    }

    freeaddrinfo(servinfo);

    if (listen(socket_servidor, SOMAXCONN) == -1) {
        perror("Error en listen");
        return -1;
    }

    return socket_servidor;
}

int esperar_cliente(int socket_servidor) {
    int socket_cliente = accept(socket_servidor, NULL, NULL);
    return socket_cliente;
}

int crear_conexion(char* ip, char* puerto) {
    struct addrinfo hints, *server_info;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(ip, puerto, &hints, &server_info) != 0) return -1;

    int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

    if (connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1) {
        freeaddrinfo(server_info);
        return -1;
    }

    freeaddrinfo(server_info);
    return socket_cliente;
}
