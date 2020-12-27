/**
 * @file icl_hash.c
 *
 * Dependency free hash table implementation.
 *
 * This simple hash table implementation should be easy to drop into
 * any other peice of code, it does not depend on anything else :-)
 *
 * @author Jakub Kurzak
 */
/* $Id: icl_hash.c 2838 2011-11-22 04:25:02Z mfaverge $ */
/* $UTK_Copyright: $ */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <limits.h>

#include "icl_hash.h"
#include "config.h"
#include "history.h"




#define BITS_IN_int     ( sizeof(int) * CHAR_BIT )
#define THREE_QUARTERS  ((int) ((BITS_IN_int * 3) / 4))
#define ONE_EIGHTH      ((int) (BITS_IN_int / 8))
#define HIGH_BITS       ( ~((unsigned int)(~0) >> ONE_EIGHTH ))
/**
 * A simple string hash.
 *
 * An adaptation of Peter Weinberger's (PJW) generic hashing
 * algorithm based on Allen Holub's version. Accepts a pointer
 * to a datum to be hashed and returns an unsigned integer.
 * From: Keith Seymour's proxy library code
 *
 * @param[in] key -- the string to be hashed
 *
 * @returns the hash index
 */
static unsigned int
hash_pjw(void* key)
{
    char *datum = (char *)key;
    unsigned int hash_value, i;

    if(!datum) return 0;

    for (hash_value = 0; *datum; ++datum) {
        hash_value = (hash_value << ONE_EIGHTH) + *datum;
        if ((i = hash_value & HIGH_BITS) != 0)
            hash_value = (hash_value ^ (i >> THREE_QUARTERS)) & ~HIGH_BITS;
    }
    return (hash_value);
}

static int string_compare(void* a, void* b)
{
    return (strcmp( (char*)a, (char*)b ) == 0);
}


/**
 * Create a new hash table.
 *
 * @param[in] nbuckets -- number of buckets to create
 * @param[in] hash_function -- pointer to the hashing function to be used
 * @param[in] hash_key_compare -- pointer to the hash key comparison function to be used
 *
 * @returns pointer to new hash table.
 * @discussion ho aggiunto l'inizializzazione del vettore di mutex
 */

icl_hash_t *
icl_hash_create( int nbuckets, unsigned int (*hash_function)(void*), int (*hash_key_compare)(void*, void*) )
{
    icl_hash_t *hash;
    int i;

    hash = (icl_hash_t*) malloc(sizeof(icl_hash_t));
    if(!hash) return NULL;

    hash->nentries = 0;
    hash->buckets = (icl_entry_t**)malloc(nbuckets * sizeof(icl_entry_t*));
    if(!hash->buckets) return NULL;

    hash->nbuckets = nbuckets;
    for(i=0;i<hash->nbuckets;i++)
        hash->buckets[i] = NULL;

    hash->hash_function = hash_function ? hash_function : hash_pjw;
    hash->hash_key_compare = hash_key_compare ? hash_key_compare : string_compare;
    //Inizializzo il vettore di variabili di mutuaesclusione
    int j;
    for(j=0; j<HASH_CLUSTERS; j++){
      pthread_mutex_init(&(hash->mtx_cluster[j]), NULL);
    }
    return hash;
}

/**
 * Search for an entry in a hash table.
 *
 * @param ht -- the hash table to be searched
 * @param key -- the key of the item to search for
 *
 * @returns pointer to the data corresponding to the key.
 *   If the key was not found, returns NULL.
 */

void *
icl_hash_find(icl_hash_t *ht, void* key)
{
    icl_entry_t* curr;
    unsigned int hash_val;

    if(!ht || !key) return NULL;

    hash_val = (* ht->hash_function)(key) % ht->nbuckets;
    //Invoco la lock sulla sezione contenente hash_val
    hashLock(ht, hash_val);

    for (curr=ht->buckets[hash_val]; curr != NULL; curr=curr->next)
        if ( ht->hash_key_compare(curr->key, key)){
          //Rilascio la lock sulla sezione contenente hash_val
          hashUnLock(ht, hash_val);
          return(curr->data);
        }
    //Se non ho trovato il valore rilascio la lock ed esco
    hashUnLock(ht, hash_val);
    return NULL;
}

/**
 * Insert an item into the hash table.
 *
 * @param ht -- the hash table
 * @param key -- the key of the new item
 * @param data -- pointer to the new item's data
 *
 * @returns pointer to the new item.  Returns NULL on error.
 */

icl_entry_t *
icl_hash_insert(icl_hash_t *ht, void* key, void *data)
{
    icl_entry_t *curr;
    unsigned int hash_val;

    if(!ht || !key) return NULL;

    hash_val = (* ht->hash_function)(key) % ht->nbuckets;

    //Richiedo la lock per la sezione contente hash_val
    hashLock(ht, hash_val);
    for (curr=ht->buckets[hash_val]; curr != NULL; curr=curr->next)
        if ( ht->hash_key_compare(curr->key, key)){
          //Rilascio la lock per la sezione contenete hash_val
          hashUnLock(ht, hash_val);
          return(NULL); /* key already exists */
        }

    //Non sono uscito dal programma: ancora non ho rilasciato la lock

    /* if key was not found */
    curr = (icl_entry_t*)malloc(sizeof(icl_entry_t));
    if(!curr){
      //Rilascio la lock per la sezione contenete hash_val
      hashUnLock(ht, hash_val);
      return NULL;
    }

    //Non sono uscito dal programma: ancora non ho rilasciato la lock

    curr->key = key;
    curr->data = data;
    curr->next = ht->buckets[hash_val]; /* add at start */

    ht->buckets[hash_val] = curr;
    ht->nentries++;
    //Rilascio la lock per la sezione contenete hash_val, prima di uscire dal programma
    hashUnLock(ht, hash_val);
    return curr;
}

