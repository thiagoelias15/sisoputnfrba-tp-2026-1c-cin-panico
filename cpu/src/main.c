#include <utils/utils.h>

int main(int argc,char* argv[])/*
argc guarda la cantidad de palabras de la terminal 
y argv[numero]guarda la palabra en esa posicion exacta*/
{
    if(argc<3){ /*si la cantidad de palabras de la termina es menor a 3 error */
        prinf("Error, se debe usar: ./bin/cpu ./config[id/numero]\n");
        return EXIT_FAILURE;
    }

    t_config* config = iniciar_config(argv[1]);

    //cargamos el archivo config usando el parametro argv[1]
    char* ip_mem = config_get_string_value(config, "IP_MEMORIA");
    char* port_mem = config_get_string_value(config, "PUERTO_MEMORIA");
    char* ip_sched = config_get_string_value(config, "IP_SCHEDULER");
    char* port_sched = config_get_string_value(config, "PUERTO_SCHEDULER"); /*aca estamos sacando la informacion,del .config, que necesita la cpu para conectarse a los demas modulos*/
    char* ip_ms = config_get_string_value(config, "IP_MS");
    char* port_ms = config_get_string_value(config, "PUERTO_MS");
    t_log* logger = log_create("cpu.log", "CPU", 1, LOG_LEVEL_INFO);

    log_info(logger, "Iniciando CPU numero %s",argv[2]);
    // Usamos %s porque argv[2] ya es un texto, %s le dice que debe dejar espacio para un string y va y busca un string, argv[2] donde esta guardado el numero o ID de la cpu.

    //conexiones
    //Ahora usamos las variables que sacamos del config(ip_mem,port_mem,etc)
    int fd_memoria = crear_conexion(ip_mem,port_mem);
    if(fd_memoria !=-1){
        int mi_codigo = 1; //cpu = 1
        send(fd_memoria,&mi_codigo,sizeof(int),0);
        //le mandamos a la memoria el texto de numero de cpu que se quiere conectar
        send(fd_memoria,argv[2],strlen(argv[2])+1,0)// se pone +1 para que tambien se mande el /0 para que el log sepa donde cortar
    }

    int fd_scheduler = crear_conexion(ip_sched,port_sched);
    if(fd_scheduler != -1){
        int mi_codigo_sched=1; //cpu=1 para el scheduler
        send(fd_scheduler,&mi_codigo_sched,sizeof(int),0);
        log_info(logger, "CPU conectada con el Scheduler %s", mi_codigo_sched);
        int fd_ms = crear_conexion(ip_ms,port_ms);
        if(fd_ms != -1){
            log_info(logger,"CPU conectada a la Memory Stick");
        }
    }

    if(fd_memoria != -1 && fd_scheduler != -1 && fd_ms !=-1){
        log_info(logger,"CPU conectada a todos los modulos correctamente");
    }

    //limpieza
    close(fd_memoria); close(fd_scheduler);close (fd_ms);
    config_destroy(config); //liberamos la memoria del config
    log_destroy(logger);

    return 0; 
}
