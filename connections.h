/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */

/*
 * @file  connection.h
 * @brief Contiene le funzioni che implementano il protocollo
 *        tra i clients ed il server
 */
#ifndef CONNECTIONS_H_
#define CONNECTIONS_H_

#define MAX_RETRIES     10  // numero massimo di tentativi di connessione
#define MAX_SLEEPING     3  // intervallo di tempo tra un tentativo e l'altro
#if !defined(UNIX_PATH_MAX)
#define UNIX_PATH_MAX  64   // dimensione massima del path
#endif

#include "message.h"

/*
 * @function openConnection
 * @brief Apre una connessione AF_UNIX verso il server
 *
 * @param path Path del socket AF_UNIX
 * @param ntimes numero massimo di tentativi di retry
 * @param secs tempo di attesa tra due retry consecutive
 *
 * @return il descrittore associato alla connessione in caso di successo
 *         -1 in caso di errore
 */
int openConnection(char* path, unsigned int ntimes, unsigned int secs);

// -------- server side -----
/*
 * @function readHeader
 * @brief Legge l'header del messaggio
 *
 * @param fd     descrittore della connessione
 * @param hdr    puntatore all'header del messaggio da ricevere
 *
 * @return <=0 se c'e' stato un errore
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa)
 */
int readHeader(long connfd, message_hdr_t *hdr);
/*
 * @function readData
 * @brief Legge il body del messaggio
 *
 * @param fd     descrittore della connessione
 * @param data   puntatore al body del messaggio
 *
 * @return <=0 se c'e' stato un errore
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa)
 */
int readData(long fd, message_data_t *data);

/*
 * @function readMsg
 * @brief Legge l'intero messaggio
 *
 * @param fd     descrittore della connessione
 * @param msg   puntatore al messaggio
 *
 * @return <=0 se c'e' stato un errore
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa)
 */
int readMsg(long fd, message_t *msg);

/* da completare da parte dello studente con altri metodi di interfaccia */

// ------- client side ------
/*
 * @function sendRequest
 * @brief Invia un messaggio di richiesta al server
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return <=0 se c'e' stato un errore
 */
int sendRequest(long fd, message_t *msg);

/*
 * @function sendData
 * @brief Invia il body del messaggio al server
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return <=0 se c'e' stato un errore
 *        >0 se l'operazione ha avuto successo
 */
int sendData(long fd, message_data_t *msg);

/*
 * @function sendHeader Invia
 * @brief Invia un header di messaggio ad un descrittore
 *
 * @param fd    Il descrittore a cui inviare l'header
 * @param hdr   L'header da inviare
 *
 * @return <=0 se c'e' stato un errore
 *            >0 se l'operazione ha avuto successo
 */
int sendHeader(long fd, message_hdr_t *hdr);

/* da completare da parte dello studente con eventuali altri metodi di interfaccia */

/*
 * @function replymsg
 * @brief Legge un messaggio di risposta dal server
 * @param fd  descrittore della connessione
 * @param msg puntatore al messaggio da ricevere
 * @return 0 in caso di successo
 *         -1 in caso di errore
*/
int replymsg(long fd, message_t *msg);

#endif /* CONNECTIONS_H_ */
