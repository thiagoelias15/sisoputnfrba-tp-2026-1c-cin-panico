#include <utils/utils.h>

t_config* iniciar_config(char*path_config){
    t_config* nuevo_config = config_create(path_config);
    if(nuevo_config == NULL){
        printf( "Error no se pudo leer el archivo %s\n", path_config);
        exit(EXIT_FAILURE); 
    }
    return nuevo_config;
}
//Funciones de servidor
int iniciar_servidor(char* puerto){
    int socket_servidor;
    struct addrinfo hints,*servinfo; //Struct que guarda tipo de conexion,IP,etc
    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_INET; //conexion IPv4
    hints.ai_socktype = SOCK_STREAM; // Sockets TCP
    hints.ai_flags = AI_PASSIVE; //Servidor // si es cliente este renglon no va
    getaddrinfo(NULL,puerto,&hints,&servinfo);

    //Crea el socket
    socket_servidor = socket(servinfo ->ai_family,servinfo->ai_socktype,servinfo->ai_protocol);

    //para evitar error "Address already in use"
    int yes = 1;
    setsockopt(socket_servidor,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));

    // Asociamos el socket a un puerto
	bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);

	// Escuchamos las conexiones entrantes
	listen(socket_servidor, SOMAXCONN); //SOMAXCONN es la cantidad maxima de conexiones que puede escuchar el server

	freeaddrinfo(servinfo);

    return socket_servidor;
}
int esperar_cliente(int socket_servidor){ //aceptamos una conexion entrante(en esta parte se bloquea hasta que el server recibe a alguien )
    int socket_cliente = accept(socket_servidor,NULL,NULL);
    return socket_cliente;
} 
 //funciones de cliente
 int crear_conexion(char* ip, char* puerto){
    struct addrinfo hints,*server_info;
    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    getaddrinfo(ip,puerto,&hints,&server_info);

    // Ahora vamos a crear el socket.
	int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
	
	// Ahora que tenemos el socket, vamos a conectarlo
	if (connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1){
        freeaddrinfo(server_info);
        return -1; //retorna -1 si falla la conexion, ponele que el server esta apagado o ya esta ocupado ese canal
    }
    freeadrinfo(server_info);
    return socket_cliente;
}

void enviar_mensaje(char* mensaje,int socket_cleinte){
    int tamaño_mensaje =strlen(mensaje)+1; //aca vemos el largo del string y +1 para que se incluya el \0 para marcar el final
    int cod_op =MENSAJE;
    int tamaño_total = sizeof(int)*2+ tamaño_mensaje; //Eel paquete pesa 4bytes (cod_op) +4 bytes(tamaño)+ el peso del texto
void* buffer =malloc(tamaño_total); // pedimos a Linux un bloque de memoria RAM vacío del tamaño exacto
int desplazamiento=0;
memcpy(buffer + desplazamiento,&cod_op,sizeof(int)); //memcpy es Memory Copy va a agarrar los bytes guardados y meterlos donde queramos
//buffer + desplazamiento es para recorrer el puntero, primero seria buffer+0 ahi se carga cod_op y avanza 4, el siguiente es buffer + 4 y asi 
send(socket_cliente,buffer,tamaño_total,0);
free(buffer); //liberamos memoria para no saturar la ram
}
int recibir_operacion(int socket_cliente){
    int cod_op:
    if(recv(socket_cliente,&cod_op,sizeof(int),MSG_WAITALL)>0){ //leemos los primero 4 bytes que llegan (el int del cod_Op)
    return cod_op;
}else{// si recv devuelve 0 o -1, significa que el cleinte del otro lado esta apagado
    close(socket_cliente);
    return -1;
}
}
char* recibir_mensaje(int socket_cliente){
    int tamaño_mensaje;
    recv(socket_cliente,&tamaño_mensaje,sizeof(int),MGS_WAITALL); //como ya leimos el cod_op antes lo siguiente son los 4 bytes del tamaño
    char* buffer = malloc(tamaño_mensaje); //sabiendo el tamaño pedimos memoria para guardar el texto
    recv(socket_cliente,buffer,tamaño_mensaje,MSG_WAITALL);// leemos el mensaje y lo guardamos
    return buffer;
}