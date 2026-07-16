#include "io_core.h"
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>

// Esta variable se mudó acá para controlar el bucle
int io_corriendo = 1;

void ejecutar_sleep(int fd_scheduler)
{

    // El SLEEP no necesita tamaño ni dirección de memoria.
    // Solo necesita saber cuánto tiempo dormir y a quién.
    int tiempo_ms, pid;

    // Recibimos el tiempo en milisegundos y el ID del proceso.
    recv(fd_scheduler, &tiempo_ms, sizeof(int), MSG_WAITALL);
    recv(fd_scheduler, &pid, sizeof(int), MSG_WAITALL);

    // 1° Log Obligatorio exigido por el TP
    log_info(logger, "## PID: %d - Inicio de IO", pid);

    // Log Obligatorio específico para SLEEP
    log_info(logger, "## PID: %d - Haciendo sleep por %d milisegundos.", pid, tiempo_ms);

    // La función de C usleep() espera microsegundos.
    // Como nuestra configuración (y lo que nos manda el Kernel) está en milisegundos, lo multiplicamos por 1000.
    usleep(tiempo_ms * 1000);

    // El proceso ya descansó lo suficiente, le avisamos al Scheduler que lo despierte (lo pase a READY).
    enviar_mensaje("FIN_IO", MENSAJE, fd_scheduler);
    send(fd_scheduler, &pid, sizeof(int), 0);

    // 2° Log Obligatorio exigido por el TP
    log_info(logger, "## PID: %d - Fin de IO", pid);
}

void ejecutar_stdin(int fd_scheduler)
{

    int tam, dir, pid;

    /* Recibimos 3 numeros enteros que nos mando el scheduler: tamaño, direccion y pid y los guardamos.
    Si se cambia el orden de envio desde el scheduler tambien debemos cambiarlo aqui sino se va a
    guardar algo en una variable que no corresponde. */
    recv(fd_scheduler, &tam, sizeof(int), MSG_WAITALL);
    recv(fd_scheduler, &dir, sizeof(int), MSG_WAITALL);
    recv(fd_scheduler, &pid, sizeof(int), MSG_WAITALL);

    //  1° Log Obligatorio exigido por el TP
    log_info(logger, "## PID: %d Inicio de IO", pid);

    //  Log Obligatorio específico para STDIN
    int cant_numeros = tam / sizeof(int);
    log_info(logger, "## PID: %d Ingrese %d numero/s:", pid, cant_numeros);
    // calloc reserva la memoria y la llena automáticamente de '\0' (barra cero).
    // Así nos ahorramos tener que rellenar a mano con un bucle if/for si el usuario escribe de menos.
    void *buffer_a_devolver = calloc(tam, 1);
    for(int i = 0; i < cant_numeros; i++) {
    char* leido = readline (">");
    if (leido != NULL)
    {
         int numero = atoi(leido);  // convertimos el texto a entero
            memcpy(buffer_a_devolver + (i * sizeof(int)), &numero, sizeof(int));
            free(leido);
        }
    }

    op_code op_respuesta = SYSCALL_STDIN;
    send(fd_scheduler, &op_respuesta, sizeof(op_code), 0);
    send(fd_scheduler, &pid, sizeof(int), 0);
    send(fd_scheduler, &dir, sizeof(int), 0);
    send(fd_scheduler, &tam, sizeof(int), 0);
    send(fd_scheduler, buffer_a_devolver, tam, 0);

    log_info(logger, "## PID: %d - Fin de IO", pid);
    free(buffer_a_devolver);
}

