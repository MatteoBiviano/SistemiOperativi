/*
 * @file server.h
 * @brief Il file contiene strutture/funzioni usate dal server
 * @author Matteo Biviano 543933
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore
 */
#if !defined(SERVER_H_)
#define SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/un.h>
#include <sys/select.h>
#include "message.h"
#include "config.h"
#include "stats.h"
#include "ops.h"
#include "icl_hash.h"
#include "history.h"


/*
 * @struct  register_user
 * @brief   struttura dati che rappresenta un utente registrato
 * @var descriptor  rappresenta il descrittore associato al client
 * @var history rappresenta la cronologia dei messaggi dell'utente
 */
typedef struct register_user_struct{
  long descriptor;
  list* history;
} register_user;


/*
 * @struct            conn_user
 * @brief             struttura dati che rappresenta un client connesso
 * @var nickname	    nickname dell'utente connesso
 * @var descriptor		descrittore che rappresenta il socket associato all'utente
 */
typedef struct conn_user_struct{
	char nickname[MAX_NAME_LENGTH+1];
  long descriptor;
} conn_user;

#endif /* SERVER_H_ */
