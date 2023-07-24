#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>


#include "map.h"
#include "queue.h"
#include "comun.h"



// función del thread
void *servicio(void *arg){
    int entero;
    int longitud;
    char *string;
    unsigned char *array_binario;
    thread_info *thinf = arg; // argumento recibido
    int res;

    // si recv devuelve <=0 el cliente ha cortado la conexión;
    // recv puede devolver menos datos de los solicitados
    // (misma semántica que el "pipe"), pero con MSG_WAITALL espera hasta que
    // se hayan recibido todos los datos solicitados o haya habido un error.
    while (1) {
        // cada "petición" comienza con un entero
        if (recv(thinf->socket, &entero, sizeof(int), MSG_WAITALL)!=sizeof(int))
            break;
        entero = ntohl(entero);
        printf("Recibido entero: %d\n", entero);
        switch (entero)
        {
        case 1:
            //DATOS RECIBIDOS:
            // llega el string, que viene precedido por su longitud
            if (recv(thinf->socket, &longitud, sizeof(int), MSG_WAITALL)!=sizeof(int))
                break;
            longitud = ntohl(longitud);
            string = malloc(longitud+1); // +1 para el carácter nulo
            // ahora sí llega el string
            if (recv(thinf->socket, string, longitud, MSG_WAITALL)!=longitud)
                break;
            string[longitud]='\0';       // añadimos el carácter nulo
            printf("Recibido string: %s\n", string);
            
            //ALGORITMO
            tema *nuevo_tema = (tema*) malloc(sizeof(tema*));
            nuevo_tema->titular= string;
            nuevo_tema->mensajes=queue_create(1); // sin cerrojos
            res = map_put(thinf->mapa_temas, nuevo_tema->titular, nuevo_tema);
                printf("Respuesta agregar_tema: %i\n", res);

            break;
        case 2:
            //ALGORITMO
            res = map_size(thinf->mapa_temas);
                printf("Respuesta ntemas: %i\n", res);

            break;
        case 3:
            //DATOS RECIBIDOS:
            // llega el string, que viene precedido por su longitud
            if (recv(thinf->socket, &longitud, sizeof(int), MSG_WAITALL)!=sizeof(int))break;
            longitud = ntohl(longitud);
            string = malloc(longitud+1); // +1 para el carácter nulo
            // ahora sí llega el string
            if (recv(thinf->socket, string, longitud, MSG_WAITALL)!=longitud)
                break;
            string[longitud]='\0';       // añadimos el carácter nulo
            printf("Recibido string: %s\n", string);

            // y finalmente llega el array binario precedido de su longitud
            if (recv(thinf->socket, &longitud, sizeof(int), MSG_WAITALL)!=sizeof(int))
                break;
            longitud = ntohl(longitud);
            array_binario = malloc(longitud); // no usa un terminador
            // llega el array
            if (recv(thinf->socket, array_binario, longitud, MSG_WAITALL)!=longitud)
                break;
            printf("Recibido array_binario: ");
            for (int i=0; i<longitud; i++) printf("%02x", array_binario[i]);
            printf("\n");

            //ALGORITMO
            int error = 0;
            tema *tema;
            mensaje *nuevo_mensaje;
            tema = map_get(thinf->mapa_temas, string, &error);
            if(error == -1){
                perror("Fallo al acceder al tema\n");
                res = -1;
            }
            else{
                nuevo_mensaje =(mensaje*) malloc(sizeof(int)+longitud*sizeof(char));
                nuevo_mensaje->mensaje = array_binario;
                nuevo_mensaje->tam_mensaje = longitud;
                res = queue_append(tema->mensajes, nuevo_mensaje);
            }
            break;
        case 4:
            //DATOS RECIBIDOS:
            // llega el string, que viene precedido por su longitud
            if (recv(thinf->socket, &longitud, sizeof(int), MSG_WAITALL)!=sizeof(int))
                break;
            longitud = ntohl(longitud);
            string = malloc(longitud+1); // +1 para el carácter nulo
            // ahora sí llega el string
            if (recv(thinf->socket, string, longitud, MSG_WAITALL)!=longitud)
                break;
            string[longitud]='\0';       // añadimos el carácter nulo
            printf("Recibido string: %s\n", string);
            // llega el entero
            if (recv(thinf->socket, &entero, sizeof(int), MSG_WAITALL)!=sizeof(int))
                break;
            entero = ntohl(entero);
            printf("Recibido entero: %d\n", entero);
            printf("El offset solicitado es %d y el tema es %s\n", entero, string);

            //ALGORITMO
            error = 0;
            printf("Ha entrado en el algoritmo");
            tema = map_get(thinf->mapa_temas, string, &error);
            if(error == -1){
                perror("Fallo al acceder al tema");
                res = error;
                break;
            }
            nuevo_mensaje = queue_get(tema->mensajes, entero, &error);
            if(error == -1) {
                perror("Fallo al acceder al mensaje");
                res = 0;
                break;
            }
            res = nuevo_mensaje->tam_mensaje;
            break;
        case 5:
            //DATOS RECIBIDOS:
            // llega el string, que viene precedido por su longitud
            if (recv(thinf->socket, &longitud, sizeof(int), MSG_WAITALL)!=sizeof(int))
                break;
            longitud = ntohl(longitud);
            string = malloc(longitud+1); // +1 para el carácter nulo
            // ahora sí llega el string
            if (recv(thinf->socket, string, longitud, MSG_WAITALL)!=longitud)
                break;
            string[longitud]='\0';       // añadimos el carácter nulo
            printf("Recibido string: %s\n", string);

            //ALGORITMO
            error = 0;
            tema = map_get(thinf->mapa_temas, string, &error);
            if(error == -1){
                perror("Fallo al acceder al tema");
                res = -1;
                break;
            }
            res = queue_size(tema->mensajes);

            break;
        default:
            break;
    }
        // envía un entero como respuesta
        res = htonl(res);
        write(thinf->socket, &res, sizeof(int));
    }
    close(thinf->socket);
    return NULL;
}
// inicializa el socket y lo prepara para aceptar conexiones
static int init_socket_server(const char * port) {
    int s;
    struct sockaddr_in dir;
    int opcion=1;
    // socket stream para Internet: TCP
    if ((s=socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("error creando socket");
        return -1;
    }
    // Para reutilizar puerto inmediatamente si se rearranca el servidor
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opcion, sizeof(opcion))<0){
        perror("error en setsockopt");
        return -1;
    }
    // asocia el socket al puerto especificado
    dir.sin_addr.s_addr=INADDR_ANY;
    dir.sin_port=htons(atoi(port));
    dir.sin_family=PF_INET;
    if (bind(s, (struct sockaddr *)&dir, sizeof(dir)) < 0) {
        perror("error en bind");
        close(s);
        return -1;
    }
    // Establece el número máx. de conexiones pendientes de aceptar
    if (listen(s, 5) < 0) {
        perror("error en listen");
        close(s);
        return -1;
    }
    return s;
}
int main(int argc, char *argv[]) {
    int s, s_conec;
    unsigned int tam_dir;
    struct sockaddr_in dir_cliente;
    map *mapa_temas;

    if (argc!=2 && argc!=3) {
        fprintf(stderr, "Uso: %s puerto [dir_commited]\n", argv[0]);
        return 1;
    }

    //Creo el mapa de temas
    mapa_temas = map_create(key_string, 1);

    // inicializa el socket y lo prepara para aceptar conexiones
    if ((s=init_socket_server(argv[1])) < 0) return -1;
    setenv("PUERTO",argv[1], 1);

    // prepara atributos adecuados para crear thread "detached"
    pthread_t thid;
    pthread_attr_t atrib_th;
    pthread_attr_init(&atrib_th); // evita pthread_join
    pthread_attr_setdetachstate(&atrib_th, PTHREAD_CREATE_DETACHED);

    while(1) {
        tam_dir=sizeof(dir_cliente);
        // acepta la conexión
        if ((s_conec=accept(s, (struct sockaddr *)&dir_cliente, &tam_dir))<0){
            perror("error en accept");
            close(s);
            return -1;
        }
        // crea el thread de servicio
        thread_info *thinf = malloc(sizeof(thread_info));
        thinf->socket=s_conec;
        thinf->mapa_temas=mapa_temas;
        pthread_create(&thid, &atrib_th, servicio, thinf);
    }
    close(s); // cierra el socket general
    return 0;
}
