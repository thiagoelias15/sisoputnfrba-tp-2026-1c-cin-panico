#include "main.h"
#include "conexiones/conexiones.h"
#include "planificador/planificador.h"

// Definición de variables globales(en el main.h con extern solo le avisabamos al compiladro que existian pero no estaban definidas)
t_list* cola_new;
t_list* cola_ready;
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

    if(argc<2) {  

        printf("[ERROR] Mal ejecutado\n");
        return EXIT_FAILURE; 
    }

    // Se carga el config, extrayendo la información necesaria para la conexión de módulos.

    cargar_configuracion_kernel(argv[1]);
    logger = log_create("Scheduler.log", "SCHEDULER", 1, LOG_LEVEL_INFO);
    log_info(logger, "Iniciando Scheduler con algoritmo: %s", kernel_config.algoritmo_planificacion);

    // 2. INICIALIZAMOS ESTRUCTURAS

    cola_new = list_create();
    cola_ready = list_create();
    cola_exec = list_create();
    cola_block = list_create();
    cola_exit = list_create();
    dic_mutex = dictionary_create();
    dic_interfaces = dictionary_create();

    pthread_mutex_init(&m_ready, NULL);
    pthread_mutex_init(&m_new, NULL);
    pthread_mutex_init(&m_block, NULL);
    pthread_mutex_init(&m_exit, NULL);
    sem_init(&sem_procesos_ready, 0, 0);

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

    //el proceso 0 o inicial seria PID 0
    t_pcb* pcb_inicial = malloc(sizeof(t_pcb));
    pcb_inicial -> pid = 0;
    pcb_inicial -> pc = 0;

    //inicializamos los registros en 0
    pcb_inicial->ax = 0; pcb_inicial->bx = 0; pcb_inicial->cx = 0; pcb_inicial->dx = 0;
    pcb_inicial->eax = 0; pcb_inicial->ebx = 0; pcb_inicial->ecx = 0; pcb_inicial->edx = 0;
    pcb_inicial->si = 0; pcb_inicial->di = 0;

    log_info(logger, "## (%d) Se crea el proceso - Estado: NEW",pcb_inicial->pid);
    //entrada NEW
    pthread_mutex_lock(&m_new);
    list_add(cola_new, pcb_inicial);
    pthread_mutex_unlock(&m_new);

    //pasaje de NEW a READY
    pthread_mutex_lock(&m_new);
    t_pcb* pcb_a_ready = list_remove(cola_new, 0);
    pthread_mutex_unlock(&m_new);

    log_info(logger, "## (%d) Pasa del estado NEW a READY", pcb_a_ready->pid); //%d imprime numeros enteros, en este caso va a buscar el el valor numerico que este guardado en pcb_a_ready->pid

    pthread_mutex_lock(&m_ready);
    list_add(cola_ready, pcb_a_ready);
    pthread_mutex_unlock(&m_ready);
    //activamos el sem para que el planificado de corto plazo lo mande  a la cpu
    sem_post(&sem_procesos_ready);

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