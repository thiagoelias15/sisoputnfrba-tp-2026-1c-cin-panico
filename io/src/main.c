#include <utils/utils.h>

int main(int argc, char* argv[]) {

    if(argc<2) {

        printf("[ERROR] Mal ejecutado");
        return EXIT_FAILURE;
    }

    // Se inicializan las herramientas de las commons y se extraen los parámetros correspondientes al config
    t_config* config = iniciar_config(argv[1]);

    char* ip_sched = config_get_string_value(config, "IP_SCHEDULER");
    char* puerto_sched = config_get_string_value(config, "PUERTO_SCHEDULER");
    char* mi_id = config_get_string_value(config,"ID_MODULO");

    // Se inicia el Logger
    t_log* logger = log_create("io.log","IO",1,LOG_LEVEL_INFO);
    log_info(logger, "Iniciando modulo de I/O ");

    // ------------------------------ CONEXIONES ------------------------------ //

    int fd_scheduler = crear_conexion(ip_sched,puerto_sched);

    if(fd_scheduler != -1) {
     // Handshake: basicamente se "presenta la IO con el scheduler usando el id del config"
        enviar_mensaje(mi_id, MENSAJE, fd_scheduler);
        log_info(logger, "I/O conectada al Scheduler correctamente."); 
    }
    
   //-------------------------------------------- Bucle de conexion con el scheduler ----------------------
    //con while(1) se queda en un bucle infinito esperando que el scheduler le asigne algo para hacer
while(1){
    //1° La IO va a escuchar el codigo de operacion.La funcion frena el hilo hasta que le llegue algo por el socket
    op_code cod_op = recibir_operacion(fd_scheduler);

    //si recibir operacion recibe -1 es porque se corto la conexion( se puede haber cerrado o crasheado el scheduler)
   if(cod_op == -1){
    log_error(logger, "El scheduler se desconecto de forma abrupta.Cerrando conexion");
   break; // ponemos break para romper el ciclo infinto y que salga para ir directo a la limpieza
   }

// segun el codigo de operacion que nos mando el scheduler la IO decide que tiene que hacer
switch(cod_op){

    case SYSCALL_STDOUT: {
        // el scheduler manda un texto diciendo STDOUT con la funcion enviar_mensaje y la IO lo recibe
        // una vez que recibio y sabe lo que tiene que hacer lo borramos para dejarlo preparado para recibir otro mensaje
       
        char* msg_aviso = recibir_mensaje(fd_scheduler);
        free(msg_aviso); //como dijimos limpiamos el texto que recibio la IO
        int tam,dir,pid; /* Recibimos 3 numeros entero que nos mando el scheduler tamaño,direccion y pid y los guardamos en sus variables correspondientes, 
        si se cambia el orden de envio desde el scheduler tambien debemos cambiarlo aqui sino se va a 
        guardar algo en una variable que no corresponde ya que no reconoce sino que las guarda por orden de llegada*/
       
        recv(fd_scheduler, &tam,sizeof(int),MSG_WAITALL);  //como sabemos WAITALL hace que el socket espere a recibir todos los bytes ya que a veces llegan por ejemplo 2 bytes y con una diferencia de milisegundos los otros 2 pero el socket ya guardo solo los primero 2 bytes esto nos evita ese error 
        recv(fd_scheduler,&dir, sizeof(int),MSG_WAITALL);
        recv(fd_scheduler,&pid, sizeof(int),MSG_WAITALL); //PID es el id del proceso(un numero entero para que sea mas simple el manejo de tantos procesos al mismo tiempo)
        log_info(logger, "## (%d) Peticion STDOUT recibida. Tamaño a leer: %d", pid,tam,dir);
        // como imprimir por pantalla o buscar en un disco duro, las acciones que hace la IO, son muy lentas tenemos que frenar al hilo por un tiempo para darle tiempo a que haga lo que se le pidio
       
        usleep(1000 * 1000); // esto seria un millon de microsegundos = 1 segundo si vemos que dsp falla se puede poner mas alizó STDOUT, aviso enviado al Scheduler.", pid);
        
        //ahora vendria la devolucion, primero hay que avisarle al scheduler que termino con la etiqueta FIN_IO y luego le mandamos la informacion,PID del proceso que scheduler debe sacar de BLOCK y mover a READY
        enviar_mensaje("FIN_IO", MENSAJE, fd_scheduler);
        send(fd_scheduler, &pid, sizeof(int),0);
        log_info(logger,"## (%d) Finalizo STDOUT.",pid);
        break;
    
    }
  
    case SYSCALL_STDIN: {
// Misma lógica de limpieza que en STDOUT.
                    char* msg_aviso = recibir_mensaje(fd_scheduler);
                    free(msg_aviso);

                    int tam, dir, pid;
                    
                    // Recibimos los parámetros exactos. 
                    recv(fd_scheduler, &tam, sizeof(int), MSG_WAITALL);
                    recv(fd_scheduler, &dir, sizeof(int), MSG_WAITALL);
                    recv(fd_scheduler, &pid, sizeof(int), MSG_WAITALL);

                    log_info(logger, "## (%d) Petición STDIN recibida. Se espera ingreso del usuario (Max %d bytes)", pid, tam);

                    // Simulamos que el usuario tarda bastante en tipear usando el teclado (2 segundos).
                    usleep(2000 * 1000); 
                   
                    // STDIN es una operación bloqueante porque esperar al usuario es lento.
                    // Le mandamos la etiqueta "FIN_IO" al Scheduler para avisar que el usuario ya terminó de tipear.
                    // Luego le enviamos el PID para que el Scheduler sepa a quién debe sacar de BLOCK y mover a READY.
                
                    enviar_mensaje("FIN_IO", MENSAJE, fd_scheduler);
                    send(fd_scheduler, &pid, sizeof(int), 0);

                    log_info(logger, "## (%d) Finalizó STDIN.", pid);
                    break;
                }

                case SYSCALL_SLEEP: {
            
                    char* msg_aviso = recibir_mensaje(fd_scheduler);
                    free(msg_aviso);// Limpiamos el texto de aviso.

                    // El SLEEP no necesita tamaño ni dirección de memoria. 
                    // Solo necesita saber cuánto tiempo dormir y a quién.
                    int tiempo_ms, pid;
                    
                    // Recibimos el tiempo en milisegundos y el ID del proceso.
                    recv(fd_scheduler, &tiempo_ms, sizeof(int), MSG_WAITALL);
                    recv(fd_scheduler, &pid, sizeof(int), MSG_WAITALL);

                    log_info(logger, "## (%d) Petición SLEEP recibida. A dormir por %d ms.", pid, tiempo_ms);

                    // La función de C usleep() espera microsegundos.
                    // Como nuestra configuración está en milisegundos, lo multiplicamos por 1000.
                    usleep(tiempo_ms * 1000); 

                    // El proceso ya descansó lo suficiente, le avisamos al Scheduler que lo despierte (lo pase a READY).
                    enviar_mensaje("FIN_IO", MENSAJE, fd_scheduler);
                    send(fd_scheduler, &pid, sizeof(int), 0);

                    log_info(logger, "## (%d) Finalizó SLEEP, aviso enviado al Scheduler.", pid);
                    break;
                }

                default:
                    // Si llega basura por el socket o una operación que no existe, caemos acá
                    // en vez de romper el programa.
                    log_warning(logger, "Operación desconocida recibida del Scheduler. Código: %d", cod_op);
                    break;
            }
        } // Fin del while(1)
    } else {
        // Si fd_scheduler devolvió -1 al principio, significa que el Scheduler estaba apagado.
        log_error(logger, "No se pudo conectar al Scheduler. Revisa si está encendido y la IP/Puerto son correctos.");
    }
       // ------------------------------ LIMPIEZA DE LA MEMORIA ------------------------------ //

    close(fd_scheduler);
    config_destroy(config);
    log_destroy(logger);

    return 0;
}

