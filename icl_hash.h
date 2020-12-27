
/*
 * @file icl_hash.h
 * @brief Header file for icl_hash routines.
 * Il file è stato preso e modificato dalle esercitazioni di laboratorio
 */
/* $Id$ */
/* $UTK_Copyright: $ */

#ifndef icl_hash_h
#define icl_hash_h

#include <stdio.h>
//Includiamo il file di configurazione contenente le define da utilizzare
#include "config.h"
//Includiamo la libreria che ci permette di usare la mutuaesclusione
#include <pthread.h>

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

typedef struct icl_entry_s {
    void* key;
    void *data;
    struct icl_entry_s* next;
} icl_entry_t;

typedef struct icl_hash_s {
    int nbuckets;
    int nentries;
    icl_entry_t **buckets;
    unsigned int (*hash_function)(void*);
    int (*hash_key_compare)(void*, void*);
    /*
     * Aggiungo l'array di mutex per l'accesso in mutua escusione della tabella
     * partiziono la tabella in HASH_CLUSTERS regioni da accedere in m.e.
     */
    pthread_mutex_t mtx_cluster[HASH_CLUSTERS];
} icl_hash_t;

icl_hash_t *
icl_hash_create( int nbuckets, unsigned int (*hash_function)(void*), int (*hash_key_compare)(void*, void*) );

void
* icl_hash_find(icl_hash_t *, void* );

icl_entry_t
* icl_hash_insert(icl_hash_t *, void*, void *);

int
icl_hash_destroy(icl_hash_t *, void (*)(void*), void (*)(void*)),
    icl_hash_dump(FILE *, icl_hash_t *);

int icl_hash_delete( icl_hash_t *ht, void* key, void (*free_key)(void*), void (*free_data)(void*) );

/*
 * @function hashLock
 * @brief La procedura richiede il lock sulla sezione a cui appartiene data
 * @param table tabella hash su quale chiamare la lock
 * @param data  valore presente all'interno della tabella
 */
void hashLock(icl_hash_t* hash, unsigned int data);

/*
 * @function hashUnLock
 * @brief La procedura rilascia il lock sulla sezione a cui appartiene data
 * @param table tabella hash su quale rilasciare la lock
 * @param data  valore presente all'interno della tabella
 */
void hashUnLock(icl_hash_t* hash, unsigned int data);

//AGGIUNGO DUE FUNZIONI PER LIBERARE LA TABELLA
/*
 * @function free_hashtable_key
 * @brief La funzione libera la chiave dell'hashtable
 * @param la chiave da liberare
 */
void free_hashtable_key(void* key);
/*
 * @function free_hashtable_data
 * @brief La funzione libera il valore dell'hashtable
 * @param il valore da liberare 
 */
void free_hashtable_data(void* data);

#define icl_hash_foreach(ht, tmpint, tmpent, kp, dp)    \
    for (tmpint=0;tmpint<ht->nbuckets; tmpint++)        \
        for (tmpent=ht->buckets[tmpint];                                \
             tmpent!=NULL&&((kp=tmpent->key)!=NULL)&&((dp=tmpent->data)!=NULL); \
             tmpent=tmpent->next)


#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif /* icl_hash_h */
