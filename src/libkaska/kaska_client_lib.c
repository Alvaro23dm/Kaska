#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-narrowing-conversions"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>

#include "comun.h"
#include "kaska.h"
#include "map.h"

typedef struct subscripcion {
    char *titular;
    int offset_local;
} subscripcion;

//Varariable global para ver si el socket está conectado
int sckt= -1;
map *subscricpciones;

// inicializa el socket y se conecta al servidor
static int init_socket_client(const char *host_server, const char * port) {
    int s;
    struct addrinfo *res;
    // socket stream para Internet: TCP
    if ((s=socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("error creando socket");
        return -1;
    }
    // obtiene la dirección TCP remota
    if (getaddrinfo(host_server, port, NULL, &res)!=0) {
        perror("error en getaddrinfo");
        close(s);
        return -1;
    }
    // realiza la conexión
    if (connect(s, res->ai_addr, res->ai_addrlen) < 0) {
        perror("error en connect");
        close(s);
        return -1;
    }
    freeaddrinfo(res);
    return s;
}





void init(){
    sckt = init_socket_client(getenv("BROKER_HOST"), getenv("BROKER_PORT"));
    subscricpciones= map_create(key_string, 0);
}

// Crea el tema especificado.
static int peticion_create_topic(char *string) {
    struct iovec iov[3]; // hay que enviar 3 elementos
    int nelem = 0;
    // preparo el envío del entero convertido a formato de red
    int entero_net = htonl(1);
    iov[nelem].iov_base=&entero_net;
    iov[nelem++].iov_len=sizeof(int);

    // preparo el envío del string mandando antes su longitud
    int longitud_str = strlen(string); // no incluye el carácter nulo
    int longitud_str_net = htonl(longitud_str);
    iov[nelem].iov_base=&longitud_str_net;
    iov[nelem++].iov_len=sizeof(int);
    iov[nelem].iov_base=string; // no se usa & porque ya es un puntero
    iov[nelem++].iov_len=longitud_str;

    // modo de operación de los sockets asegura que si no hay error
    // se enviará todo (misma semántica que los "pipes")
    if (writev(sckt, iov, 3)<0) {
        perror("error en writev");
        close(sckt);
        return -1;
    }
    int res;
    // recibe un entero como respuesta
    if (recv(sckt, &res, sizeof(int), MSG_WAITALL) != sizeof(int)) {
        perror("error en recv");
        close(sckt);
        return -1;
    }
    return ntohl(res);
}

// Devuelve 0 si OK y un valor negativo en caso de error.
int create_topic(char *topic) {
    if(sckt == -1) init();
    return peticion_create_topic(topic);
}

static int peticion_ntopics() {
    struct iovec iov[1]; // hay que enviar 1 elementos
    int nelem = 0;
    // preparo el envío del entero convertido a formato de red
    int entero_net = htonl(2);
    iov[nelem].iov_base=&entero_net;
    iov[nelem++].iov_len=sizeof(int);

    // modo de operación de los sockets asegura que si no hay error
    // se enviará todo (misma semántica que los "pipes")
    if (writev(sckt, iov, 1)<0) {
        perror("error en writev");
        close(sckt);
        return -1;
    }
    int res;
    // recibe un entero como respuesta
    if (recv(sckt, &res, sizeof(int), MSG_WAITALL) != sizeof(int)) {
        perror("error en recv");
        close(sckt);
        return -1;
    }
    int x = ntohl(res);
    return x;
}

// Devuelve cuántos temas existen en el sistema y un valor negativo
// en caso de error.
int ntopics(void) {
    if(sckt == -1) init();
    return peticion_ntopics();

}

// SEGUNDA FASE: PRODUCIR/PUBLICAR

// Envía el mensaje al tema especificado; nótese la necesidad
// de indicar el tamaño, ya que puede tener un contenido de tipo binario.
// Devuelve el offset si OK y un valor negativo en caso de error.
// Crea el tema especificado.
static int peticion_send_msg(char *string, int longitud_array, void *array) {
    struct iovec iov[5]; // hay que enviar 5 elementos
    int nelem = 0;
    // preparo el envío del entero convertido a formato de red
    int entero_net = htonl(3);
    iov[nelem].iov_base=&entero_net;
    iov[nelem++].iov_len=sizeof(int);

    // preparo el envío del string mandando antes su longitud
    int longitud_str = strlen(string); // no incluye el carácter nulo
    int longitud_str_net = htonl(longitud_str);
    iov[nelem].iov_base=&longitud_str_net;
    iov[nelem++].iov_len=sizeof(int);
    iov[nelem].iov_base=string; // no se usa & porque ya es un puntero
    iov[nelem++].iov_len=longitud_str;

    // preparo el envío del array mandando antes su longitud
    int longitud_arr_net = htonl(longitud_array);
    iov[nelem].iov_base=&longitud_arr_net;
    iov[nelem++].iov_len=sizeof(int);
    iov[nelem].iov_base=array; // no se usa & porque ya es un puntero
    iov[nelem++].iov_len=longitud_array;

    // modo de operación de los sockets asegura que si no hay error
    // se enviará todo (misma semántica que los "pipes")
    if (writev(sckt, iov, 5)<0) {
        perror("error en writev");
        close(sckt);
        return -1;
    }
    int res;
    // recibe un entero como respuesta
    if (recv(sckt, &res, sizeof(int), MSG_WAITALL) != sizeof(int)) {
        perror("error en recv");
        close(sckt);
        return -1;
    }
    return ntohl(res);
}

int send_msg(char *topic, int msg_size, void *msg) {
    if(sckt == -1) init();
    return peticion_send_msg(topic, msg_size, msg);
}
// Devuelve la longitud del mensaje almacenado en ese offset del tema indicado
// y un valor negativo en caso de error.
static int peticion_msg_length(char *string, int offset) {
    struct iovec iov[4]; // hay que enviar 1 elementos
    int nelem = 0;
    // preparo el envío del entero 1 convertido a formato de red
    int entero_net1 = htonl(4);
    iov[nelem].iov_base=&entero_net1;
    iov[nelem++].iov_len=sizeof(int);

    // preparo el envío del string mandando antes su longitud
    int longitud_str = strlen(string); // no incluye el carácter nulo
    int longitud_str_net = htonl(longitud_str);
    iov[nelem].iov_base=&longitud_str_net;
    iov[nelem++].iov_len=sizeof(int);
    iov[nelem].iov_base=string; // no se usa & porque ya es un puntero
    iov[nelem++].iov_len=longitud_str;

    // preparo el envío del entero 2 convirtiéndolo a formato de red
    int entero_net2 = htonl(offset);
    iov[nelem].iov_base=&entero_net2;
    iov[nelem++].iov_len=sizeof(int);

    // modo de operación de los sockets asegura que si no hay error
    // se enviará todo (misma semántica que los "pipes")
    if (writev(sckt, iov, 4)<0) {
        perror("error en writev");
        close(sckt);
        return -1;
    }
    int res;
    // recibe un entero como respuesta
    if (recv(sckt, &res, sizeof(int), MSG_WAITALL) != sizeof(int)) {
        perror("error en recv");
        close(sckt);
        return -1;
    }
    int x = ntohl(res);
    return x;
}

int msg_length(char *topic, int offset) {
    if(sckt == -1) init();
    return peticion_msg_length(topic, offset);
}
// Obtiene el último offset asociado a un tema en el broker, que corresponde
// al del último mensaje enviado más uno y, dado que los mensajes se
// numeran desde 0, coincide con el número de mensajes asociados a ese tema.
// Devuelve ese offset si OK y un valor negativo en caso de error.
static int peticion_end_offset(char *string) {
    struct iovec iov[3]; // hay que enviar 3 elementos
    int nelem = 0;
    // preparo el envío del entero convertido a formato de red
    int entero_net = htonl(5);
    iov[nelem].iov_base=&entero_net;
    iov[nelem++].iov_len=sizeof(int);

    // preparo el envío del string mandando antes su longitud
    int longitud_str = strlen(string); // no incluye el carácter nulo
    int longitud_str_net = htonl(longitud_str);
    iov[nelem].iov_base=&longitud_str_net;
    iov[nelem++].iov_len=sizeof(int);
    iov[nelem].iov_base=string; // no se usa & porque ya es un puntero
    iov[nelem++].iov_len=longitud_str;

    // modo de operación de los sockets asegura que si no hay error
    // se enviará todo (misma semántica que los "pipes")
    if (writev(sckt, iov, 3)<0) {
        perror("error en writev");
        close(sckt);
        return -1;
    }
    int res;
    // recibe un entero como respuesta
    if (recv(sckt, &res, sizeof(int), MSG_WAITALL) != sizeof(int)) {
        perror("error en recv");
        close(sckt);
        return -1;
    }
    return ntohl(res);
}

int end_offset(char *topic) {
    if(sckt == -1) init();
    return peticion_end_offset(topic);
}

// TERCERA FASE: SUBSCRIPCIÓN

// Se suscribe al conjunto de temas recibidos. No permite suscripción
// incremental: hay que especificar todos los temas de una vez.
// Si un tema no existe o está repetido en la lista simplemente se ignora.
// Devuelve el número de temas a los que realmente se ha suscrito
// y un valor negativo solo si ya estaba suscrito a algún tema.
int subscribe(int ntopics, char **topics) {
    if(sckt == -1) init();
    if (map_size(subscricpciones)!=0)return -1;
    if(ntopics == 0) return 0;
    subscripcion *ptr_subscripcion;
    int temas_aceptados = 0;

    for (int i = 0; i< ntopics; i++){
        int offset = end_offset(topics[i]);
        if( offset != -1){
            printf("Suscribiendo a tema %s\n", topics[i]);
            temas_aceptados ++;
            ptr_subscripcion=(subscripcion*)malloc(sizeof(subscripcion*));
            ptr_subscripcion->titular = strdup(topics[i]);
            ptr_subscripcion->offset_local= offset;
            map_put(subscricpciones, ptr_subscripcion->titular, ptr_subscripcion);
        }
    }
    printf("Temas aceptados: %i\n", temas_aceptados);
    if( temas_aceptados == 0)return -1;
    return temas_aceptados;
}

// Se da de baja de todos los temas suscritos.
// Devuelve 0 si OK y un valor negativo si no había suscripciones activas.
int unsubscribe(void) {
    if(sckt == -1) init();
    if (map_size(subscricpciones)==0)return -1;
    map_destroy(subscricpciones, NULL);
    subscricpciones = NULL;
    return 0;
}

// Devuelve el offset del cliente para ese tema y un número negativo en
// caso de error.
int position(char *topic) {
    if(sckt == -1) init();
    int error;
    subscripcion *ptr_subscripcion;
    ptr_subscripcion = map_get(subscricpciones, topic, &error);
    if (error==-1)return error;
    return ptr_subscripcion->offset_local;
}

// Modifica el offset del cliente para ese tema.
// Devuelve 0 si OK y un número negativo en caso de error.
int seek(char *topic, int offset) {
    if(sckt == -1) init();
    int error;
    subscripcion *ptr_subscripcion;
    ptr_subscripcion = map_get(subscricpciones, topic, &error);
    if (error!=-1) ptr_subscripcion->offset_local = offset;
    return error;
}

// CUARTA FASE: LEER MENSAJES

// Obtiene el siguiente mensaje destinado a este cliente; los dos parámetros
// son de salida.
// Devuelve el tamaño del mensaje (0 si no había mensaje)
// y un número negativo en caso de error.
int poll(char **topic, void **msg) {
    return 0;
}

// QUINTA FASE: COMMIT OFFSETS

// Cliente guarda el offset especificado para ese tema.
// Devuelve 0 si OK y un número negativo en caso de error.
int commit(char *client, char *topic, int offset) {
    return 0;
}

// Cliente obtiene el offset guardado para ese tema.
// Devuelve el offset y un número negativo en caso de error.
int commited(char *client, char *topic) {
    return 0;
}




#pragma clang diagnostic pop
