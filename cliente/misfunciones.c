/****************************************************************************/
/* Plantilla para implementación de funciones del cliente (rcftpclient)     */
/* $Revision$ */
/* Aunque se permite la modificación de cualquier parte del código, se */
/* recomienda modificar solamente este fichero y su fichero de cabeceras asociado. */
/****************************************************************************/

/**************************************************************************/
/* INCLUDES                                                               */
/**************************************************************************/
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include "rcftp.h"		 // Protocolo RCFTP
#include "rcftpclient.h" // Funciones ya implementadas
#include "multialarm.h"	 // Gestión de timeouts
#include "misfunciones.h"
#include "vemision.h"

/**************************************************************************/
/* VARIABLES GLOBALES                                                     */
/**************************************************************************/
// elegir 1 o 2 autores y sustituir "Apellidos, Nombre" manteniendo el formato
// char* autores="Autor: Apellidos, Nombre"; // un solo autor
char *autores = "Autor: Toral Pallás, Héctor\nAutor: Pizarro Martinez, Francisco Javier"; // dos autores

// variable para indicar si mostrar información extra durante la ejecución
// como la mayoría de las funciones necesitaran consultarla, la definimos global
extern char verb;

// variable externa que muestra el número de timeouts vencidos
// Uso: Comparar con otra variable inicializada a 0; si son distintas, tratar un timeout e incrementar en uno la otra variable
extern volatile const int timeouts_vencidos;

/**************************************************************************/
/* Obtiene la estructura de direcciones del servidor */
/**************************************************************************/
struct addrinfo *obtener_struct_direccion(char *dir_servidor, char *servicio, char f_verbose)
{
	struct addrinfo hints;		// variable para especificar la solicitud
	struct addrinfo *servinfo;	// puntero para respuesta de getaddrinfo()
	struct addrinfo *direccion; // puntero para recorrer la lista de direcciones de servinfo
	int status;					// finalización correcta o no de la llamada getaddrinfo()
	int numdir = 1;				// contador de estructuras de direcciones en la lista de direcciones de servinfo

	// sobreescribimos con ceros la estructura para borrar cualquier dato que pueda malinterpretarse
	memset(&hints, 0, sizeof hints);

	// genera una estructura de dirección con especificaciones de la solicitud
	if (f_verbose)
	{
		printf("1 - Especificando detalles de la estructura de direcciones a solicitar... \n");
		fflush(stdout);
	}

	hints.ai_family = AF_UNSPEC; // opciones: AF_UNSPEC; IPv4: AF_INET; IPv6: AF_INET6; etc.

	if (f_verbose)
	{
		printf("\tFamilia de direcciones/protocolos: ");
		switch (hints.ai_family)
		{
		case AF_UNSPEC:
			printf("IPv4 e IPv6\n");
			break;
		case AF_INET:
			printf("IPv4)\n");
			break;
		case AF_INET6:
			printf("IPv6)\n");
			break;
		default:
			printf("No IP (%d)\n", hints.ai_family);
			break;
		}
		fflush(stdout);
	}

	hints.ai_socktype = SOCK_DGRAM; // especificar tipo de socket

	if (f_verbose)
	{
		printf("\tTipo de comunicación: ");
		switch (hints.ai_socktype)
		{
		case SOCK_STREAM:
			printf("flujo (TCP)\n");
			break;
		case SOCK_DGRAM:
			printf("datagrama (UDP)\n");
			break;
		default:
			printf("no convencional (%d)\n", hints.ai_socktype);
			break;
		}
		fflush(stdout);
	}

	// flags específicos dependiendo de si queremos la dirección como cliente o como servidor
	if (dir_servidor != NULL)
	{
		// si hemos especificado dir_servidor, es que somos el cliente y vamos a conectarnos con dir_servidor
		if (f_verbose)
			printf("\tNombre/dirección del equipo: %s\n", dir_servidor);
	}
	else
	{
		// si no hemos especificado, es que vamos a ser el servidor
		if (f_verbose)
			printf("\tNombre/dirección del equipo: ninguno (seremos el servidor)\n");
		hints.ai_flags = AI_PASSIVE; // especificar flag para que la IP se rellene con lo necesario para hacer bind
	}
	if (f_verbose)
		printf("\tServicio/puerto: %s\n", servicio);

