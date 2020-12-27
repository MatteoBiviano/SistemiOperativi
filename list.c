/*
 * chatterbox Progetto del corso di LSO 2017
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
 /*
  * @file list.h
  * @brief Implementazione funzioni per operare su una lista doppiamente linkata
  *          concorrente contenente stringhe
  * @author Matteo Biviano 543933
  * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
  * originale dell'autore
  */
#if !defined(LIST_C_)
#define LIST_C_

#include <string.h>
#include <pthread.h>
#include <type.h>
#include <stdlib.h>
#include <errno.h>

#include "list.h"
/*
 *  @function initList
 *  @brief crea una lista di stringhe doppiamente linkata concorrente
 *  @return puntatore alla lista creata
 *           NULL in caso di errore (errno settato)
 */
list* initList(){
  //Alloco la nuova lista
  list* new=calloc(1, sizeof(list));
  //Controllo che sia stata correttamente allocata
  if(new){
    //Inizializzo la variabile di mutuaesclusione
    int mutex_val = pthread_mutex_init(&(new->mtx_list), NULL);
    //Controllo che tutto sia andato bene
    if(mutex_val != 0) {
      //Setto errno
      errno = mutex_val;
      //Libero il puntatore
      free(new);
      return NULL;
    }
    return new;
  }
  //L'allocazione non è avvenuta con successo
  return NULL;
}

/*
 *  @function push
 *  @brief La funzione inserisce un nodo
 *  @param l lista in cui inserire l'elemento
 *  @param nick  la stringa da inserire
 *  @return <0 in caso di errore (errno settato)
 */
int push(list *l,const char* nick){
  //Controllo i parametri d'inresso
  if(nick==NULL || l ==NULL){
    //Setto errno
    errno=EINVAL;
    //Esco indicando l'errore
    return -1;
  }
  //Salvo la lunghezza della stringa d'argomento
  size_t dim=strlen(nick);
  //Creo il nuovo nodo da inserire nella lista
  node* new=calloc(1,sizeof(node));
  //Controllo la corretta allocazione
  if(new){
    //Devo Copiare la stringa nel nodo
    new->nick=calloc(dim+1, sizeof(char));
    //Controllo la corretta allocazione
    if(new->nick){
      strncpy(new->nick, nick, dim);
      //Accedo in m.e. alla lista
      //Richiedo la m.e
      int mutex_val = pthread_mutex_lock(&(l->mtx_list));
      //Controllo che tutto sia andato bene
      if(mutex_val != 0) {
        //Setto errno
        errno = mutex_val;
        //Libero il puntatore
        return -1;
      }
      /**** DEVO INSERIRE L'ELEMENTO ****/
      //Se la testa è vuota
      if(l->testa == NULL) {
        //Inserisco l'elemento in testa
        l->testa  = new;
        //Dopo aver aggiunto l'elemento, incremento la dimensione della lista
        l->dim++;
      }else{
        //Devo inserire l'elemento in coda
        node *corr= l->testa;
        node *prev = NULL;
        //Scorro la lista
        while(corr != NULL) {
          //Se trovo un elemento uguale al precedente lancio errore
          if(strcmp(nick, corr->nick) == 0) {
            //Setto errno
            errno = EALREADY;
            //Rilascio la m.e.
            int mutex_val = pthread_mutex_unlock(&(l->mtx_list));
            //Controllo che tutto sia andato bene
            if(mutex_val != 0) {
              //Setto errno
              errno = mutex_val;
              return -1;
            }
            return -1;
          }
          //Altrimenti continuo a scorrere la lista
          prev = corr;
          corr = corr->next;
        }
        //Arrivato in fondo "prev" è l'ultimo elemento della lista
        prev->next = new;
        new->prev = prev;
        //Dopo aver aggiunto l'elemento, incremento la dimensione della lista
        l->dim++;
      }
      //Rilascio la m.e.
      int mutex_val = pthread_mutex_unlock(&(l->mtx_list));
      //Controllo che tutto sia andato bene
      if(mutex_val != 0) {
        //Setto errno
        errno = mutex_val;
        //Libero il puntatore
        return -1;
      }
      return 0;
    }
    //L'allocazione non è avvenuta con successo
    return -1;
  }
  //L'allocazione non è avvenuta con successo
  return -1;

}

/*
 *  @function pop
 *  @brief La procedura estra un elemento dalla lista
 *  @param l  lista da cui estrarre l'elemento
 *  @param nick elemento da rimuovere
 *  @return <0 in caso di errore (errno settato)
 */
