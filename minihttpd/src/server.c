
/*Servidor TCP */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "server.h"

#define PUERTO 8080
#define QLEN 32

int crearServidor(void)
{
    struct sockaddr_in sin;
    int msock;
    int qlen = QLEN;
    int opcion = 1;

    msock = socket(AF_INET, SOCK_STREAM, 0);

    if (msock < 0) {
        printf("Error en socket: %s\n", strerror(errno));
        return -1;
    }
    if (setsockopt(msock, SOL_SOCKET, SO_REUSEADDR,
                   &opcion, sizeof(opcion)) < 0) {
        printf("Error en setsockopt: %s\n", strerror(errno));
        close(msock);
        return -1;
    }

    memset(&sin, 0, sizeof(sin));

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sin.sin_port = htons(PUERTO);

    if (bind(msock, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        printf("Error en bind: %s\n", strerror(errno));
        close(msock);
        return -1;
    }
    if (listen(msock, qlen) < 0) {
        printf("Error en listen: %s\n", strerror(errno));
        close(msock);
        return -1;
    }
    printf("Servidor TCP preparado en puerto %d\n", PUERTO);
    return msock;
}
