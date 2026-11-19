
/*MiniHTTPd */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include "server.h"
#include "http.h"
#include "files.h"
#include "mime.h"

#define MAXEVENTOS 32
#define MAXSOLICITUD 8192
#define MAXCABECERA 512

#define RECIBIENDO 0
#define ENVIANDO 1

struct Cliente {
    int fd;
    int estado;
    int cerrarDespues;

    char buf[MAXSOLICITUD + 1];
    size_t usado;
    size_t consumido;

    char cabecera[MAXCABECERA];
    size_t totalCabecera;
    size_t enviadoCabecera;

    unsigned char *archivo;
    const unsigned char *cuerpo;
    size_t tamano;
    size_t enviado;
};

int noBloqueante(int fd);
int cambiarEvento(int epfd, struct Cliente *cliente, int tipo);
size_t finEncabezados(const char *buf, size_t n);
int pideCerrar(const char *buf, size_t n);
int prepararRespuesta(struct Cliente *cliente, size_t fin, int forzar400);
int revisarSolicitud(struct Cliente *cliente);
int enviarRespuesta(struct Cliente *cliente);
void limpiarRespuesta(struct Cliente *cliente);
void cerrarCliente(int epfd, struct Cliente *cliente);

int noBloqueante(int fd)
{
    int flags;
    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
        return -1;
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
        return -1;
    return 0;
}

int cambiarEvento(int epfd, struct Cliente *cliente, int tipo)
{
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = tipo | EPOLLRDHUP;
    ev.data.ptr = cliente;
    return epoll_ctl(epfd, EPOLL_CTL_MOD, cliente->fd, &ev);
}

size_t finEncabezados(const char *buf, size_t n)
{
    size_t i;
    for (i = 0; i + 3 < n; i++) {

        if (buf[i] == '\r' && buf[i+1] == '\n' &&
            buf[i+2] == '\r' && buf[i+3] == '\n')
            return i + 4;
    }
    return 0;
}

int pideCerrar(const char *buf, size_t n)
{
    size_t pos;
    size_t fin;
    size_t ini;
    size_t j;
    size_t a;
    size_t b;
    pos = 0;

    while (pos + 1 < n) {
        if (buf[pos] == '\r' && buf[pos+1] == '\n')
            break;
        pos++;
    }

    pos += 2;

    while (pos + 1 < n) {
        fin = pos;
        while (fin + 1 < n) {
            if (buf[fin] == '\r' && buf[fin+1] == '\n')
                break;
            fin++;
        }
        if (fin + 1 >= n || fin == pos)
            break;
        j = pos;
        while (j < fin && buf[j] != ':')
            j++;
        if (j - pos == 10 &&
            strncasecmp(buf + pos, "Connection", 10) == 0) {
            ini = j + 1;
            while (ini < fin) {
                a = ini;
                while (a < fin &&
                       (buf[a] == ' ' || buf[a] == '\t'))
                    a++;
                b = a;
                while (b < fin && buf[b] != ',')
                    b++;
                j = b;
                while (j > a &&
                       (buf[j-1] == ' ' || buf[j-1] == '\t'))
                    j--;
                if (j - a == 5 &&
                    strncasecmp(buf + a, "close", 5) == 0)
                    return 1;
                ini = b + 1;
            }
        }
        pos = fin + 2;
    }

    return 0;
}

void limpiarRespuesta(struct Cliente *cliente)
{
    free(cliente->archivo);
    cliente->archivo = NULL;
    cliente->cuerpo = NULL;
    cliente->tamano = 0;
    cliente->enviado = 0;
    cliente->totalCabecera = 0;
    cliente->enviadoCabecera = 0;
}

