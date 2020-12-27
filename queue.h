/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */

/*
 * @file queue.h
 * @brief File contenente la struttura e la dichiarazione delle funzioni per gestire una Coda
 * NOTA: il file è stato preso e modificato dalle soluzioni proposte per l'esercitazione di laboratorio n.12
 */

#ifndef QUEUE_H_
#define QUEUE_H_

#include <pthread.h> //per poter gestire la mutuesclusione
/*
 *  @struct node_t
 *  @brief struttura che serve per rappresentare un elemento della coda
 *  @var val   dati contenuti nel nodo
 *  @var next  puntatore al nodo successivo
 */
typedef struct node {
    void *data;
    struct node *next;
} node_t;

/*
 *  @struct queue_t
 *  @brief struttura che serve per rappresentare una coda
 *  @var head   puntatore alla testa della coda
 *  @var tail   puntatore al nodo successivo
 *  @var q_len  lunghezza della coda
 *  @var qcond  condition della coda
 *  @var qlock   mutex della coda
 */
typedef struct queue{
    node_t        *head;
    node_t        *tail;
    unsigned long  q_len;
    pthread_cond_t* qcond;
    pthread_mutex_t* qlock;
} queue_t;

/*
 * @function initQueue
 * @brief alloca ed inizializza una coda -> chiamata da un solo thread
 * @return  puntatore alla coda allocata
 *          NULL se si sono verificati problemi nell'allucazione (setta errno)
 */
queue_t *initQueue();

/*
 * @function deleteQueue
 * @brief cancella una coda allocata con initQueue -> chiamata da un solo thread
 * @param q  puntatore alla coda da cancellare
 */
void deleteQueue(queue_t *q);

/*
 * @function push
 * @brief inserisce un dato nella coda
 * @param q puntatore alla coda in cui inserire il dato
 * @param data puntatore al dato da inserire
 * @return  0 in caso di successivo
 *         -1 in caso di errore (setta errno)
 */
int push(queue_t *q, void *data);

/*
 * @function pop
 * @brief estrae un dato nella coda
 * @param q puntatore alla coda da cui estrarre il dato
 * @return  puntatore al dato estratto
 */
void  *pop(queue_t *q);

/*
 * @function length
 * @brief ritorna la lunghezza della coda. L'accesso non è in mutua esclusione
 * @param q puntatore alla coda di cui ritornare la lunghezza
 * @return  lunghezza della coda
 */
unsigned long length(queue_t *q);

/*
 * @function toEmpty
 * @brief La funzione svuota la coda (eliminandone tutti gli elementi)
 * @param q La coda da svuotare
 * @return int 0 in caso di successo, altrimenti -1 ed errno settato.
 */
int toEmpty(queue_t *q);

#endif /* QUEUE_H_ */
