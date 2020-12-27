/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */

/*
 * @file utility.h
 * @brief File contenente funzioni per leggere e scrivere bytes e define di utilità
 * NOTA: il file contiene funzioni estrapolate (e modificate) dal libro "Unix Network Programming, Volume 1: The Sockets Networking API, 3rd Edition"
 */
#if !defined(UTILITY_H_)
#define UTILITY_H_

#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

//Gestione degli errori: Parametro diverso da 0
#define handle_error_0(X, msg) \
  if(X!=0){                   \
    perror(#msg);                \
    exit(EXIT_FAILURE);         \
  }

//Gestione degli errori: Parametro minore di 0
#define handle_error_min(X,msg)\
  if(X<0){                   \
    perror(#msg);                \
    exit(EXIT_FAILURE);         \
  }
//Gestione degli errori: Parametro uguale a -1
#define handle_error_1(X, msg) \
  if(X==-1){                   \
    perror(#msg);                \
    exit(EXIT_FAILURE);         \
  }
//Gestione degli errori: Parametro uguale a NULL
#define handle_error_NULL(X, msg) \
  if(X==NULL){             \
    perror(#msg);           \
    exit(EXIT_FAILURE);     \
  }

/*
 * @function readn
 * @brief Legge n bytes da un descrittore
 *        Si comporta come una read, ma riavvia la read in caso di lettura incompleta
 *        Se ci sono meno bytes di quelli richiesti, avviene una lettura parziale
 * @param fd   descrittore da cui leggere
 * @param buf buffer in cui memorizzare il dato letto
 * @param size    numero di bytes da leggere
 * @return ssize_t > 0   numero di bytes letti\n
 *         ssize_t = 0   in caso di EOF\n
 *         ssize_t < 0   in caso di errore
 */
int readn(long fd, void *buf, size_t size);

/*
 * @function writen
 * @brief Scrive n bytes in un descrittore
 *        Si comporta come una write, ma riavvia la write in caso di interruzioni
 *        Se non basta lo spazio nel buffer può avvenire una scrittura parziale
 * @param fd    descrittore in cui scrivere
 * @param buf  buffer in uscita
 * @param size     numero di bytes da scrivere
 * @return ssize_t > 0 numero di bytes scritti\n
 *         ssize_t < 0 in caso di errore
 */
int writen(long fd, void *buf, size_t size);




#endif /* UTILITY_H_ */
