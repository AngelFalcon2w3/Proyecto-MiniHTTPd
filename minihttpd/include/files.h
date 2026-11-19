
/*Funciones para leer archivos */

#ifndef FILES_H
#define FILES_H

#include <stddef.h>

#define ARCHIVO_OK 0
#define ARCHIVO_NO_EXISTE 1
#define ARCHIVO_PROHIBIDO 2
#define ARCHIVO_ERROR 3

int leerArchivo(const char *ruta,unsigned char **datos,size_t *tamano);

#endif
