/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
 /*
  * @file  message.h
  * @brief Contiene il formato del messaggio
  * Si dichiara il contenuto del file "message.h" è opera originale
  * dello studente Matteo Biviano (matr. 543933)
  */

#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <assert.h>
#include <string.h>
#include "config.h"
#include "ops.h"
#include <stdlib.h>
#include <stdio.h>


/**
 *  @struct header
 *  @brief header del messaggio
 *
 *  @var op tipo di operazione richiesta al server
 *  @var sender nickname del mittente
 */
typedef struct {
    op_t     op;
    char sender[MAX_NAME_LENGTH+1];
} message_hdr_t;

/**
 *  @struct header
 *  @brief header della parte dati
 *
 *  @var receiver nickname del ricevente
 *  @var len lunghezza del buffer dati
 */
typedef struct {
    char receiver[MAX_NAME_LENGTH+1];
    unsigned int   len;
} message_data_hdr_t;

/**
 *  @struct data
 *  @brief body del messaggio
 *
 *  @var hdr header della parte dati
 *  @var buf buffer dati
 */
typedef struct {
    message_data_hdr_t  hdr;
    char               *buf;
} message_data_t;

/**
 *  @struct messaggio
 *  @brief tipo del messaggio
 *
 *  @var hdr header
 *  @var data dati
 */
typedef struct {
    message_hdr_t  hdr;
    message_data_t data;
} message_t;

/* ------ funzioni di utilità ------- */

/**
 * @function setheader
 * @brief scrive l'header del messaggio
 *
 * @param hdr puntatore all'header
 * @param op tipo di operazione da eseguire
 * @param sender mittente del messaggio
 */
static inline void setHeader(message_hdr_t *hdr, op_t op, char *sender) {
#if defined(MAKE_VALGRIND_HAPPY)
    memset((char*)hdr, 0, sizeof(message_hdr_t));
#endif
    hdr->op  = op;
    strncpy(hdr->sender, sender, strlen(sender)+1);
}
/**
 * @function setData
 * @brief scrive la parte dati del messaggio
 *
 * @param msg puntatore al body del messaggio
 * @param rcv nickname o groupname del destinatario
 * @param buf puntatore al buffer
 * @param len lunghezza del buffer
 */
static inline void setData(message_data_t *data, char *rcv, const char *buf, unsigned int len) {
#if defined(MAKE_VALGRIND_HAPPY)
    memset((char*)&(data->hdr), 0, sizeof(message_data_hdr_t));
#endif

    strncpy(data->hdr.receiver, rcv, strlen(rcv)+1);
    data->hdr.len  = len;
    data->buf      = (char *)buf;
}

/*
 * @function errorMsg
 * @brief         La funzione inizializza un messaggio d'errore
 * @param msg     messaggio, creato esternamente, da inizializzare
 * @param op      operazione di errore
 * @param txt     eventuale testo del messaggio d'errore
 * @param dest     testo del messaggio di errore (parametro opzionale)
 */
static inline void errorMsg(message_t *msg, op_t op, const char* txt, const char *dest) {
  //Ripuliamo il messaggio
  memset(msg, 0, sizeof(message_t));
  //Assegniamo l'operazione, cioè il tipo di errore del messaggio
  msg->hdr.op = op;
  //Assegniamo il testo, se è inserito
  if(txt != NULL) {
    //Copio la lunghezza del testo
    size_t len = strlen(txt);
    /**** Setto i campi del messaggio ****/
    //Copio il testo nel messaggio
    msg->data.buf = calloc(len + 1, sizeof(char));
    strncpy(msg->data.buf, dest, len);
    //Copio la lunghezza dell'header dati
    msg->data.hdr.len = len;
  }
  //Assegno il Nickname del destinatario, se inserito
  if(dest!=NULL){
    //Copio gli "strlen(dest)" bit da "dest" a "msg->data.hdr.receiver"
    memcpy(&(msg->data.hdr.receiver), dest, strlen(dest));
  }
}


#endif /* MESSAGE_H_ */
