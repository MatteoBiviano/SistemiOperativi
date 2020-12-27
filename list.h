/*
 * chatterbox Progetto del corso di LSO 2017
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
 /*
  * @file list.h
  * @brief Dichiarazioni strutture e funzioni per operare su una lista doppiamente linkata
  *          concorrente contenente stringhe
  * @author Matteo Biviano 543933
  * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
  * originale dell'autore
  */
#if !defined(LIST_H_)
#define LIST_H_

#include <pthread.h>
#include <type.h>

/*
 * @struct node
 * @brief La struttura rappresenta il nodo di una lista
 * @var prev puntatore al nodo precendente
 * @var nick valore del nodo (cioè, una stringa)
 * @var next puntatore al nodo successivo
 */
typedef struct struct_node {
  struct struct_node *prev;
  char* nick;
  struct struct_node *next;
} node;

/*
 * @struct list
 * @brief La struttura rappresenta una lista concorrente
 * @var testa puntatore alla testa della lista
 * @var mtx_list variabili di mutuaesclusione per gestire la concorrenza
 * @var dim lunghezza della lista
 */
typedef struct struct_list{
  node* testa;
  pthread_mutex_t mtx_list;
  size_t dim;

}list;

/*
 *  @function initList
 *  @brief crea una lista di stringhe doppiamente linkata concorrente
 *  @return puntatore alla lista creata
 *           NULL in caso di errore (errno settato)
 */
list* initList();

/*
 *  @function push
 *  @brief La funzione inserisce un nodo
 *  @param l lista in cui inserire l'elemento
 *  @param nick  la stringa da inserire
 *  @return <0 in caso di errore (errno settato)
 */
int push(list *l,const char* nick);

/*
 *  @function pop
 *  @brief La procedura estra un elemento dalla lista
 *  @param l  lista da cui estrarre l'elemento
 *  @param nick elemento da rimuovere
 *  @return <0 in caso di errore (errno settato)
 */
int pop(list *l, const char* nick);

/*
 * @function deleteNode
 * @brief La funzione elimina ricorsivamente tutti i nodi a partire da quello in argomento
 * @param head Nodo da cui partire
 */
void deleteNode(node *head);

/*
 *  @function deleteList
 *  @brief libera la lista dalla memoria
 *  @param l puntatore alla lista da liberare
 *  @return <0 in caso di errore (errno settato)
 */
int deleteList(list *l);

/*
 * @function listToArray
 * @brief Crea un'array contenente gli elementi della lista
 * @param l lista da cui creare l'array
 * @param array puntatore ad un array di stringhe
 * @return <0 in caso di errore (errno settato)
 */
int listToArray(list *l, char*** array);

#endif /* LIST_H_ */
