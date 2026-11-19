
/*Funciones del protocolo HTTP */

#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>

#define HTTP_OK 200
#define HTTP_BAD_REQUEST 400
#define HTTP_METHOD_NOT_ALLOWED 405

int procesarHTTP(const char *buf, size_t n,char *ruta, size_t capacidad);

#endif
