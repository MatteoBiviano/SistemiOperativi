/*
 * chatterbox Progetto del corso di LSO 2017
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/*
 * @file history.c
 * @brief Implementazione funzioni per operare su una buffer circolare
 *        concorrente, contenente messaggi e rappresentante la history
 * @author Matteo Biviano 543933
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore
 */

#if !defined(HISTORY_C_)
#define HISTORY_C_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#include "config.h"
#include "message.h"
#include "history.h"
#include "connections.h"
#include "utility.h"


/*
 *  @function initBuff
 *  @brief crea un buffer circolare concorrente di capacità massima size
 *  @param size capacità massima del buffer
 *  @return puntatore al buffer creato
 */
buff *initBuff(int size){
  //Alloco il nuovo buffer
  buff *new = malloc (sizeof(buff));
  //Inizializzo il nuovo buffer
  new->testa=NULL;
  new->coda=NULL;
  //Inizializzo la variabile di mutuaesclusione
  int mutex_val = pthread_mutex_init(&(new->mtx_buff), NULL);
  //Controllo che tutto sia andato bene
  if(mutex_val != 0) {
    //Setto errno
    errno = mutex_val;
    //Libero il puntatore
    free(new);
    return NULL;
  }
  new->num=0;
  //Controlliamo il parametro d'ingresso
  if(size<1) size=1;
  new->capacity=size;
  return new;
}
/*
 * @function initElem
 * @brief crea un nuovo elemento per il buffer
 * @param msg è il messaggio che deve essere inserito nell'elemento
 * @return l'elemento creato
 */
elem* initElem(message_t msg){
  //Alloco il nuovo elemento da inserire
  elem* new = malloc(sizeof(elem));
  memset(new, 0, sizeof(elem));
  /**** Creo ed inizializzo il messaggio  ****/
  char* buffer=malloc((msg.data.hdr.len)*sizeof(char));
  message_t msg1;
  //Copio il messaggio
  strncpy(buffer, msg.data.buf, msg.data.hdr.len);
  //Uso le funzioni implementate in "message.h"
  setHeader(&msg1.hdr, msg.hdr.op, msg.hdr.sender);
  setData(&msg1.data, msg.data.hdr.receiver, buffer, msg.data.hdr.len);
  //Inizializzo il nuovo elemento
  new->data=msg1;
  new->next=NULL;
  return new;
}

/*
 *  @function push
 *  @brief La funzione inserisce un elemento
 *  @param l buffer in cui inserire l'elemento
 *  @param msg  il messaggio da da inserire
 */
void push(buff *l, message_t msg){
  //Creo il nuovo nodo
  elem* new =initElem(msg);
  //Inserisco il nuovo elemento nel buffer, in m.e.
  pthread_mutex_lock(&(l->buff));
  /*** Controllo se ho spazio o meno nel buffer ***/
  if(l->capacity==l->num){
    //Non ho spazio, quindi elimino il messaggio più vecchio

    //Nuova testa del buffer (il secondo elemento del buffer)
    elem* newHead=l->testa->next;
    //Devo eliminare il messaggio più vecchio (il primo)
    free((l->testa->data).data.buf);
    free(l->testa);
    //Salvo la nuova testa
    l->testa=newHead;
    l->coda->next=new;
    l->coda=l->coda->next;
  }
  //Se ho ancora spazio
  if(l->num < l->capacity){
    //Se la testa è vuota
    if(l->testa==NULL){
      //Inserisco in testa
      l->testa=new;
      l->coda=l->testa;
    }
    else{ //la testa non è vuota
      //Inserisco all'interno del buffer
      l->coda->next=new;
      l->coda=l->coda->next;
    }
    //Incremento il numero di elementi inseriti
    l->num++;
  }
  pthread_mutex_unlock(&(l->mtx_buff));
}
/*
 *  @function pop
 *  @brief La procedura estra il primo elemento dal buffer
 *  @param l  buffer da cui estrarre l'elemento
 *  @return puntatore all'elemento tolto
 *          NULL se il buffer è vuoto
 */
message_t pop(buff *l){
  //Devo accedere in mutuaesclusione al buffer
  pthread_mutex_lock(&(l->mtx_buff));
  //Salvo il messaggio più vecchio del buffer(il primo presente)
  message_t ret =l->testa->data;
  //Dichiaro una variabile che userò per salvarmi la nuova testa del buffer
  elem* newHead=NULL;
  //Salvo la nuova testa (il secondo elemento del buffer)
  newHead=l->testa->next;
  //Libero il primo elemento
  free(l->testa);
  //Faccio diventare il secondo elemento la nuova testa
  l->testa=newHead;
  //Decremento il numero di elementi nel buffer
  l->num--;
  pthread_mutex_unlock(&(l->mtx_buff));
  return ret;
}
/*
 *  @function deleteBuffer
 *  @brief libera il buffer dalla memoria
 *  @param l puntatore al buffer da liberare
 *  @return <0 in caso di errore (errno settato)
 */
int deleteBuffer(buff *l){
  //Controllo i parametri
  if(l == NULL) {
    errno = EINVAL;
    return -1;
  }
  int n_elem=l->num;
  //Libero il buffer in modo ricorsivo
  while(l->testa!=NULL){
    //Passo ricorsivo
    message_t corr=pop(l);
    //Termino: libero il Messaggio
    free(corr.data.buf);
    //Decremento il numero di elementi
    n_elem--;
  }
  //Distruggo e libero la variabile di mutuaesclusione
  int mutex_val = pthread_mutex_destroy(&(l->mtx_buff));
  if(mutex_val !=0){
    //Setto errno
    errno=mutex_val;
    free(l);
    return -1;
  }
  if(l!=NULL){
    free(l);
  }
  //Restituisco 1 se ho liberato tutto il buffer
  return n_elem==0;
}

/*
 * @function buffToArray
 * @brief Crea un'array contenente gli elementi della history
 * @param l history da cui creare l'array
 * @return NULL in caso di errore
 */
elem** buffToArray(buff *l){
  //Mi salvo la dimensione dell'array
  int dim=l->num;
  //Indice di scansione
  int k=0;
  //Dichiaro l'array di elementi
  elem** ret=NULL;
  //Dichiaro un puntatore usato per la scansione
  elem* corr=NULL;
  /*** Accedo in mutuaesclusione al buffer ***/
  pthread_mutex_lock(&(l->mtx_buff));
  //Controllo se ho elementi nel buffer (alternativamente dim!=0)
  if(l->testa!=NULL){
    //Alloco l'array
    ret=(elem**) malloc(dim*sizeof(elem));
    //Scansiono il buffer per copiarne gli elementi
    corr=l->testa;
    while(corr!=NULL){
      ret[k]=corr;
      k++;
      corr=corr->next;
    }
  }
  pthread_mutex_unlock(&(l->mtx_buff));
  return ret;
}
#endif /* HISTORY_C_ */