	// llamada a getaddrinfo() para obtener la estructura de direcciones solicitada
	// getaddrinfo() pide memoria dinámica al SO, la rellena con la estructura de direcciones,
	// y escribe en servinfo la dirección donde se encuentra dicha estructura.
	// La memoria *dinámica* reservada por una función NO se libera al salir de ella.
	// Para liberar esta memoria, usar freeaddrinfo()
	if (f_verbose)
	{
		printf("2 - Solicitando la estructura de direcciones con getaddrinfo()... ");
		fflush(stdout);
	}
	status = getaddrinfo(dir_servidor, servicio, &hints, &servinfo);
	if (status != 0)
	{
		fprintf(stderr, "Error en la llamada getaddrinfo(): %s\n", gai_strerror(status));
		exit(1);
	}
	if (f_verbose)
	{
		printf("hecho\n");
	}

	// imprime la estructura de direcciones devuelta por getaddrinfo()
	if (f_verbose)
	{
		printf("3 - Analizando estructura de direcciones devuelta... \n");
		direccion = servinfo;
		while (direccion != NULL)
		{ // bucle que recorre la lista de direcciones
			printf("    Dirección %d:\n", numdir);
			printsockaddr((struct sockaddr_storage *)direccion->ai_addr);
			// "avanzamos" direccion a la siguiente estructura de direccion
			direccion = direccion->ai_next;
			numdir++;
		}
	}

	// devuelve la estructura de direcciones devuelta por getaddrinfo()
	return servinfo;
}

/**************************************************************************/
/* Imprime una direccion */
/**************************************************************************/
void printsockaddr(struct sockaddr_storage *saddr)
{
	struct sockaddr_in *saddr_ipv4; // puntero a estructura de dirección IPv4
	// el compilador interpretará lo apuntado como estructura de dirección IPv4
	struct sockaddr_in6 *saddr_ipv6; // puntero a estructura de dirección IPv6
	// el compilador interpretará lo apuntado como estructura de dirección IPv6
	void *addr;					  // puntero a dirección. Como puede ser tipo IPv4 o IPv6 no queremos que el compilador la interprete de alguna forma particular, por eso void
	char ipstr[INET6_ADDRSTRLEN]; // string para la dirección en formato texto
	int port;					  // para almacenar el número de puerto al analizar estructura devuelta

	if (saddr == NULL)
	{
		printf("La dirección está vacía\n");
	}
	else
	{
		printf("\tFamilia de direcciones: ");
		fflush(stdout);
		if (saddr->ss_family == AF_INET6)
		{ // IPv6
			printf("IPv6\n");
			// apuntamos a la estructura con saddr_ipv6 (el cast evita el warning),
			// así podemos acceder al resto de campos a través de este puntero sin más casts
			saddr_ipv6 = (struct sockaddr_in6 *)saddr;
			// apuntamos a donde está realmente la dirección dentro de la estructura
			addr = &(saddr_ipv6->sin6_addr);
			// obtenemos el puerto, pasando del formato de red al formato local
			port = ntohs(saddr_ipv6->sin6_port);
		}
		else if (saddr->ss_family == AF_INET)
		{ // IPv4
			printf("IPv4\n");
			saddr_ipv4 = (struct sockaddr_in *)saddr;
			addr = &(saddr_ipv4->sin_addr);
			port = ntohs(saddr_ipv4->sin_port);
		}
		else
		{
			fprintf(stderr, "familia desconocida\n");
			exit(1);
		}
		// convierte la dirección ip a string
		inet_ntop(saddr->ss_family, addr, ipstr, sizeof ipstr);
		printf("\tDirección (interpretada según familia): %s\n", ipstr);
		printf("\tPuerto (formato local): %d\n", port);
	}
}
/**************************************************************************/
/* Configura el socket, devuelve el socket y servinfo */
/**************************************************************************/
int initsocket(struct addrinfo *servinfo, char f_verbose)
{
	int sock;

	printf("\nSe usará ÚNICAMENTE la primera dirección de la estructura\n");

	// crea un extremo de la comunicación y devuelve un descriptor
	if (f_verbose)
	{
		printf("Creando el socket (socket)... ");
		fflush(stdout);
	}
	sock = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	if (sock < 0)
	{
		perror("Error en la llamada socket: No se pudo crear el socket");
		/*muestra por pantalla el valor de la cadena suministrada por el programador, dos puntos y un mensaje de error que detalla la causa del error cometido */
		exit(1);
	}
	if (f_verbose)
		printf("hecho\n");

	return sock;
}
/**************************************************************************/
/* Funcion envio de mensaje */
/**************************************************************************/
int enviarDatos(struct rcftp_msg *mensaje_enviar, int socket, struct sockaddr *remote, socklen_t remotelen)
{
	int sendsize = sendto(socket, mensaje_enviar, sizeof(*mensaje_enviar), 0, remote, remotelen);
	if (sendsize != sizeof(*mensaje_enviar) || sendsize < 0)
	{
		printf("Error en sendto");
		exit(1);
	}
	return sendsize;
}
/**************************************************************************/
/*  Función para recibir datos  */
/**************************************************************************/
int recibirDatos(int socket, struct rcftp_msg *buffer, int length, struct addrinfo *servinfo)
{
	int recvsize = recvfrom(socket, (char *)buffer, length, 0, (struct sockaddr *)servinfo, &servinfo->ai_addrlen);
	if (errno != EAGAIN && recvsize < 0)
	{
		printf("Error en rcvfrom");
		exit(1);
	}
	return recvsize;
}