/**
 * Free one hash table entry located by key (key and data are freed using functions).
 *
 * @param ht -- the hash table to be freed
 * @param key -- the key of the new item
 * @param free_key -- pointer to function that frees the key
 * @param free_data -- pointer to function that frees the data
 *
 * @returns 0 on success, -1 on failure.
 */
int icl_hash_delete(icl_hash_t *ht, void* key, void (*free_key)(void*), void (*free_data)(void*))
{
    icl_entry_t *curr, *prev;
    unsigned int hash_val;

    if(!ht || !key) return -1;
    hash_val = (* ht->hash_function)(key) % ht->nbuckets;

    prev = NULL;

    //Richiedo la lock per la sezione contente hash_val
    hashLock(ht, hash_val);

    for (curr=ht->buckets[hash_val]; curr != NULL; )  {
        if ( ht->hash_key_compare(curr->key, key)) {
            if (prev == NULL) {
                ht->buckets[hash_val] = curr->next;
            } else {
                prev->next = curr->next;
            }
            if (*free_key && curr->key) (*free_key)(curr->key);
            if (*free_data && curr->data) (*free_data)(curr->data);
            ht->nentries++;
            free(curr);
            //Rilascio la lock per la sezione contenete hash_val
            //prima di uscire dal programma
            hashUnLock(ht, hash_val);
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }
    //Rilascio la lock per la sezione contenete hash_val
    //prima di uscire dal programma
    hashUnLock(ht, hash_val);
    return -1;
}

/**
 * Free hash table structures (key and data are freed using functions).
 *
 * @param ht -- the hash table to be freed
 * @param free_key -- pointer to function that frees the key
 * @param free_data -- pointer to function that frees the data
 *
 * @returns 0 on success, -1 on failure.
 */
int
icl_hash_destroy(icl_hash_t *ht, void (*free_key)(void*), void (*free_data)(void*))
{
    icl_entry_t *bucket, *curr, *next;
    int i;

    if(!ht) return -1;

    for (i=0; i<ht->nbuckets; i++) {
        bucket = ht->buckets[i];
        for (curr=bucket; curr!=NULL; ) {
            next=curr->next;
            if (*free_key && curr->key) (*free_key)(curr->key);
            if (*free_data && curr->data) (*free_data)(curr->data);
            free(curr);
            curr=next;
        }
    }
    //Distruggo le mutex che permettono di accedere in m.e. alle regioni
    int j=0;
    for(j=0; j<HASH_CLUSTERS; j++){
      pthread_mutex_destroy(&(ht->mtx_cluster[j]));
    }
    if(ht->buckets) free(ht->buckets);
    if(ht) free(ht);

    return 0;
}

/**
 * Dump the hash table's contents to the given file pointer.
 *
 * @param stream -- the file to which the hash table should be dumped
 * @param ht -- the hash table to be dumped
 *
 * @returns 0 on success, -1 on failure.
 */

int
icl_hash_dump(FILE* stream, icl_hash_t* ht)
{
    icl_entry_t *bucket, *curr;
    int i;

    if(!ht) return -1;

    for(i=0; i<ht->nbuckets; i++) {
        bucket = ht->buckets[i];
        for(curr=bucket; curr!=NULL; ) {
            if(curr->key)
                fprintf(stream, "icl_hash_dump: %s: %p\n", (char *)curr->key, curr->data);
            curr=curr->next;
        }
    }

    return 0;
}

/***** AGGIUNGIAMO PROVEDURE PER GESTIRE LA MUTUASCLUSIONE DELLA TABELLA *****/
/*
 * @function hashLock
 * @brief La procedura richiede il lock sulla sezione a cui appartiene data
 * @param table tabella hash su quale chiamare la lock
 * @param data  valore presente all'interno della tabella
 */
void hashLock(icl_hash_t* hash, unsigned int data){
  //Cerco l'indice della mutex
  //Prendo la parte intera inferiore
  int index= (int) data/HASH_SECTION;
  //richiedo la mutuaesclusione
  pthread_mutex_lock(&(hash->mtx_cluster[index]));
}

/*
 * @function hashUnLock
 * @brief La procedura rilascia il lock sulla sezione a cui appartiene data
 * @param table tabella hash su quale rilasciare la lock
 * @param data  valore presente all'interno della tabella
 */
void hashUnLock(icl_hash_t* hash, unsigned int data){
  //Cerco l'indice della mutex
  //Prendo la parte intera inferiore
  int index= (int) data/HASH_SECTION;
  //richiedo la mutuaesclusione
  pthread_mutex_unlock(&(hash->mtx_cluster[index]));
}

/*
 * @function free_hashtable_key
 * @brief La funzione libera la chiave dell'hashtable
 * @param key la chiave da liberare
 */
void free_hashtable_key(void* key){
  free((char*) key);
}
/*
 * @function free_hashtable_data
 * @brief La funzione libera il valore dell'hashtable
 * @param il valore da liberare
 */
void free_hashtable_data(void* data){
  //Per gli utenti il valore è la historyQueue
  //richiamo la funzione della libreria history.h
  deleteList((list*)data);
}
