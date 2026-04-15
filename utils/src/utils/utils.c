#include "utils.h"

t_config* iniciar_config(char* path_config) {

    t_config* nuevo_config = config_create(path_config);

    if (nuevo_config == NULL) {

        printf("Error no se pudo leer el archivo %s\n", path_config);
        exit(EXIT_FAILURE);
    }
    
    return nuevo_config;
}

// ------------------------------ FUNCIONES DE SERVIDOR ------------------------------ //

int iniciar_servidor(char* puerto) {

    if (puerto != NULL) {

        // Borra de los archivos de texto (configs) si quedo un espacio,\n, o caracteres invisibles al final de la linea de la terminal
        puerto[strcspn(puerto, "\r\n ;")] = 0;
    }

    int socket_servidor;
    struct addrinfo hints, *servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, puerto, &hints, &servinfo) != 0) {
        return -1;
    }

    // 1. CREAR EL SOCKET 
    socket_servidor = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

    if (socket_servidor == -1) {

        freeaddrinfo(servinfo);
        return -1;
    }

    // 2. Se elimina el tiempo de espera para el siguiente BIND al interrumpir una conexión con el comando ^C
    int yes = 1;
    setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    // 3. ASOCIAR AL PUERTO (Bind)
    if (bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        
        perror("Error en bind");
        close(socket_servidor);
        freeaddrinfo(servinfo);
        return -1;
    }

    freeaddrinfo(servinfo);

    // 4. ESCUCHAR
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

// ------------------------------ FUNCIONES DE CLIENTE ------------------------------ //

int crear_conexion(char* ip, char* puerto) {
    
    struct addrinfo hints, *server_info;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(ip, puerto, &hints, &server_info) != 0) {
        return -1;
    }

    int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

    if (connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1) {

        freeaddrinfo(server_info);
        return -1;
    }

    freeaddrinfo(server_info);
    return socket_cliente;
}

void enviar_mensaje(char* mensaje, op_code codigo_operacion, int socket_cliente) {

    int tamaño_mensaje = strlen(mensaje) + 1;
    int cod_op = codigo_operacion; 
    int tamaño_total = sizeof(int) * 2 + tamaño_mensaje;
    void* buffer = malloc(tamaño_total);
    int desplazamiento = 0;

    memcpy(buffer + desplazamiento, &cod_op, sizeof(int));
    desplazamiento += sizeof(int);

    memcpy(buffer + desplazamiento, &tamaño_mensaje, sizeof(int));
    desplazamiento += sizeof(int);

    memcpy(buffer + desplazamiento, mensaje, tamaño_mensaje);

    send(socket_cliente, buffer, tamaño_total, 0);

    free(buffer);
}

int recibir_operacion(int socket_cliente) {

    int cod_op;

    if (recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0) {
        return cod_op;
    }
    else {
        
        close(socket_cliente);
        return -1;
    }
}

char* recibir_mensaje(int socket_cliente) {

    int tamaño_mensaje;

    if (recv(socket_cliente, &tamaño_mensaje, sizeof(int), MSG_WAITALL) != sizeof(int)) {
        return NULL;
    }

    char* buffer = malloc(tamaño_mensaje);

    if (recv(socket_cliente, buffer, tamaño_mensaje, MSG_WAITALL) != tamaño_mensaje) {
        
        free(buffer);
        return NULL;
    }
    
    return buffer;
}