/**************************************************************************/
/* Funcion que comprueba si la version y el checksum del mensaje es valido. */
/**************************************************************************/
int esMensajeValido(struct rcftp_msg mensaje)
{
	return mensaje.version == RCFTP_VERSION_1 && issumvalid(&mensaje, sizeof(mensaje)) == 1 ? 1 : 0;
}

/**************************************************************************/
/*  Función para ver si la respuesta es la esperada  */
/**************************************************************************/
int respuestaEsperada(struct rcftp_msg mensaje_sent, struct rcftp_msg mensaje_recv)
{
	if (mensaje_sent.flags == F_FIN) {
		return mensaje_recv.flags == F_FIN &&
			   ntohl(mensaje_recv.next) == (ntohl(mensaje_sent.numseq) + ntohs(mensaje_sent.len)) && 
			   mensaje_recv.flags != F_BUSY && mensaje_recv.flags != F_ABORT;
	} else {
		return ntohl(mensaje_recv.next) == (ntohl(mensaje_sent.numseq) + ntohs(mensaje_sent.len)) && 
			   mensaje_recv.flags != F_BUSY && mensaje_recv.flags != F_ABORT;
	}
}

/**************************************************************************/
/* Funcion crear mensaje: Crea un mensaje nuevo RCFTP */
/**************************************************************************/
struct rcftp_msg crearMensajeRCFTP(char* mensaje, size_t length, size_t numseq, int ultimoMensaje)
{
	struct rcftp_msg mensaje_enviar;
	mensaje_enviar.version = RCFTP_VERSION_1;
	mensaje_enviar.flags   = (ultimoMensaje == 1) ? F_FIN : F_NOFLAGS;
	mensaje_enviar.numseq  = htonl((uint32_t)numseq);
	mensaje_enviar.next    = htonl(0);
	mensaje_enviar.len     = htons((uint16_t)length);
	int j;
	for (j = 0; j < length; j++)
	{
		mensaje_enviar.buffer[j] = mensaje[j];
	}
	mensaje_enviar.sum = 0;
	mensaje_enviar.sum = xsum((char *)&mensaje_enviar, sizeof(mensaje_enviar));
	return mensaje_enviar;
}

