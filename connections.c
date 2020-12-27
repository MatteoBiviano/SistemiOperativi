/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */

/*
 * @file connections.c
 * @brief File contenente la struttura e la funzione per la configurazione del server
 *
 * Si dichiara il contenuto del file "connections.c" è opera originale
 * dello studente Matteo Biviano (matr. 543933)
 */

#if !defined(CONNECTIONS_C_)
#define CONNECTIONS_C_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "connections.h"
#include "utility.h"
#include "message.h"

/**
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
int openConnection(char* path, unsigned int ntimes, unsigned int secs){
  //Controllo dei parametri
  if(path==NULL||strlen(path)>UNIX_PATH_MAX){
    // setto errno:
    // -EINVAL:
    errno=EINVAL;
    return -1;
  }
  //Creazione del socket per la comunicazione
  int fd_skt = socket(AF_UNIX, SOCK_STREAM, 0);
  handle_error_1(fd_skt, "Errore nella creazione del socket\n");

  //Creazione struct sigaction
  struct sockaddr_un sa;
  memset(&sa, '0', sizeof(sa));
  strncpy(sa.sun_path, path ,UNIX_PATH_MAX);
  sa.sun_family=AF_UNIX;

  //Controllo che il numero di tentativi di retry non superi il numero massimo stabilito
  if(ntimes>MAX_RETRIES) ntimes=MAX_RETRIES;
  //Controllo che il tempo di intervallo tra due tentativi non superi il tempo massimo
  if(secs > MAX_SLEEPING) secs=MAX_SLEEPING;
  int tmp=1;
  //Proviamo a collegarci fino a ntimes volte, aspettando secs tra ogni tentativo
  while(ntimes>0 && (tmp=connect(fd_skt, (struct sockaddr *)&sa, sizeof(sa)))==-1){
    if(errno==ENOENT){
      //Il server è occupato
      ntimes--;
      sleep(secs);
    }
    else{
      fprintf(stderr, "errore in connection\n");
      return -1;
    }
  }
  return tmp==0 ? fd_skt : -1;
}

// -------- server side -----
/**
 * @function readHeader
 * @brief Legge l'header del messaggio
 *
 * @param fd     descrittore della connessione
 * @param hdr    puntatore all'header del messaggio da ricevere
 *
 * @return <=0 se c'e' stato un errore
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa)
 */
int readHeader(long connfd, message_hdr_t *hdr){
  if(connfd<0 || hdr==NULL){
    errno=EINVAL;
    return -1;
  }
  int lung=readn(connfd, hdr, sizeof(message_hdr_t));
  return lung;
}
/**
 * @function readData
 * @brief Legge il body del messaggio
 *
 * @param fd     descrittore della connessione
 * @param data   puntatore al body del messaggio
 *
 * @return <=0 se c'e' stato un errore
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa)
 */
int readData(long fd, message_data_t *data){

  if(fd<0 || data==NULL){
		errno=EINVAL;
    return -1;
  }
  int lung;
  if((lung=readn(fd, &(data->hdr), sizeof(message_data_hdr_t))) <=0) return lung;

  if(data->hdr.len <=0){
    data->buf=NULL;
    return -1;
  }
  else{
    data->buf=(char*)malloc(data->hdr.len * sizeof(char));
    if(!data->buf){
      perror("Errore nella malloc di data->buf");
      return -1;
    }
    memset(data->buf, 0, data->hdr.len * sizeof(char));
    if( (lung = readn(fd, data -> buf, (data -> hdr.len) * sizeof(char))) <= 0){
      free(data -> buf);
      return lung;
    }
  }
  return lung;
}

/**
 * @function readMsg
 * @brief Legge l'intero messaggio
 *
 * @param fd     descrittore della connessione
 * @param msg   puntatore al messaggio
 *
 * @return <=0 se c'e' stato un errore
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa)
 */
int readMsg(long fd, message_t *msg){
  int lung;
  if((lung=readHeader(fd, &(msg->hdr)))<=0) return lung;
  lung=readData(fd, &(msg->data));
  return lung;
}

/* da completare da parte dello studente con altri metodi di interfaccia */

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
int sendHeader(long fd, message_hdr_t *hdr){
  int lung =writen(fd, hdr, sizeof(message_hdr_t));
  return lung;
}

// ------- client side ------
/**
 * @function sendRequest
 * @brief Invia un messaggio di richiesta al server
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return <=0 se c'e' stato un errore
 */
int sendRequest(long fd, message_t *msg){
  int lung;
  if((lung=readHeader(fd, &(msg->hdr)))<=0) return lung;
  lung=sendData(fd, &(msg->data));
  return lung;
}

/**
 * @function sendData
 * @brief Invia il body del messaggio al server
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return <=0 se c'e' stato un errore
 */
int sendData(long fd, message_data_t *msg){
  int lung;
  if((lung = writen(fd, &(msg -> hdr), sizeof(message_data_hdr_t))) <= 0) return lung;
  lung = writen(fd, msg -> buf, msg->hdr.len);
  return lung;
}

/* da completare da parte dello studente con eventuali altri metodi di interfaccia */
/*
 * @function replymsg
 * @brief Legge un messaggio di risposta dal server
 * @param fd  descrittore della connessione
 * @param msg puntatore al messaggio da ricevere
 * @return 0 in caso di successo
 *         -1 in caso di errore
*/
int replymsg(long fd, message_t *msg){
  if((readHeader(fd, &(msg->hdr)) !=0)|| (readData(fd, &(msg->data)) !=0)) return -1;
  return 0;
}
#endif /* CONNECTIONS_C_ */
