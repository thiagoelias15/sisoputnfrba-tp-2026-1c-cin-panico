#include <utils/utils.h>

int main (int argc, char*argv[]){
      t_log* logger = log_create("memoria.log","MEMORIA",1,LOG_LEVEL_INFO);
    if(argc <2){
        printf("Error: Mal ejecutado");
        return EXIT_FAILURE;
    }
 
   // cargamos el config para sacar la informacion
   t_config* config = iniciar_config(argv[1]);
   char* puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
if(puerto == NULL){
    log_error(logger,"No se encontro el PUERTO_ESCUCHA en el config");
    return EXIT_FAILURE;
}
   //iniciamos logger 
   
   log_info(logger, "Iniciando Servidor de Memoria en el puerto %s", puerto);

   //Abrimos el puerto para empezar a escuchar
   int fd_escucha = iniciar_servidor(puerto);
   if(fd_escucha == -1){
    log_error(logger,"Fallo al iniciar el servidor de memoria");
    return EXIT_FAILURE;
   }
log_info(logger,"Servidor de memoria encendida.Esperando a los modulos");

//ciclo while para esperar a los clientes 
int clientes_conectados = 0;
int fd_cpu = -1, fd_ms = -1, fd_scheduler = -1, fd_swap = -1; //inicializamos cada modulo en -1 no en 0 porque 0 ya es un valor reservado por linux
while (clientes_conectados < 4){
    int socket_cliente = esperar_cliente(fd_escucha);
    int cod_op = recibir_operacion(socket_cliente);
    if(cod_op == MENSAJE){ 
        //si llego un mensaje lo desarmamos para armar un paquete
        char* mensaje = recibir_mensaje(socket_cliente);
        //aca se identifica quien se conecto segun el texto
        if(strcmp(mensaje,"HANDSHAKE_CPU") == 0 ){
            log_info(logger,"CPU Conectada");
            fd_cpu = socket_cliente;
            clientes_conectados++;
 }
else if (strcmp(mensaje,"HANDSHAKE_MS")== 0){
    log_info(logger,"Memory Stick Conectada");
    fd_ms = socket_cliente;
    clientes_conectados++;
}    
else if (strcmp(mensaje,"HANDSHAKE_SCHEDULER")== 0){
    log_info(logger,"Kernel Scheduler Conectado");
    fd_scheduler = socket_cliente;
    clientes_conectados++;
}
else if (strcmp(mensaje, "HANDSHAKE_SWAP")==0){
    log_info(logger, "SWAP Conectado");
    fd_swap = socket_cliente;
    clientes_conectados++;
}
else{
    log_warning(logger, "Mensaje de handshake desconocido: %s",mensaje); // si llego un mensaje de un modulo no deseado error
    close(socket_cliente);
}
free(mensaje);
}else{
    log_warning(logger, "Operacion desconocida.Se espera un MENSAJE");
    close(socket_cliente); // si no llego un mensaje error
}
}
log_info(logger,"Todos los clientes conectados.Memoria lista para operar");
// aca supongo ira despues lo del checkpoint 2
close(fd_cpu);
close(fd_ms);
close(fd_scheduler);
close(fd_swap);
close(fd_escucha);
config_destroy(config);
log_destroy(logger);
return 0;
}
