/**************************************************************************/
/*  algoritmo 1 (basico)  */
/**************************************************************************/
void alg_basico(int socket, struct addrinfo *servinfo)
{
	int ultimoMensaje = 0;
	int ultimoMensajeConfirmado = 0;
	char buffer[RCFTP_BUFLEN]; 			// Buffer para almacenar los datos. 
	ssize_t length;						// longitud de lo que vamos a leer del fichero
	ssize_t numeroSec = 0;				// numero de secuencia
	struct rcftp_msg mensaje;			// Estructura de el envío.
	struct rcftp_msg respuesta;			// Estructura de la respuesta

	length = readtobuffer(buffer, RCFTP_BUFLEN);
	if (length < RCFTP_BUFLEN && length >= 0)
	{
		ultimoMensaje = 1;
	}
	mensaje = crearMensajeRCFTP(buffer, length, numeroSec, ultimoMensaje);
	while (ultimoMensajeConfirmado == 0)
	{
		// Enviando mensaje
		enviarDatos(&mensaje, socket, servinfo->ai_addr, servinfo->ai_addrlen);
		printf("Mensaje enviado\n");
		print_rcftp_msg(&mensaje, sizeof(mensaje));

		// Recibo mensaje
		recibirDatos(socket, &respuesta, sizeof(respuesta), servinfo);
		printf("Mensaje recibido\n");
		print_rcftp_msg(&respuesta, sizeof(respuesta));

		if (esMensajeValido(respuesta) && respuestaEsperada(mensaje, respuesta))
		{
			if (ultimoMensaje == 1)
			{
				ultimoMensajeConfirmado = 1;
			}
			else
			{
				numeroSec = numeroSec + length;
				length = readtobuffer(buffer, RCFTP_BUFLEN);
				if (length < RCFTP_BUFLEN && length >= 0)
				{
					ultimoMensaje = 1;
				}
				mensaje = crearMensajeRCFTP(buffer, length, numeroSec, ultimoMensaje);
			}
		}
	}
}

void alg_stopwait(int socket, struct addrinfo *servinfo)
{
	int sockflags;
	int timeouts_procesados = 0;
	sockflags = fcntl(socket,F_GETFL,0);
	fcntl(socket, F_SETFL, sockflags | O_NONBLOCK);
	signal(SIGALRM,handle_sigalrm);
	printf("Comunicación con algoritmo stop&wait\n");

	int ultimoMensaje=0;
	int ultimoMensajeConfirmado=0;
	
	char buffer[RCFTP_BUFLEN]; //Buffer para almacenar los datos. 
	ssize_t len; //longitud de lo que vamos a leer del fichero
	ssize_t numeroSec = 0; //numero de secuencia
	struct rcftp_msg mensajeEnvio;	//Estructura de el envío.
	struct rcftp_msg mensajeRespuesta; //Estructura de la respuesta

	len=readtobuffer(buffer,RCFTP_BUFLEN); 
	if (len<RCFTP_BUFLEN && len>= 0){ //Cuando tenemos menos de 512 bytes es cuando estamos enviando el ultimo paquete.
		ultimoMensaje = 1;
	}
	
	mensajeEnvio=crearMensajeRCFTP(buffer, len, numeroSec, ultimoMensaje); //Creamos la estructura del mensaje que vamos a enviar.

	while (ultimoMensajeConfirmado==0){
		//Enviando mensaje
		enviarDatos(&mensajeEnvio, socket, servinfo->ai_addr, servinfo->ai_addrlen);
		printf("Mensaje enviado\n");
		print_rcftp_msg(&mensajeEnvio, sizeof(mensajeEnvio));
        addtimeout();
        int esperar = 1;
        int datosRecibidos;
        
        while (esperar) {
            datosRecibidos = recibirDatos(socket, &mensajeRespuesta, sizeof(mensajeRespuesta), servinfo);
            if (datosRecibidos > 0) {
                canceltimeout();
                esperar = 0;
                printf("Mensaje recibido\n");
				print_rcftp_msg(&mensajeRespuesta, sizeof(mensajeRespuesta));
            }
            if (timeouts_procesados != timeouts_vencidos) {
                esperar = 0;
                timeouts_procesados++;
            }
        }

		if(esMensajeValido(mensajeRespuesta) && respuestaEsperada(mensajeEnvio,mensajeRespuesta)){
			if(ultimoMensaje==1){
				ultimoMensajeConfirmado = 1;
			}
			else{
				numeroSec = numeroSec + len; //modifica el numero de secuencia segun lo que ha leido
				len = readtobuffer(buffer,RCFTP_BUFLEN);
				if(len<RCFTP_BUFLEN && len>= 0){ //Fin del fichero
					ultimoMensaje= 1;
				}
				mensajeEnvio=crearMensajeRCFTP(buffer, len, numeroSec, ultimoMensaje); 
			}
		}
	}
}

