
/*Lectura de archivos del servidor */

#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>

#include "files.h"

#define MAXARCHIVO (4 * 1024 * 1024)

int rutaProhibida(const char *ruta)
{
    size_t i;
    size_t inicio;
    size_t largo;

    if (ruta == NULL || ruta[0] != '/')
        return 1;

    inicio = 1;
    i = 1;

    while (1) {

        if (ruta[i] == '/' || ruta[i] == '\0') {

            largo = i - inicio;

            if (largo == 1 &&
                ruta[inicio] == '.')
                return 1;

            if (largo == 2 &&
                ruta[inicio] == '.' &&
                ruta[inicio + 1] == '.')
                return 1;

            if (largo == 0 && i > 1)
                return 1;

            if (ruta[i] == '\0')
                break;

            inicio = i + 1;
        }

        i++;
    }

    return 0;
}

int leerArchivo(const char *ruta,unsigned char **datos,size_t *tamano)
{
    FILE *archivo;
    struct stat info;

    char raiz[PATH_MAX];
    char nombre[PATH_MAX];
    char real[PATH_MAX];

    unsigned char *buf;

    size_t largo;
    size_t leidos;
    size_t i;

    long n;
    int resultado;

    if (datos == NULL || tamano == NULL)
        return ARCHIVO_ERROR;

    *datos = NULL;
    *tamano = 0;

    if (ruta == NULL || ruta[0] != '/')
        return ARCHIVO_PROHIBIDO;

    for (i = 0; ruta[i] != '\0'; i++) {

        if (ruta[i] == '?' || ruta[i] == '#' ||
            ruta[i] == '%' || ruta[i] == '\\' ||
            (unsigned char)ruta[i] < 32 ||
            (unsigned char)ruta[i] == 127)
            return ARCHIVO_PROHIBIDO;
    }

    if (rutaProhibida(ruta))
        return ARCHIVO_PROHIBIDO;

    if (realpath("www", raiz) == NULL) {
        printf("Error en carpeta www: %s\n",strerror(errno));
        return ARCHIVO_ERROR;
    }

    if (strcmp(ruta, "/") == 0)
        ruta = "/index.html";

    resultado = snprintf(nombre, sizeof(nombre),"%s%s", raiz, ruta);

    if (resultado < 0 ||
        (size_t)resultado >= sizeof(nombre))
        return ARCHIVO_PROHIBIDO;

    if (realpath(nombre, real) == NULL) {

        if (errno == ENOENT || errno == ENOTDIR)
            return ARCHIVO_NO_EXISTE;

        if (errno == EACCES || errno == EPERM ||
            errno == ELOOP)
            return ARCHIVO_PROHIBIDO;

        return ARCHIVO_ERROR;
    }

    largo = strlen(raiz);

    if (strncmp(real, raiz, largo) != 0 ||
        real[largo] != '/')
        return ARCHIVO_PROHIBIDO;

    if (stat(real, &info) < 0) {

        if (errno == EACCES || errno == EPERM)
            return ARCHIVO_PROHIBIDO;

        return ARCHIVO_ERROR;
    }

    if (!S_ISREG(info.st_mode))
        return ARCHIVO_PROHIBIDO;

    if (info.st_size < 0 ||
        info.st_size > MAXARCHIVO)
        return ARCHIVO_ERROR;

    archivo = fopen(real, "rb");

    if (archivo == NULL) {

        if (errno == EACCES || errno == EPERM)
            return ARCHIVO_PROHIBIDO;

        return ARCHIVO_ERROR;
    }

    if (fseek(archivo, 0, SEEK_END) != 0) {
        fclose(archivo);
        return ARCHIVO_ERROR;
    }

    n = ftell(archivo);

    if (n < 0 || n > MAXARCHIVO) {
        fclose(archivo);
        return ARCHIVO_ERROR;
    }

    if (fseek(archivo, 0, SEEK_SET) != 0) {
        fclose(archivo);
        return ARCHIVO_ERROR;
    }

    buf = malloc((size_t)n + 1);

    if (buf == NULL) {
        fclose(archivo);
        return ARCHIVO_ERROR;
    }

    leidos = fread(buf, 1, (size_t)n, archivo);

    if (leidos != (size_t)n) {
        free(buf);
        fclose(archivo);
        return ARCHIVO_ERROR;
    }

    buf[leidos] = '\0';

    fclose(archivo);

    *datos = buf;
    *tamano = leidos;

    return ARCHIVO_OK;
}
