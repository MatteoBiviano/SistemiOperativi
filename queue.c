/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */

/*
 * @file queue.c
 * @brief File contenente l'implementazione delle funzioni per gestire una Coda
 * NOTA: il file è stato preso e modificato dalle soluzioni proposte per l'esercitazione di laboratorio n.12
 */
#if !defined(QUEUE_C_)
#define QUEUE_C_

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "queue.h"
#include "utility.h"

/****** FUNZIONI PER GESTRIRE LA MUTUAESCLUSIONE DELLA CODA *******/
static void LockQueue(queue_t* q)  {
	pthread_mutex_lock(q->qlock);
}
static void UnlockQueue(queue_t* q) {
	pthread_mutex_unlock(q->qlock);
}
static void UnlockQueueAndWait(queue_t* q) {
	pthread_cond_wait(q->qcond, q->qlock);
}
static void UnlockQueueAndSignal(queue_t* q) {
    pthread_cond_signal(q->qcond);
    pthread_mutex_unlock(q->qlock);
}

/****** FUNZIONI PER GESTIRE LA CODA ******/
static node_t *allocNode() {
	return malloc(sizeof(node_t));
}
static queue_t *allocQueue() {
	return malloc(sizeof(queue_t));
}
static void freeNode(node_t *node) {
	 free((void*)node);
}

/*
 * @function initQueue
 * @brief alloca ed inizializza una coda -> chiamata da un solo thread
 * @return  puntatore alla coda allocata
 *          NULL se si sono verificati problemi nell'allucazione (setta errno)
 */
queue_t *initQueue(){
	queue_t *q = allocQueue();
	if(!q) return NULL;
	q -> head = allocNode();
	if(!q -> head) return NULL;
	q -> head -> data = NULL;
	q -> head -> next = NULL;
	q -> tail = q -> head;
	q -> q_len = 0;
	q->qlock=(pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
	q->qcond=(pthread_cond_t*) malloc(sizeof(pthread_cond_t));

	handle_error_0(pthread_mutex_init(q->qlock, NULL), "Errore nella creazione della mutex");
	handle_error_0(pthread_cond_init(q->qcond, NULL), "Errore nella creazione della condition");
	return q;
}

/*
 * @function deleteQueue
 * @brief cancella una coda allocata con initQueue -> chiamata da un solo thread
 * @param q  puntatore alla coda da cancellare
 */
void deleteQueue(queue_t *q){
	while(q -> head != q -> tail){
		node_t *p = (node_t*)q -> head;
		q -> head = q -> head -> next;
		freeNode(p);
	}
	if(q -> head) freeNode((void*)q -> head);
	pthread_mutex_destroy(q->qlock);
	pthread_cond_destroy(q->qcond);
	free(q->qlock);
	free(q->qcond);
	free(q);
}

/*
 * @function push
 * @brief inserisce un dato nella coda
 * @param q puntatore alla coda in cui inserire il dato
 * @param data puntatore al dato da inserire
 * @return  0 in caso di successivo
 *         -1 in caso di errore (setta errno)
 */
int push(queue_t *q, void* data){
  assert(data != NULL);
	node_t *n = allocNode();
	n->data = data;
	n->next = NULL;
	LockQueue(q);
	q->tail->next = n;
	q->tail       = n;
	q->q_len      += 1;
	UnlockQueueAndSignal(q);
	return 0;
/*	LockQueue(q);
	if(q->head->next ==NULL){
		//La coda è vuota
		q->head=calloc(1, sizeof(node_t));
		if(q->head==NULL) return -1;
		q->tail=q->head;
		q->head->next= n;
		//q->head->next=NULL;
		//Incremento il numero di elementi
		q->q_len+=1;
	}else{
		//La coda ha più di un elemento
		q -> tail -> next = n;
		q -> tail = n;
		q -> q_len += 1;
	}
	UnlockQueueAndSignal(q);
	return 0;*/

}

/*
 * @function pop
 * @brief estrae un dato nella coda
 * @param q puntatore alla coda da cui estrarre il dato
 * @return  puntatore al dato estratto
 */
void* pop(queue_t *q){
	LockQueue(q);
	while(q -> head == q -> tail){
		UnlockQueueAndWait(q);
	}
  //locked
	assert(q -> head -> next);
	node_t *n = (node_t *)q -> head;
	void* data = (q -> head -> next) -> data;
	q -> head = q -> head -> next;
	q -> q_len -= 1 ;
	assert(q -> q_len >= 0);
	UnlockQueue(q);
	freeNode(n);
	return data;
}

/*
 * @function length
 * @brief ritorna la lunghezza della coda. L'accesso non è in mutua esclusione
 * @param q puntatore alla coda di cui ritornare la lunghezza
 * @return  lunghezza della coda
 */
unsigned long length(queue_t *q){
	unsigned long len = q -> q_len;
	return len;
}
/*
 * @function toEmpty
 * @brief La funzione svuota la coda (eliminandone tutti gli elementi)
 * @param q La coda da svuotare
 * @return int 0 in caso di successo, altrimenti -1 ed errno settato.
 */
int toEmpty(queue_t *q){
	//Controllo di aver passato una coda non vuota
	if(q==NULL){
		//Setto errno
		errno=EINVAL;
		return -1;
	}
	else{
		//La coda non è vuota
		//Accedo in mutua esclusione alla coda
		LockQueue(q);
		//Pulisco la coda
		while(q -> head != q -> tail){
			node_t *p = (node_t*)q -> head;
			q -> head = q -> head -> next;
			freeNode(p);
		}
		q->head=NULL;
		q->tail=NULL;
		q->q_len=0;
		UnlockQueue(q);
	}
	return 0;
}

#endif /*QUEUE_C_ */
