#include "main.h"
#include "conexiones/conexiones.h"
#include "planificador/planificador.h"

int PID_GLOBAL = 0;
sem_t sem_cpu_libre;
// Definición de variables globales(en el main.h con extern solo le avisabamos al compiladro que existian pero no estaban definidas)
t_list* cola_new;
t_list** colas_ready;
int cantidad_colas = 0;
t_list* cola_exec;
t_list* cola_block;
t_list* cola_exit;
pthread_mutex_t m_ready, m_new, m_block, m_exit;
sem_t sem_procesos_ready;
t_dictionary* dic_mutex;
t_dictionary* dic_interfaces;

int fd_cpu = -1;
int fd_memoria = -1;
t_log* logger;
int scheduler_corriendo = 1;

int main(int argc,char* argv[]) {
    // Se verifica la información en el config
    if(argc<3) {  

     printf("[ERROR] Uso correcto: ./bin/kernel_scheduler [config_path] [script_name.prc]\n");
        return EXIT_FAILURE; 
    }
    // Se carga el config, extrayendo la información necesaria para la conexión de módulos.

    cargar_configuracion_kernel(argv[1]);
    logger = log_create("Scheduler.log", "SCHEDULER", 1, LOG_LEVEL_INFO);
    log_info(logger, "Iniciando Scheduler con algoritmo: %s", kernel_config.algoritmo_planificacion);

    // 2. INICIALIZAMOS ESTRUCTURAS

 inicializar_estructuras();

    // ------------------------------ SCHEDULER COMO CLIENTE ------------------------------ //

    // 3. CONEXIÓN A MEMORIA (Usando nuestra esctuctura config)el . indica que ingresamos o tomamos dicho campo de un determinado struct
    fd_memoria = crear_conexion(kernel_config.ip_memoria, kernel_config.puerto_memoria);
    
    if(fd_memoria != -1) {
        enviar_mensaje(kernel_config.id_modulo, MENSAJE, fd_memoria);
        log_info(logger, "Scheduler conectado a la memoria con ID: %s", kernel_config.id_modulo);
    } else {
        log_error(logger, "Fallo en la conexión a la memoria");
        return EXIT_FAILURE; 
    }

    //------------------------ Planificacion a largo plazo(proceso 0 o inical) ---------------------------------------

    
crear_proceso(argv[2], 0 );

    // --------------------------Planificador a corto plazo-------------------------------------//

    pthread_t hilo_planificador; //declaramos variable de tipo pthread_t llamada hilo_planificador
    pthread_create(&hilo_planificador, NULL, planificador_corto_plazo, NULL); /*creamos un hilo nuevo con estos 4 parametros
    &hilo_planificador es para guardar el ID del hilo creado,NULL es para que no haga nada especial solo configuracion estandar,planificador_corto_plazo es la funcion que va a ejecutar el hilo */
    pthread_detach(hilo_planificador); //al crear un hilo linux deja vinculados el hilo nuevo al hilo main con detach desvinculamos los hilos para que el nuevo trabaje de forma independiente

    // ------------------------------ SCHEDULER COMO SERVIDOR ------------------------------ //

    int fd_escucha = iniciar_servidor(kernel_config.puerto_escucha);
    log_info(logger, "Servidor del scheduler encendido. Escuchando CPUs e IOs");

    while(scheduler_corriendo) { //al poner while(scheduler_corrieendo) es un TRUE osea siempre verdadero lo que permite que el while sea infinito
        
        int* socket_cliente = malloc(sizeof(int));
        *socket_cliente = esperar_cliente(fd_escucha);
        
        if(*socket_cliente != -1) {
            pthread_t hilo_cliente;
            pthread_create(&hilo_cliente, NULL, atender_cliente, socket_cliente);
            pthread_detach(hilo_cliente);
        } else {
            free(socket_cliente);
        }
    }

    destruir_configuracion_kernel();
    return 0;
}