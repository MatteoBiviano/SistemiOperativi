/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */

/*
 * @file utility.c
 * @brief File contenente funzioni per leggere e scrivere bytes
 * NOTA: il file è stato preso dal libro "Unix Network Programming, Volume 1: The Sockets Networking API, 3rd Edition"
 */
#if !defined(UTILITY_C_)
#define UTILITY_C_

#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

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
int readn(long fd, void *buf, size_t size) {
  size_t left = size;
  int r;
  char *bufptr = (char*)buf;
  while(left>0) {
	   if ((r=read((int)fd ,bufptr,left)) == -1) {
	      if (errno == EINTR) continue; //Chiamo di nuovo read
	       return -1;
	   }
     if (r == 0) return 0;   // gestione chiusura socket (EOF)
     left    -= r;
	   bufptr  += r;
  }
  return size;
}

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
int writen(long fd, void *buf, size_t size) {
  size_t left = size;
  int r;
  char *bufptr = (char*)buf;
  while(left>0) {
	   if ((r=write((int)fd ,bufptr,left)) == -1) {
	      if (errno == EINTR) continue;
	      return -1;
	   }
	   if (r == 0) return 0;
     left    -= r;
	   bufptr  += r;
  }
  return 1;
}


#endif /* UTILITY_C_ */