int pop(list *l, const char* nick){
  //Controllo i parametri d'inresso
  if(nick==NULL || l ==NULL){
    //Setto errno
    errno=EINVAL;
    //Esco indicando l'errore
    return -1;
  }
  //Accedo in m.e. alla lista
  //Richiedo la m.e
  int mutex_val = pthread_mutex_lock(&(l->mtx_list));
  //Controllo che tutto sia andato bene
  if(mutex_val != 0) {
    //Setto errno
    errno = mutex_val;
    //Libero il puntatore
    return -1;
  }
  //Per cercare l'elemento uso un flag di controllo
  int trovato = 0;
  //Solo se la lista ha almeno un elemento posso effettuare la ricerca
  if(l->testa==NULL){
    //Setto errno
    errno = ENOENT;
    //Rilascio la m.e.
    int mutex_val = pthread_mutex_unlock(&(l->mtx_list));
    //Controllo che tutto sia andato bene
    if(mutex_val != 0) {
      //Setto errno
      errno = mutex_val;
      //Libero il puntatore
      return -1;
    }
    return -1;
  }
  //Variabile per scorrere la lista
  node* corr= l->testa;
  //Finché non arrivo alla fine
  while(corr!=NULL && !trovato){
    //Se trovo l'elemento
    if(strcmp(nick, corr->nick)==0){
      //Setto il flag
      trovato=1;
      /**** AGGIORNO LA LISTA ****/
      node* c_next= corr->next;
      node* c_prev= corr->prev;
      //Controllo il puntatore all'elemento precedente
      if(c_prev!=NULL){
        c_prev->next=c_next;
      }else{
        l->testa=c_next;
      }
      //Controllo il puntatore all'elemento successivo
      if(c_next!=NULL){
        c_next->prev=c_prev;
      }
      //Libero l'elemento
      free(corr->nick);
      free(corr);
    }
    //Vado avanti nella lista se non ho trovato l'elemento
    if(!trovato) corr=corr->next;
  }
  //Se non ho trovato l'elemento lancio errore
  if(!trovato){
    //Setto errno
    errno=ENOENT;
    //Rilascio la m.e.
    int mutex_val = pthread_mutex_unlock(&(l->mtx_list));
    //Controllo che tutto sia andato bene
    if(mutex_val != 0) {
      //Setto errno
      errno = mutex_val;
      //Libero il puntatore
      return -1;
    }
    return -1;
  }
  else{
    //Diminuisco la dimensione della lista
    l->dim--;
    //Rilascio la m.e.
    int mutex_val = pthread_mutex_unlock(&(l->mtx_list));
    //Controllo che tutto sia andato bene
    if(mutex_val != 0) {
      //Setto errno
      errno = mutex_val;
      //Libero il puntatore
      return -1;
    }
  }
  return 0;
}

/*
 * @function deleteNode
 * @brief La funzione elimina ricorsivamente tutti i nodi a partire da quello in argomento
 * @param head Nodo da cui partire
 */
void deleteNode(node *head) {
  //Controllo il parametro d'ingresso
  if(head!=NULL){
    //Chiamo ricorsivamente la procedura
    if(head->next != NULL){
      deleteNode(head->next);
    }
    //Libero il nodo
    free(head->nick);
    free(head);
  }
  //Il parametro d'ingresso è nullo
  return;
}

/*
 *  @function deleteList
 *  @brief libera la lista dalla memoria
 *  @param l puntatore alla lista da liberare
 *  @return <0 in caso di errore (errno settato)
 */
int deleteList(list *l){
  //Controllo i parametri
  if(l == NULL) {
    errno = EINVAL;
    return -1;
  }
  //Richiamo la procedura creata per eliminare i nodi
  deleteNode(l->head);
  int mutex_val = pthread_mutex_destroy(&(l->mtx_list));
  if(mutex_val !=0){
    //Setto errno
    errno=mutex_val;
    free(l);
    return -1;
  }
  free(l);
  return 0;
}

/*
 * @function listToArray
 * @brief Crea un'array contenente gli elementi della lista
 * @param l lista da cui creare l'array
 * @param array puntatore ad un array di stringhe
 * @return <0 in caso di errore (errno settato)
 */
int listToArray(list *l, char*** array){
  //Controllo i parametri d'ingresso
  if(array==NULL || l ==NULL){
    //Setto errno
    errno=EINVAL;
    //Esco
    return -1;
  }
  //Accedo in m.e. alla lista
  //Richiedo la m.e
  int mutex_val = pthread_mutex_lock(&(l->mtx_list));
  //Controllo che tutto sia andato bene
  if(mutex_val != 0) {
    //Setto errno
    errno = mutex_val;
    //Libero il puntatore
    return -1;
  }
  //Creo l'array di stringhe da riempire
  char** temp=calloc(l->dim, sizeof(char*));
  //Controlo che l'allocazione sia avvenuta con successo
  if(temp){
    //Salvo un puntatore alla testa della lista per scorrerla
    node* corr=l->testa;
    size_t j;
    //Uso un for per acceredere all'array temp
    for(j=0; j<l->dim; j++){
      //Salvo la lunghezza della stringa
      size_t length = strlen(corr->nick);
      //Alloco lo spazio per contenere corr->nick
      temp[i]=calloc(length+1, sizeof(char));
      //Copio la stringa in temp[i]
      strncpy(temp[i], corr->nick, length);
      if(temp[i]){
        //L'allocazione è avvenuta correttamente
        //Vado avanti nella lista e nell'array
        corr=corr->next;
      }
      else{
        //Rilascio la m.e.
        int mutex_val = pthread_mutex_unlock(&(l->mtx_list));
        //Controllo che tutto sia andato bene
        if(mutex_val != 0) {
          //Setto errno
          errno = mutex_val;
          //Libero il puntatore
          return -1;
        }
        return -1;
      }
    }
    //Salvo l'array nel buffer corrispondente
    *array=temp;
    //Rilascio la m.e.
    int mutex_val = pthread_mutex_unlock(&(l->mtx_list));
    //Controllo che tutto sia andato bene
    if(mutex_val != 0) {
      //Setto errno
      errno = mutex_val;
      //Libero il puntatore
      return -1;
    }
    //Se è andato tutto bene restituisco 0
    return 0;
  }
  else {
    //Rilascio la m.e.
    int mutex_val = pthread_mutex_unlock(&(l->mtx_list));
    //Controllo che tutto sia andato bene
    if(mutex_val != 0) {
      //Setto errno
      errno = mutex_val;
      //Libero il puntatore
      return -1;
    }
    return -1;
  }
}
#endif /* LIST_C_ */