int prepararRespuesta(struct Cliente *cliente,
                      size_t fin, int forzar400)
{
    char ruta[1024];
    const char *estadoHTTP;
    const char *tipo;
    const char *extra;
    const char *conexion;
    int codigo;
    int resultado;
    int n;

    limpiarRespuesta(cliente);
    cliente->cerrarDespues = 1;
    tipo = "text/plain; charset=utf-8";
    extra = "";

    if (forzar400)
        codigo = HTTP_BAD_REQUEST;
    else
        codigo = procesarHTTP(cliente->buf, fin,ruta, sizeof(ruta));
    if (codigo == HTTP_BAD_REQUEST) {
        estadoHTTP = "400 Bad Request";
        cliente->cuerpo =
            (const unsigned char *)"Solicitud incorrecta\n";
        cliente->tamano = strlen((const char *)cliente->cuerpo);
    }
    else if (codigo == HTTP_METHOD_NOT_ALLOWED) {
        estadoHTTP = "405 Method Not Allowed";
        extra = "Allow: GET\r\n";
        cliente->cuerpo =
            (const unsigned char *)"Metodo no permitido\n";
        cliente->tamano = strlen((const char *)cliente->cuerpo);
    }
    else if (codigo == HTTP_OK) {
        cliente->cerrarDespues = pideCerrar(cliente->buf, fin);
        resultado = leerArchivo(ruta,
                                &cliente->archivo,
                                &cliente->tamano);

        if (resultado == ARCHIVO_OK) {
            estadoHTTP = "200 OK";
            if (strcmp(ruta, "/") == 0)
                tipo = "text/html";
            else
                tipo = obtenerMIME(ruta);

            cliente->cuerpo = cliente->archivo;
        }
        else if (resultado == ARCHIVO_NO_EXISTE) {
            estadoHTTP = "404 Not Found";
            cliente->cuerpo =
                (const unsigned char *)"Archivo no encontrado\n";
            cliente->tamano = strlen((const char *)cliente->cuerpo);
        }
        else if (resultado == ARCHIVO_PROHIBIDO) {

            estadoHTTP = "403 Forbidden";
            cliente->cuerpo =
                (const unsigned char *)"Acceso prohibido\n";
            cliente->tamano = strlen((const char *)cliente->cuerpo);
        }

        else {
            estadoHTTP = "500 Internal Server Error";
            cliente->cerrarDespues = 1;
            cliente->cuerpo =
                (const unsigned char *)"Error interno del servidor\n";
            cliente->tamano = strlen((const char *)cliente->cuerpo);
        }
    }

    else {
        return -1;
    }
    if (cliente->cerrarDespues)
        conexion = "close";
    else
        conexion = "keep-alive";
    n = snprintf(cliente->cabecera,
                 sizeof(cliente->cabecera),
                 "HTTP/1.1 %s\r\n"
                 "Content-Type: %s\r\n"
                 "Content-Length: %zu\r\n"
                 "%s"
                 "Connection: %s\r\n"
                 "\r\n",
                 estadoHTTP, tipo, cliente->tamano,
                 extra, conexion);
    if (n < 0 || (size_t)n >= sizeof(cliente->cabecera))
        return -1;

    cliente->totalCabecera = (size_t)n;
    cliente->enviadoCabecera = 0;
    cliente->enviado = 0;
    cliente->consumido = fin;
    cliente->estado = ENVIANDO;
    printf("Respuesta: %s - Connection: %s\n",
           estadoHTTP, conexion);
    return 0;
}

int revisarSolicitud(struct Cliente *cliente)
{
    size_t fin;
    fin = finEncabezados(cliente->buf, cliente->usado);
    if (fin > 0) {
        if (prepararRespuesta(cliente, fin, 0) < 0)
            return -1;
        return 1;
    }
    if (cliente->usado == MAXSOLICITUD) {
        if (prepararRespuesta(cliente, 0, 1) < 0)
            return -1;
        return 1;
    }
    return 0;
}

int enviarRespuesta(struct Cliente *cliente)
{
    ssize_t cc;
    while (cliente->enviadoCabecera < cliente->totalCabecera) {
        cc = send(cliente->fd,
                  cliente->cabecera + cliente->enviadoCabecera,
                  cliente->totalCabecera - cliente->enviadoCabecera,
                  MSG_NOSIGNAL);
        if (cc > 0)
            cliente->enviadoCabecera += (size_t)cc;
        else if (cc < 0 && errno == EINTR)
            continue;
        else if (cc < 0 &&
                 (errno == EAGAIN || errno == EWOULDBLOCK))
            return 0;
        else
            return -1;
    }
    while (cliente->enviado < cliente->tamano) {
        cc = send(cliente->fd,
                  cliente->cuerpo + cliente->enviado,
                  cliente->tamano - cliente->enviado,
                  MSG_NOSIGNAL);
        if (cc > 0)
            cliente->enviado += (size_t)cc;
        else if (cc < 0 && errno == EINTR)
            continue;
        else if (cc < 0 &&
                 (errno == EAGAIN || errno == EWOULDBLOCK))
            return 0;
        else
            return -1;
    }

    return 1;
}

void cerrarCliente(int epfd, struct Cliente *cliente)
{
    epoll_ctl(epfd, EPOLL_CTL_DEL, cliente->fd, NULL);
    printf("Cliente desconectado: %d\n", cliente->fd);
    close(cliente->fd);
    limpiarRespuesta(cliente);
    free(cliente);
}