// Función aislada para manejar específicamente STDOUT
void ejecutar_stdout(int fd_scheduler)
{

    int pid;
    /* Recibimos el numero entero que nos mando el scheduler y lo guardamos en su variable correspondiente,
    si se cambia el orden de envio desde el scheduler tambien debemos cambiarlo aqui sino se va a
    guardar algo en una variable que no corresponde ya que no reconoce sino que las guarda por orden de llegada*/

    // PID es el id del proceso(un numero entero para que sea mas simple el manejo de tantos procesos al mismo tiempo)
    // como sabemos WAITALL hace que el socket espere a recibir todos los bytes ya que a veces llegan por ejemplo 2 bytes y con una diferencia de milisegundos los otros 2 pero el socket ya guardo solo los primero 2 bytes esto nos evita ese error
    recv(fd_scheduler, &pid, sizeof(int), MSG_WAITALL);

    // [Como dicta el enunciado, KernelMemory ya buscó el texto en la memoria física,
    // y el Scheduler nos manda directamente el string armado. Lo recibimos así:
    char *texto_a_imprimir = recibir_mensaje(fd_scheduler);

    // 1° Log Obligatorio exigido por el TP
    log_info(logger, "## PID: %d - Inicio de IO", pid);

    // El enunciado dice explícitamente: "imprimir por pantalla y en el archivo de Log."
    // Reemplazamos el usleep() por la acción real.
    log_info(logger, "## PID: %d - %s", pid, texto_a_imprimir);

    // ahora vendria la devolucion, primero hay que avisarle al scheduler que termino con la etiqueta FIN_IO y luego le mandamos la informacion,PID del proceso que scheduler debe sacar de BLOCK y mover a READY
    enviar_mensaje("FIN_IO", MENSAJE, fd_scheduler);
    send(fd_scheduler, &pid, sizeof(int), 0);

    // 2° Log Obligatorio exigido por el TP
    log_info(logger, "## PID: %d - Fin de IO", pid);

    // Limpiamos la memoria del texto que nos mandó el Kernel
    free(texto_a_imprimir);
}

//-------------------------------------------- Bucle de conexion con el scheduler ----------------------
void atender_peticiones_io(int fd_scheduler)
{

    // con while(1) se queda en un bucle infinito esperando que el scheduler le asigne algo para hacer
    while (io_corriendo)
    {

        // 1° La IO va a escuchar el codigo de operacion.La funcion frena el hilo hasta que le llegue algo por el socket
        op_code cod_op = recibir_operacion(fd_scheduler);

        // si recibir operacion recibe -1 es porque se corto la conexion( se puede haber cerrado o crasheado el scheduler)
        if (cod_op == -1)
        {
            log_error(logger, "El scheduler se desconecto de forma abrupta.Cerrando conexion");
            break; // ponemos break para romper el ciclo infinto y que salga para ir directo a la limpieza
        }

        // segun el codigo de operacion que nos mando el scheduler la IO decide que tiene que hacer
        switch (cod_op)
        {

        case SYSCALL_STDOUT:
        {
            // el scheduler manda un texto diciendo STDOUT con la funcion enviar_mensaje y la IO lo recibe
            // una vez que recibio y sabe lo que tiene que hacer lo borramos para dejarlo preparado para recibir otro mensaje
            char *msg_aviso = recibir_mensaje(fd_scheduler);
            free(msg_aviso); // como dijimos limpiamos el texto que recibio la IO

            // Saltamos a la función modularizada para ejecutar la lógica limpia
            ejecutar_stdout(fd_scheduler);
            break;
        }

        case SYSCALL_STDIN:
        {
            char *msg_aviso = recibir_mensaje(fd_scheduler);
            free(msg_aviso); // limpiamos el texto que recibio la IO

            // Saltamos a la función modularizada
            ejecutar_stdin(fd_scheduler);
            break;
        }

        case SYSCALL_SLEEP:
        {
            char *msg_aviso = recibir_mensaje(fd_scheduler);
            free(msg_aviso); // Limpiamos el texto de aviso.

            // Saltamos a la función modularizada para ejecutar la espera
            ejecutar_sleep(fd_scheduler);
            break;
        }

        default:
            // Si llega basura por el socket o una operación que no existe, caemos acá en vez de romper el programa.
            log_warning(logger, "Operación desconocida recibida del Scheduler. Código: %d", cod_op);
            break;
        }
    } // Fin del while(io_corriendo)
}