#include "config.h"
#include <stdio.h>
#include <stdlib.h>

t_config* iniciar_config(char* path_config) {
    t_config* nuevo_config = config_create(path_config);

    if (nuevo_config == NULL) {
        printf("Error no se pudo leer el archivo %s\n", path_config);
        exit(EXIT_FAILURE);
    }

    return nuevo_config;
}
