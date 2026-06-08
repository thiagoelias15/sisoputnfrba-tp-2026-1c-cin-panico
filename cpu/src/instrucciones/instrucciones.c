#include "instrucciones.h"
#include "../main.h"

/* En C, el texto "AX" (char*) y la variable física 'pcb->ax' no tienen relación.

C no sabe abrir una variable a partir de un texto de forma automática.

Esta función actúa como un "traductor". Compara el texto recibido por red 

usando strcmp() y, al encontrar la coincidencia, guarda el número en el campo  del struct PCB que corresponde*/

void setear_valor_registro(t_pcb* pcb, char* reg, uint32_t valor) {
    if(strcmp(reg, "AX") == 0) pcb -> ax = (uint8_t)valor;
    else if(strcmp(reg, "BX") == 0) pcb -> bx = (uint8_t)valor;
    else if(strcmp(reg, "CX") == 0) pcb -> cx = (uint8_t)valor;
    else if(strcmp(reg, "DX") == 0) pcb -> dx = (uint8_t)valor;
    else if(strcmp(reg, "EAX") == 0) pcb -> eax = valor;
    else if(strcmp(reg, "EBX") == 0) pcb -> ebx = valor;
    else if(strcmp(reg, "ECX") == 0) pcb -> ecx = valor;
    else if(strcmp(reg, "EDX") == 0) pcb -> edx = valor;
    else if(strcmp(reg, "SI") == 0) pcb -> si = valor;
    else if(strcmp(reg, "DI") == 0) pcb -> di = valor;
}
/*Hace exactamente lo inverso a la función anterior. Compara el texto (ej: "BX"), busca la 

 variable física correspondiente en nuestro struct (pcb->bx), y nos devuelve el número 

 real que tiene guardado adentro para poder hacer operaciones matemáticas (SUM, SUB)*/

uint32_t obtener_valor_registro(t_pcb* pcb, char* reg) {
    if(strcmp(reg, "AX") == 0) return pcb->ax;
    if(strcmp(reg, "BX") == 0) return pcb->bx;
    if(strcmp(reg, "CX") == 0) return pcb->cx;
    if(strcmp(reg, "DX") == 0) return pcb->dx;
    if(strcmp(reg, "EAX") == 0) return pcb->eax;
    if(strcmp(reg, "EBX") == 0) return pcb->ebx;
    if(strcmp(reg, "ECX") == 0) return pcb->ecx;
    if(strcmp(reg, "EDX") == 0) return pcb->edx;
    if(strcmp(reg, "SI") == 0) return pcb->si;
    if(strcmp(reg, "DI") == 0) return pcb->di;
    return 0;
}

/* MMU traduce una direccion logica a fisica basandose en segmentacion, divide
la direccion logica por el tamaño maximo para hallar el id del segmento, luego
busca ese segmento en la tabla del proceso y verifica que el desplazamiento sea valido.*/

int traducir_direccion_mmu(uint32_t dir_logica, uint32_t tam_a_leer_escribir, t_pcb* pcb){
    //formulas del enunciado
    int num_segmento = dir_logica / tam_max_segmento;
    int desplazamiento = dir_logica % tam_max_segmento;

    //buscamos el segmento en la tabla de procesos
    t_segmento* segmento_encontrado = NULL;
    for(int i = 0; i < list_size(pcb->tabla_segmentos); i++){
        t_segmento* seg = list_get(pcb-> tabla_segmentos, i);
        if(seg-> id == num_segmento){
            segmento_encontrado = seg;
            break;
        }
    }
    // verificamos erroes: si el segmento no existe o si nos pasamos del tamaño permitido
    if(segmento_encontrado == NULL || (desplazamiento + tam_a_leer_escribir) > segmento_encontrado -> tamanio){
        return -1; // -1 seria SEG_FAULT
    }
    // si todo estuvo bien, calculamos la dirección física final
    int direccion_fisica = segmento_encontrado -> direccion_base + desplazamiento;
    return direccion_fisica;
}

void ejecutar_instrucciones(char** tokens, t_pcb* pcb, int* desalojar, op_code* motivo, int* modifico_pc, t_log* logger) {
    char* comando = tokens[0];
    // Instrucciones matematicas y de registro
    if(strcmp(comando, "SET") == 0) {
        setear_valor_registro(pcb, tokens[1], atoi(tokens[2]));
    } 
    else if(strcmp(comando, "SUM") == 0) {
        uint32_t dest = obtener_valor_registro(pcb, tokens[1]);
        uint32_t orig = obtener_valor_registro(pcb, tokens[2]);
        setear_valor_registro(pcb, tokens[1], dest + orig);
    } 
    else if(strcmp(comando, "SUB") == 0) {
        uint32_t dest = obtener_valor_registro(pcb, tokens[1]);
        uint32_t orig = obtener_valor_registro(pcb, tokens[2]);
        setear_valor_registro(pcb, tokens[1], dest - orig);
    } 
    else if(strcmp(comando, "JNZ") == 0) {
        if(obtener_valor_registro(pcb, tokens[1]) != 0) {
            pcb->pc = atoi(tokens[2]);
            *modifico_pc = 1;
        }
    } 
    // Syscalls: requieren intervencion del kernel scheduler,
    else if(strcmp(comando, "SLEEP") == 0) {
        *desalojar = 1;
        *motivo = SYSCALL_SLEEP;
    } 
    else if (strcmp(comando, "STDOUT") == 0) {
      uint32_t dir_logica = obtener_valor_registro(pcb, tokens[1]);
      uint32_t tam = obtener_valor_registro(pcb, tokens[2]);
      //ejecutamos la MMU para validar que la direccion no rompa la memory
      int dir_fisica = traducir_direccion_mmu(dir_logica, tam, pcb);
      *desalojar = 1;
      if(dir_fisica == -1){
        *motivo = SEG_FAULT; // la MMU freno el proceso
      }else{
        *motivo = SYSCALL_STDOUT;
      }
    }
    else if (strcmp(comando, "STDIN") == 0) {
        uint32_t dir_logica = obtener_valor_registro(pcb, tokens[1]);
        uint32_t tam = obtener_valor_registro(pcb, tokens[2]);
        
        // Ejecutamos la MMU para validar que la dirección no rompa la memoria
        int dir_fisica = traducir_direccion_mmu(dir_logica, tam, pcb);

        *desalojar = 1;
        if(dir_fisica == -1) {
            *motivo = SEG_FAULT; // La MMU frenó el proceso 
        } else {
            *motivo = SYSCALL_STDIN;
        }
    }
    else if(strcmp(comando, "MUTEX_CREATE") == 0) {
        *desalojar = 1;
        *motivo = SYSCALL_MUTEX_CREATE;
    } 
    else if(strcmp(comando, "MUTEX_LOCK") == 0) {
        *desalojar = 1;
        *motivo = SYSCALL_MUTEX_LOCK;
    } 
    else if(strcmp(comando, "MUTEX_UNLOCK") == 0) {
        *desalojar = 1;
        *motivo = SYSCALL_MUTEX_UNLOCK;
    }
    else if(strcmp(comando, "EXIT") == 0) {
        *desalojar = 1;
        *motivo = SYSCALL_EXIT;
    }
}