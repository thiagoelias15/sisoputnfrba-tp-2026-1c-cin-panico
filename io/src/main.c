#include "main.h"
#include "core/io_core.h" 

// Declaración real del logger global que pusimos en main.h
t_log* logger;


int main(int argc, char* argv[]) {

    // [NUEVO] Validamos que lleguen los 3 parametros que exige el enunciado del TP
    if(argc < 3) {
        printf("[ERROR] Mal ejecutado. Uso correcto: ./bin/io [Archivo Config] [Tipo]\n");
        return EXIT_FAILURE;
    }

    // [NUEVO] Guardamos el path y el tipo de I/O (GENERICA, STDIN, etc.)
    char* path_config = argv[1];
    char* tipo_interfaz = argv[2]; 

    // Se inicializan las herramientas de las commons y se extraen los parámetros correspondientes al config
    cargar_configuracion_io(path_config);

    // Se inicia el Logger
    logger = log_create("io.log", io_config.id_modulo, 1, LOG_LEVEL_INFO);
    
    // Agregamos el tipo_interfaz al log inicial para saber con que estamos trabajando
    log_info(logger, "Iniciando modulo de I/O. Interfaz: %s, Tipo: %s", io_config.id_modulo, tipo_interfaz);

    // ------------------------------ CONEXIONES ------------------------------ //

    int fd_scheduler = crear_conexion(io_config.ip_sched, io_config.puerto_sched);

    if(fd_scheduler != -1) {

        // Handshake: basicamente se "presenta la IO con el scheduler usando el id del config"
        enviar_mensaje(io_config.id_modulo, MENSAJE, fd_scheduler);
        
        //Log minimo y obligatorio exigido por la catedra
        log_info(logger, "## Conectado a Kernel Scheduler"); 
        
        //Pasamos al bucle principal en el archivo io_core.c para mantener el main limpio
        atender_peticiones_io(fd_scheduler);

    } else {
        // Si fd_scheduler devolvió -1 al principio, significa que el Scheduler estaba apagado.
        log_error(logger, "No se pudo conectar al Scheduler. Revisa si está encendido y la IP/Puerto son correctos.");
    }
    
    // ------------------------------ LIMPIEZA DE LA MEMORIA ------------------------------ //

    close(fd_scheduler);
    destruir_configuracion_io();
    log_destroy(logger);

    return 0;
}