int main(void)
{
    struct sockaddr_in fsin;
    struct epoll_event ev;
    struct epoll_event eventos[MAXEVENTOS];
    struct Cliente *cliente;
    socklen_t alen;
    int msock;
    int ssock;
    int epfd;
    int nfds;
    int i;
    int cerrar;
    int resultado;
    ssize_t cc;
    size_t restantes;

    msock = crearServidor();

    if (msock < 0) {
        printf("Error al iniciar servidor\n");
        return 1;
    }
    if (noBloqueante(msock) < 0) {
        close(msock);
        return 1;
    }

    epfd = epoll_create1(0);

    if (epfd < 0) {
        close(msock);
        return 1;
    }

    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.ptr = NULL;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, msock, &ev) < 0) {
        close(epfd);
        close(msock);
        return 1;
    }

    printf("MiniHTTPd - Conexiones persistentes\n");
    printf("Esperando conexiones en puerto 8080...\n");

    while (1) {
        nfds = epoll_wait(epfd, eventos, MAXEVENTOS, -1);
        if (nfds < 0) {
            if (errno == EINTR)
                continue;
            printf("Error en epoll_wait: %s\n", strerror(errno));
            break;
        }
        for (i = 0; i < nfds; i++) {
            if (eventos[i].data.ptr == NULL) {
                while (1) {
                    alen = sizeof(fsin);
                    ssock = accept(msock,(struct sockaddr *)&fsin,&alen);
                    if (ssock < 0) {
                        if (errno == EINTR)
                            continue;
                        if (errno == EAGAIN ||
                            errno == EWOULDBLOCK)
                            break;
                        printf("Error en accept: %s\n",strerror(errno));
                        break;
                    }
                    if (noBloqueante(ssock) < 0) {
                        close(ssock);
                        continue;
                    }
                    cliente = calloc(1, sizeof(struct Cliente));

                    if (cliente == NULL) {
                        close(ssock);
                        continue;
                    }

                    cliente->fd = ssock;
                    cliente->estado = RECIBIENDO;

                    memset(&ev, 0, sizeof(ev));

                    ev.events = EPOLLIN | EPOLLRDHUP;
                    ev.data.ptr = cliente;

                    if (epoll_ctl(epfd, EPOLL_CTL_ADD,ssock, &ev) < 0) {
                        close(ssock);
                        free(cliente);
                        continue;
                    }
                    printf("Cliente conectado: %d\n", ssock);
                }
            }
            else {
                cliente = eventos[i].data.ptr;
                cerrar = 0;
                if (eventos[i].events & (EPOLLERR | EPOLLHUP))
                    cerrar = 1;
                if (!cerrar && cliente->estado == RECIBIENDO && (eventos[i].events & EPOLLIN)) {
                    while (1) {
                        resultado = revisarSolicitud(cliente);
                        if (resultado < 0) {
                            cerrar = 1;
                            break;
                        }
                        if (resultado == 1) {
                            if (cambiarEvento(epfd, cliente,EPOLLOUT) < 0)
                                cerrar = 1;
                            break;
                        }
                        cc = read(cliente->fd,cliente->buf + cliente->usado,
                                  MAXSOLICITUD - cliente->usado);
                        if (cc > 0) {
                            cliente->usado += (size_t)cc;
                            cliente->buf[cliente->usado] = '\0';
                        }
                        else if (cc == 0) {
                            cerrar = 1;
                            break;
                        }
                        else if (errno == EINTR) {
                            continue;
                        }
                        else if (errno == EAGAIN ||
                                 errno == EWOULDBLOCK) {
                            break;
                        }
                        else {
                            cerrar = 1;
                            break;
                        }
                    }
                }

                if (!cerrar && cliente->estado == ENVIANDO &&
                    (eventos[i].events & EPOLLOUT)) {
                    resultado = enviarRespuesta(cliente);
                    if (resultado < 0)
                        cerrar = 1;
                    else if (resultado == 1) {
                        printf("Respuesta enviada: socket %d\n",cliente->fd);
                        if (cliente->cerrarDespues) {
                            cerrar = 1;
                        }
                        else {
                            restantes = cliente->usado - cliente->consumido;
                            memmove(cliente->buf,cliente->buf + cliente->consumido,restantes);

                            cliente->usado = restantes;
                            cliente->buf[restantes] = '\0';
                            limpiarRespuesta(cliente);
                            cliente->estado = RECIBIENDO;
                            cliente->consumido = 0;

                            resultado = revisarSolicitud(cliente);

                            if (resultado < 0) {
                                cerrar = 1;
                            }
                            else if (resultado == 1) {
                                if (cambiarEvento(epfd, cliente,EPOLLOUT) < 0)
                                    cerrar = 1;
                            }
                            else {
                                if (cambiarEvento(epfd, cliente,EPOLLIN) < 0)
                                    cerrar = 1;
                            }
                        }
                    }
                }
                if (!cerrar && cliente->estado == RECIBIENDO && (eventos[i].events & EPOLLRDHUP)) {
                    if (cliente->usado == 0)
                        cerrar = 1;
                }
                if (cerrar)
                    cerrarCliente(epfd, cliente);
            }
        }
    }
    close(epfd);
    close(msock);
    return 0;
}
