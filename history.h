/*
 * chatterbox Progetto del corso di LSO 2017
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/*
 * @file history.h
 * @brief Dichiarazioni strutture e funzioni per operare su un buffer circolare
 *        concorrente, contenente messaggi e rappresentante la history di un client
 * @author Matteo Biviano 543933
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell'autore
 */

#if !defined(HISTORY_H_)
#define HISTORY_H_

#include <string.h>
#include <pthread.h>
#include "message.h"

/*
 *  @struct elem
 *  @brief elemento della buffer
 *  @var data informazione dell'elemento (cioè un messaggio)
 *  @var next puntatore all'elemento successivo
 */
typedef struct struct_elem {
  message_t data;
  struct struct_elem *next;
} elem;

/*
 *  @struct buff
 *  @brief buffer circolare concorrente
 *  @var testa puntatore alla testa del buffer
 *  @var coda puntatore alla coda del buffer
 *  @var mtx_buff variabile di mutuaesclusione per gestire la concorrenza
 *  @var num numero di elementi attualmente contenuti nel buffer
 *  @var capacity capienza del buffer
 */
typedef struct struct_list{
  size_t num;
  size_t capacity;
  pthread_mutex_t mtx_buff;
  elem *testa;
  elem *coda;
} buff;

/*
 *  @function initBuff
 *  @brief crea un buffer circolare concorrente di capacità massima size
 *  @param size capacità massima del buffer
 *  @return puntatore al buffer creato
 */
buff *initBuff(int size);

/*
 *  @function push
 *  @brief La funzione inserisce un elemento
 *  @param l buffer in cui inserire l'elemento
 *  @param msg  il messaggio da da inserire
 */
void push(buff *l, message_t msg);

/*
 *  @function pop
 *  @brief La procedura estra il primo elemento dal buffer
 *  @param l  buffer da cui estrarre l'elemento
 *  @return puntatore all'elemento tolto
 *          NULL se il buffer è vuoto
 */
message_t pop(buff *l);

/*
 *  @function deleteBuffer
 *  @brief libera il buffer dalla memoria
 *  @param l puntatore al buffer da liberare
 *  @return <0 in caso di errore (errno settato)
 */
int deleteBuffer(buff *l);

/*
 * @function buffToArray
 * @brief Crea un'array contenente gli elementi della history
 * @param l history da cui creare l'array
 * @return NULL in caso di errore
 */
elem** buffToArray(buff *l);

#endif /* HISTORY_H_ */
