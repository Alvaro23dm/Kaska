/*
 * Incluya en este fichero todas las definiciones que pueden
 * necesitar compartir el broker y la biblioteca, si es que las hubiera.
 */
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>


#include "map.h"
#include "queue.h"

#ifndef _COMUN_H
#define _COMUN_H        1

typedef struct tema {
    char *titular;
    queue *mensajes;
} tema;

typedef struct mensaje {
    void *mensaje;
    int tam_mensaje;
} mensaje;

// información que se la pasa el thread creado
typedef struct thread_info {
    int socket; // añadir los campos necesarios
    map *mapa_temas;// mapa con todos los temas
} thread_info;



#endif // _COMUN_H
