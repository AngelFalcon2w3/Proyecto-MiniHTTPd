# Proyecto Integrador MiniHTTPd

Este proyecto implementa un **servidor web HTTP/1.1 básico en lenguaje C**, desarrollado para Linux (Ubuntu) sin utilizar bibliotecas HTTP externas. Utiliza **sockets TCP** para la comunicación, **`epoll`** para administrar múltiples clientes y un sistema de lectura de archivos estáticos para responder solicitudes realizadas desde un navegador o una herramienta como `curl`.

El servidor se ha probado con el sitio web estático **Centro Médico Vida Saludable**, compuesto por HTML, CSS, JavaScript e imágenes.

## Descripción del Proyecto

El objetivo principal es comprender el funcionamiento de un servidor HTTP. MiniHTTPd recibe conexiones TCP, interpreta solicitudes `GET`, analiza encabezados HTTP/1.1 y entrega archivos almacenados en la carpeta `www/` con el tipo MIME y código de estado correspondientes.

### Funcionalidades Principales

* **Servidor TCP:** creación del socket, asociación al puerto 8080 y escucha de conexiones mediante `socket()`, `bind()`, `listen()` y `accept()`.
* **Multiplexación con `epoll`:** atención de varios clientes mediante eventos de lectura (`EPOLLIN`) y escritura (`EPOLLOUT`) y sockets no bloqueantes.
* **Solicitudes HTTP/1.1:** procesamiento del método `GET` y análisis de los encabezados `Host`, `Connection` y `User-Agent`.
* **Conexiones persistentes:** soporte básico de keep-alive para realizar varias solicitudes mediante un mismo socket TCP, cierre cuando se recibe `Connection: close`.
* **Archivos estáticos:** envío de HTML, CSS, JavaScript, PNG y JPG desde `www/`.
* **Tipos MIME:** identificación de `text/html`, `text/css`, `application/javascript`, `image/png` e `image/jpeg`.
* **Códigos de estado:** respuestas `200 OK`, `400 Bad Request`, `403 Forbidden`, `404 Not Found`, `405 Method Not Allowed` y `500 Internal Server Error` para los casos implementados.
* **Validaciones de seguridad:** comprobaciones de tamaño de solicitudes y rutas, uso de `realpath()` para restringir el acceso a `www/`

## Arquitectura del Servidor

El funcionamiento de MiniHTTPd sigue estas etapas:

1. **Conexión TCP:** `server.c` configura el socket y escucha en `127.0.0.1:8080`.
2. **Gestión de eventos:** `main.c` registra los sockets y atiende eventos mediante `epoll`.
3. **Análisis HTTP:** `http.c` interpreta la línea de solicitud y comprueba los encabezados.
4. **Acceso a archivos:** `files.c` localiza y lee archivos dentro de `www/`.
5. **Identificación MIME:** `mime.c` determina el tipo de contenido según la extensión.
6. **Respuesta HTTP:** el servidor envía la línea de estado, los encabezados y el contenido del archivo, después cierra o reutiliza la conexión según corresponda.

## Instalación y Configuración

### Prerrequisitos

* Ubuntu o una distribución Linux compatible con `epoll`.
* Compilador GCC.
* Herramienta `make`.
* Navegador web y `curl` para las pruebas.

En Ubuntu podemos instalar estas herramientas con:

```bash
sudo apt update
sudo apt install build-essential curl 
```

### 1. Obtener el proyecto

Si el proyecto esta en el equipo de manera  local:

```bash
cd ~/minihttpd
```

Como el repositorio esté publicado en GitHub, se podrá clonar utilizando su URL real.

### 2. Compilar con Makefile

Desde la carpeta principal del proyecto:

```bash
make
```
Como alternativa, se puede compilar directamente con GCC:

```bash
gcc -Wall -Wextra -Iinclude src/main.c src/server.c src/http.c src/files.c src/mime.c -o minihttpd
```

## Ejecución del Servidor

Inicia MiniHTTPd desde el directorio principal, ya que los archivos estáticos se buscan en la carpeta relativa `www/`:

```bash
cd ~/minihttpd
./minihttpd
```

