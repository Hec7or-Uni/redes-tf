/****************************************************************************/
/* Plantilla para cabeceras de funciones del cliente (rcftpclient)          */
/* Plantilla $Revision$ */
/* Autor: Toral Pallás, Héctor */
/* Autor: Pizarro Martinez, Francisco Javier */
/****************************************************************************/

/**
 * Obtiene la estructura de direcciones del servidor
 *
 * @param[in] dir_servidor String con la dirección de destino
 * @param[in] servicio String con el servicio/número de puerto
 * @param[in] f_verbose Flag para imprimir información adicional
 * @return Dirección de estructura con la dirección del servidor
 */
struct addrinfo* obtener_struct_direccion(char *dir_servidor, char *servicio, char f_verbose);

/**
 * Imprime una estructura sockaddr_in o sockaddr_in6 almacenada en sockaddr_storage
 *
 * @param[in] saddr Estructura de dirección
 */
void printsockaddr(struct sockaddr_storage * saddr);

/**
 * Configura el socket
 *
 * @param[in] servinfo Estructura con la dirección del servidor
 * @param[in] f_verbose Flag para imprimir información adicional
 * @return Descriptor del socket creado
 */
int initsocket(struct addrinfo *servinfo, char f_verbose);

/**
 * Envia un mensaje
 *
 * @param[in] mensaje Estructura del mensaje a enviar
 * @param[in] socket Descriptor de socket
 * @param[in] remote Descriptor de una dirección generica
 * @param[in] remotelen Tamaño en bytes de la dirección
 * @return bytes enviados
 */
ssize_t enviarDatos(struct rcftp_msg *mensaje, int socket, struct sockaddr *remote, socklen_t remotelen); 
	
/**
 * Recibe un mensaje
 *
 * @param[in] socket Descriptor de socket
 * @param[in] respuesta Estructura del mensaje a recibir
 * @param[in] length Tamaño en bytes de la estructura del mensaje a recibir
 * @param[in] servinfo Estructura con la dirección del servidor
 * @return bytes recibidos
 */
ssize_t recibirDatos(int socket, struct rcftp_msg *respuesta, int length, struct addrinfo *servinfo);

/**
 * Funcion crear mensaje: Crea un mensaje nuevo RCFTP
 *
 * @param[in] mensaje contenido del mensaje RCFTP a crear
 * @param[in] length longitud del mensaje a escribir
 * @param[in] numseq numero de secuencia actual
 * @param[in] ultimoMensaje booleano que indica si es el mensaje final
 * @return struct rcftp_msg
 */
struct rcftp_msg crearMensajeRCFTP(char* mensaje, size_t length, size_t numseq, int ultimoMensaje);

/**
 * Funcion crear mensaje: Crea un mensaje nuevo RCFTP
 *
 * @param[in] mensaje mensaje con el formato RCFTP
 * @return 1: valido, 0: invalido
 */
int esMensajeValido(struct rcftp_msg mensaje);

/**
 * Comprobacion de respuesta esperada correctamente.
 *
 * @param[in] mensaje_sent mensaje enviado
 * @param[in] mensaje_recv mensaje recibido
 * @return 1: esperada, 0: no esperada
 */
int respuestaEsperada(struct rcftp_msg mensaje_sent, struct rcftp_msg mensaje_recv);

/**
 * Algoritmo 1 del cliente
 *
 * @param[in] socket Descriptor del socket
 * @param[in] servinfo Estructura con la dirección del servidor
 */
void alg_basico(int socket, struct addrinfo *servinfo);

/**************************************************************************/
/*  algoritmo 2 (stop & wait)  */
/**************************************************************************/
void alg_stopwait(int socket, struct addrinfo *servinfo);

/**************************************************************************/
/*  algoritmo 3 (ventana deslizante)  */
/**************************************************************************/
void alg_ventana(int socket, struct addrinfo *servinfo, int window);