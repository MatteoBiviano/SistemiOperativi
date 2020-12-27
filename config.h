/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file config.h
 * @brief File contenente alcune define con valori massimi utilizzabili
 */

#if !defined(CONFIG_H_)
#define CONFIG_H_

//Lunghezza massima del nome di un utente o di un gruppo
#define MAX_NAME_LENGTH                  32

/* aggiungere altre define qui */

/*
 * Numero massimo di caratteri che viene considerato siano presenti
 * in una riga nel file di configurazione
 */
#if !defined MAX_LINE_LENGTH
#define MAX_LINE_LENGTH 256
#endif /* MAX_LINE_LENGTH */

/* Numero massimo di richieste pendenti di connessione.
 * Se questo numero viene superato la connect potrebbe bloccarsi
 * NOTA: questa define serve per il controllo della listen nel server chatty
 */
#if !defined LISTEN_BACKLOG
#define LISTEN_BACKLOG 50
#endif /* LISTEN_BACKLOG */

/* Numero totale di buckets in cui l'hashtable viene suddivisa
 * usiamo come valore una potenza di 2 (2^9)
 */
#if !defined HASH_BSIZE
#define HASH_BSIZE 512
#endif /* HASH_BSIZE */

/* In quante regioni suddividere l'hashtable
 * per l'accesso in mutua esclusione
 */
#if !defined HASH_CLUSTERS
#define HASH_CLUSTERS 32
#endif /* HASH_CLUSTERS */

/*
 * Numero di entry in ogni regione dell'hashtable
 */
#if !defined HASH_SECTION
#define HASH_SECTION ((HASH_BSIZE)/(HASH_CLUSTERS))
#endif /* HASH_SECTION */


// to avoid warnings like "ISO C forbids an empty translation unit"
typedef int make_iso_compilers_happy;

#endif /* CONFIG_H_ */