El servidor escucha localmente en el puerto **8080**. Abre:

```text
http://127.0.0.1:8080/
```

## Estructura del Proyecto

```text
minihttpd/
├── Makefile                # Compilación del servidor
├── README.md               # Documentación del proyecto
├── include/
│   ├── server.h            # Declaraciones del servidor TCP
│   ├── http.h              # Declaraciones y códigos de HTTP
│   ├── files.h             # Declaraciones de lectura de archivos
│   └── mime.h              # Declaración de tipos MIME
├── src/
│   ├── main.c              # Bucle de eventos epoll y envío HTTP
│   ├── server.c            # Creación y configuración del socket TCP
│   ├── http.c              # Análisis y validación de solicitudes
│   ├── files.c             # Lectura y verificación de rutas en www/
│   └── mime.c              # Identificación del tipo de contenido
└── www/
    ├── index.html          # Página principal del hospital
    ├── styles.css          # Hoja de estilos del sitio
    ├── main.js             # Funciones JavaScript del sitio
    ├── logo.png           # Imagen PNG de prueba
    └── hospital.jpg           # Imagen JPG de prueba
```


## Ejemplos de Uso y Pruebas

### Ejemplo 1: Solicitar la página HTML

```bash
curl -i http://127.0.0.1:8080/index.html
```

**Resultado esperado:** `HTTP/1.1 200 OK` y `Content-Type: text/html`.

### Ejemplo 2: Comprobar CSS y JavaScript

```bash
curl -s -D - -o /dev/null http://127.0.0.1:8080/styles.css
curl -s -D - -o /dev/null http://127.0.0.1:8080/main.js
```

**Resultados esperados:** `200 OK`, con tipos `text/css` y `application/javascript`.

### Ejemplo 3: Comprobar PNG y JPG

```bash
curl -s -D - -o /dev/null http://127.0.0.1:8080/logo.png
curl -s -D - -o /dev/null http://127.0.0.1:8080/hospital.jpg
```

**Resultados esperados:** `200 OK`, con `image/png` e `image/jpeg`.

### Ejemplo 4: Comprobar códigos de error

```bash
# Archivo inexistente: 404
curl -i http://127.0.0.1:8080/noexiste.html

# Método no admitido: 405
curl -i -X POST http://127.0.0.1:8080/index.html

# Intento de salir del directorio www/: 403
curl --path-as-is -i http://127.0.0.1:8080/../../etc/passwd

# Solicitud HTTP/1.1 sin Host: 400
printf 'GET /index.html HTTP/1.1\r\nConnection: close\r\n\r\n' | nc -N 127.0.0.1 8080
```

Para verificar `500 Internal Server Error`, se utilizó un archivo de prueba mayor que el límite configurado de 4 MiB y posteriormente se eliminó.

### Ejemplo 5: Conexiones persistentes

```bash
curl -v -o /dev/null http://127.0.0.1:8080/index.html \
  -o /dev/null http://127.0.0.1:8080/styles.css \
  -o /dev/null http://127.0.0.1:8080/main.js
```

**Resultado esperado:** `curl` reutiliza la conexión TCP para atender varias solicitudes HTTP/1.1.

### Ejemplo 6: Concurrencia mediante epoll

Se realizaron pruebas con varias conexiones TCP simultáneas utilizando Netcat. En el código, `epoll_create1()`, `epoll_ctl()` y `epoll_wait()` permiten administrar los eventos de los sockets sin crear un hilo por cliente.

## Consideraciones de Seguridad y Alcance

* MiniHTTPd admite el método `GET`, otros métodos reciben `405 Method Not Allowed` en los casos validados.
* La implementación incorpora un límite general de solicitud de 8192 bytes y validaciones de línea, URI y encabezados.
* Se utiliza `realpath()` para comprobar que las rutas solicitadas permanezcan dentro de `www/`.
* El servidor escucha en `127.0.0.1`, por lo que está destinado a pruebas locales.
* Se trata de un servidor didáctico, no incorpora HTTPS, autenticación, ni procesamiento real de reservas médicas.

## Autor

* Angel Falcon 