/**************************************************************************/
/*  Función para ver si la respuesta es la esperada  */
/**************************************************************************/
int esLaRespuestaEsperadaGBN(struct rcftp_msg mensaje_sent, struct rcftp_msg mensaje_recv, int next_min_win, int next_max_win)
{
	if (mensaje_sent.flags == F_FIN) {
		return mensaje_recv.flags == F_FIN &&
			   ntohl(mensaje_recv.next) == next_max_win && 
			   mensaje_recv.flags != F_BUSY && mensaje_recv.flags != F_ABORT;
	} else {
		return ntohl(mensaje_recv.next) > next_min_win && ntohl(mensaje_recv.next) <= next_max_win && 
			   mensaje_recv.flags != F_BUSY && mensaje_recv.flags != F_ABORT;
	}
}

void alg_ventana(int socket, struct addrinfo *servinfo,int window) {
	setwindowsize(window);
	int sockflags;
    sockflags = fcntl(socket, F_GETFL, 0);
    fcntl(socket, F_SETFL, sockflags| O_NONBLOCK); 
    signal(SIGALRM, handle_sigalrm);
    printf("Comunicación con algoritmo go-back-n\n");
    int numDatosRecibidos = 0;
    int ultimoMensaje = 0;
    int ultimoMensajeConfirmado = 0;
    char buffer[RCFTP_BUFLEN];
    ssize_t len = 0;
    ssize_t numeroSec = 0;
    struct rcftp_msg mensaje;
    struct rcftp_msg respuesta;
    int timeouts_procesados = 0;
	int next_min_win = 0;
	int next_max_win = 0;
	int aux = 0;
    while (!ultimoMensajeConfirmado){
		
        if ((getfreespace() >= RCFTP_BUFLEN) && !ultimoMensaje) {
			numeroSec += len;
            len = readtobuffer(buffer, RCFTP_BUFLEN);
            if ((len < RCFTP_BUFLEN) && (len >= 0)) {
                ultimoMensaje = 1;
            }
            mensaje = crearMensajeRCFTP(buffer, len, numeroSec, ultimoMensaje); 
			enviarDatos(&mensaje, socket, servinfo->ai_addr, servinfo->ai_addrlen);
            printf("Mensaje enviado\n");
			print_rcftp_msg(&mensaje, sizeof(mensaje));
            addtimeout();
            addsentdatatowindow(buffer, len);
			next_max_win += len;
        }
        numDatosRecibidos = recibirDatos(socket, &respuesta, sizeof(respuesta), servinfo);
        if (numDatosRecibidos > 0) {
			printf("Mensaje recibido\n");
			print_rcftp_msg(&respuesta, sizeof(respuesta));
            if (esMensajeValido(respuesta) && esLaRespuestaEsperadaGBN(mensaje,respuesta,next_min_win,next_max_win)) {
				canceltimeout();
                freewindow(ntohl(respuesta.next));
				next_min_win = ntohl(respuesta.next);
                if (ultimoMensaje) {
                    ultimoMensajeConfirmado = 1;
                }
            }
        }
        if (timeouts_procesados != timeouts_vencidos) {
			int a = RCFTP_BUFLEN;
			aux = getdatatoresend(buffer, &a);
            mensaje = crearMensajeRCFTP(buffer, a, aux, ultimoMensaje); 
            enviarDatos(&mensaje, socket, servinfo->ai_addr, servinfo->ai_addrlen);
            addtimeout();
            timeouts_procesados++;
        }
    }
}

