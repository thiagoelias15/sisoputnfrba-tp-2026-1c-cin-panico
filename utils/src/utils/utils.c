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

	log_trace(logger, "Listo para escuchar a mi cliente");

    return socket_servidor;
}
int esperar_cliente(int socket_servidor){ //aceptamos una conexion entrante(en esta parte se bloquea hasta que el server recibe a alguien )
    int socket_cliente = accept(socket_servidor,NULL,NULL);
    log_info(logger, "Se conecto un cliente!");
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

