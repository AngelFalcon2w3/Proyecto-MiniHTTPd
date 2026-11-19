#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "mime.h"

const char *obtenerMIME(const char *ruta)
{
    const char *extension;
    extension = strrchr(ruta, '.');

    if (extension == NULL)
        return "application/octet-stream";
    if (strcasecmp(extension, ".html") == 0)
        return "text/html";
    if (strcasecmp(extension, ".css") == 0)
        return "text/css";
    if (strcasecmp(extension, ".js") == 0)
        return "application/javascript";
    if (strcasecmp(extension, ".png") == 0)
        return "image/png";
    if (strcasecmp(extension, ".jpg") == 0 ||
        strcasecmp(extension, ".jpeg") == 0)
        return "image/jpeg";
    if (strcasecmp(extension, ".svg") == 0)
        return "image/svg+xml";
    if (strcasecmp(extension, ".ico") == 0)
        return "image/x-icon";
    if (strcasecmp(extension, ".txt") == 0)
        return "text/plain";
    return "application/octet-stream";
}
