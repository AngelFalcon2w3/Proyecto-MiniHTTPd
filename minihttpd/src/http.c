
/*Validacion de solicitudes HTTP */
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#include "http.h"

#define MAXLINEA 2048
#define MAXMETODO 16

int procesarHTTP(const char *buf, size_t n,char *ruta, size_t capacidad)
{
    char linea[MAXLINEA];
    char metodo[MAXMETODO];
    char version[16];
    char *p1;
    char *p2;
    char *separador;
    char *valor;
    size_t pos;
    size_t fin;
    size_t largo;
    size_t i;

    int tieneHost;
    int terminado;

    if (buf == NULL || ruta == NULL || capacidad == 0)
        return HTTP_BAD_REQUEST;

    ruta[0] = '\0';

    if (n < 4 || n > 8192)
        return HTTP_BAD_REQUEST;
    if (memchr(buf, '\0', n) != NULL)
        return HTTP_BAD_REQUEST;
    if (memcmp(buf + n - 4, "\r\n\r\n", 4) != 0)
        return HTTP_BAD_REQUEST;

    pos = 0;
    fin = 0;

    while (fin + 1 < n) {
        if (buf[fin] == '\r' && buf[fin+1] == '\n')
            break;
        fin++;
    }
    if (fin + 1 >= n || fin >= MAXLINEA)
        return HTTP_BAD_REQUEST;
    largo = fin;
    for (i = 0; i < largo; i++) {
        if ((unsigned char)buf[i] < 32 ||
            (unsigned char)buf[i] == 127)
            return HTTP_BAD_REQUEST;
    }
    memcpy(linea, buf, largo);
    linea[largo] = '\0';

    p1 = strchr(linea, ' ');

    if (p1 == NULL)
        return HTTP_BAD_REQUEST;

    *p1 = '\0';
    p1++;

    p2 = strchr(p1, ' ');

    if (p2 == NULL)
        return HTTP_BAD_REQUEST;

    *p2 = '\0';
    p2++;

    if (strlen(linea) == 0 ||
        strlen(linea) >= sizeof(metodo) ||
        strlen(p1) == 0 ||
        strlen(p1) >= capacidad ||
        strlen(p2) == 0 ||
        strlen(p2) >= sizeof(version))
        return HTTP_BAD_REQUEST;

    if (strchr(p2, ' ') != NULL)
        return HTTP_BAD_REQUEST;

    memcpy(metodo, linea, strlen(linea) + 1);
    memcpy(ruta, p1, strlen(p1) + 1);
    memcpy(version, p2, strlen(p2) + 1);

    for (i = 0; metodo[i] != '\0'; i++) {
        if (!isalpha((unsigned char)metodo[i]))
            return HTTP_BAD_REQUEST;
    }
    if (strcmp(version, "HTTP/1.1") != 0)
        return HTTP_BAD_REQUEST;
    if (ruta[0] != '/')
        return HTTP_BAD_REQUEST;
    for (i = 0; ruta[i] != '\0'; i++) {
        if ((unsigned char)ruta[i] <= 32 ||
            (unsigned char)ruta[i] == 127)
            return HTTP_BAD_REQUEST;
    }

    printf("\n--- Solicitud HTTP ---\n");
    printf("Metodo: %s\n", metodo);
    printf("Ruta: %s\n", ruta);
    printf("Version: %s\n", version);

    pos = fin + 2;
    tieneHost = 0;
    terminado = 0;

    while (pos < n) {
        fin = pos;
        while (fin + 1 < n) {
            if (buf[fin] == '\r' && buf[fin+1] == '\n')
                break;
            fin++;
        }
        if (fin + 1 >= n || fin - pos >= MAXLINEA)
            return HTTP_BAD_REQUEST;

        largo = fin - pos;
        if (largo == 0) {
            if (fin + 2 != n)
                return HTTP_BAD_REQUEST;

            terminado = 1;
            break;
        }

        memcpy(linea, buf + pos, largo);
        linea[largo] = '\0';

        for (i = 0; i < largo; i++) {
            if ((unsigned char)linea[i] < 32 &&
                linea[i] != '\t')
                return HTTP_BAD_REQUEST;
            if ((unsigned char)linea[i] == 127)
                return HTTP_BAD_REQUEST;
        }

        separador = strchr(linea, ':');

        if (separador == NULL || separador == linea)
            return HTTP_BAD_REQUEST;
        if (separador[-1] == ' ' ||
            separador[-1] == '\t')
            return HTTP_BAD_REQUEST;
        for (i = 0; linea + i < separador; i++) {
            if (!isalnum((unsigned char)linea[i]) &&
                linea[i] != '-' && linea[i] != '_')
                return HTTP_BAD_REQUEST;
        }

        *separador = '\0';
        valor = separador + 1;

        while (*valor == ' ' || *valor == '\t')
            valor++;

        if (strcasecmp(linea, "Host") == 0) {
            if (tieneHost || strlen(valor) == 0)
                return HTTP_BAD_REQUEST;
            tieneHost = 1;
            printf("Host: %s\n", valor);
        }
        else if (strcasecmp(linea, "Connection") == 0) {
            printf("Connection: %s\n", valor);
        }
        else if (strcasecmp(linea, "User-Agent") == 0) {
            printf("User-Agent: %s\n", valor);
        }
        pos = fin + 2;
    }
    if (!terminado || !tieneHost)
        return HTTP_BAD_REQUEST;
    if (strcmp(metodo, "GET") != 0) {
        printf("Metodo no permitido: %s\n", metodo);
        return HTTP_METHOD_NOT_ALLOWED;
    }
    printf("Solicitud GET valida\n");
    return HTTP_OK;
}
