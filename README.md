# redes-tf
## Implementación de protocolos y algoritmos de secuenciación
## Protocolo RCFTP
RCFTP es un protocolo que, usado sobre UDP, añade fiabilidad y secuenciación de datos. En nuestro
caso, el cliente enviará mensajes de acuerdo a las especificaciones correspondientes y el servidor le irá
confirmando la recepción correcta de los mensajes.

### Formato del mensaje

|   1 B   |  1 B  | 2 B |  4 B   | 4 B  | 2 B | 512 B  |
|:-------:|:-----:|:---:|:------:|:----:|:---:|:------:|
| Versión | Flags | Sum | NúmSeq | Next | Len | Buffer |


```c
struct rcftp_msg {
    uint8_t     version;    /* Versión RCFTP_VERSION_1; otro valor es inválido*/
    uint8_t     flags;      /* Flags. Mascara de bits de los defines F_X */
    uint16_t    sum;        /* Checksum en network format calculado con xsum */
    uint32_t    numseq;     /* Número de secuencia, medido en bytes */
    uint32_t    next;       /* Siguiente numseq esperado , medido en bytes */
    uint16_t    len;        /* Longitud de datos válidos en el campo buffer */
    uint8_t     buffer[RCFTP_BUFLEN];   /* Datos de tamaño fijo 512 B */
}
```

### Flags

`F_NOFLAGS`
    Flag por defecto
`F_BUSY`
    Flag de servidor ocupado atendiendo a otro cliente
`F_FIN`
    Flag de intención/confirmación de finalizar transmisiÓn
`F_ABORT`
    Flag de aviso de finalización forzosa


## Instalación

`Cliente`
```shell
./cliente/make
```

`Servidor`
```shell
./servidor/make
```

## Contenido
`rcftp.h` `rcftp.c`
Cabeceras y funciones del protocolo RCFTP.

`multialarm.h` `multialarm.c`
Cabeceras y funciones de gestión de temporizadores.

`rcftpclient.h` `rcftpclient.c`
Cabeceras, programa principal y varias funciones del cliente RCFTP.

`misfunciones.h` `misfunciones.c`
Definiciones y cabeceras de varias de las funciones a implementar.
Espacio para implementar las funciones que necesites.

## Uso
`cliente`
```shell
./rcftpclient [-v] -a[alg] [-t[Ttrans]] [-T[timeout]] [-w[tam]] -d<dirección> -p<puerto>
    -v              Muestra detalles en salida estándar
    -a[alg]         Algoritmo de secuenciación a utilizar:
    1               Algoritmo básico
    2               Algoritmo Stop&Wait
    3               Algoritmo de ventana deslizante Go-Back-n
    -w[tam]         Tamaño (en bytes) de la ventana de emisión (sólo usado con -a3) (por defecto: 2048)
    -t[Ttrans]      Tiempo de transmisión a simular, en microsegundos (por defecto: 200000)
    -T[timeout]     Tiempo de expiración a simular, en microsegundos (por defecto: 1000000)
    -d<dirección>   Dirección del servidor
    -p<puerto>      Servicio o número de puerto del servidor
```
`client example`
```shell
cat fichero.txt | ./rcftpclient -v -a1 -d155.210.152.183 -p32002
```

`servidor`
```shell
./rcftpd -p<puerto> [-v] [-a[alg]] [-e[frec]] [-t[Ttrans]] [-r[Tprop]]
    -p<puerto>      Especifica el servicio o número de puerto
    -v              Muestra detalles en salida estándar
    -a[alg]         Ajusta el comportamiento al algoritmo del cliente (por defecto: 0):
    0:              Sin mensajes incorrectos
    1:              Fuerza mensajes incorrectos hasta su corrección
    2:              Fuerza mensajes incorrectos/pérdidas/duplicados hasta su corrección
    3:              Fuerza mensajes incorrectos/pérdidas/duplicados
    -e[frec]        Fuerza en media un mensaje incorrecto de cada [frec] (por defecto: 5)
    -t[Ttrans]      Tiempo de transmisión a simular, en microsegundos (por defecto: 200000)
    -r[Tprop]       Tiempo de propagación a simular, en microsegundos (por defecto: 250000)
```
`client example`
```shell
./rcftpd -p32002 -v -a0  
```
**Nota**: Los algoritmos 1-3 generan errores aleatoriamente. Además, los algoritmos 1 y 2
mantienen el error generado hasta que el cliente responda correctamente.
