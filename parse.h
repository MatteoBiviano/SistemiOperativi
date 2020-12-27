/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */

/*
 * @file parse.h
 * @brief File contenente la struttura e la funzione per la configurazione del server
 *
 * Si dichiara il contenuto del file "parse.h" è opera originale
 * dello studente Matteo Biviano (matr. 543933)
 */
#if !defined(PARSE_H_)
#define PARSE_H_
/*
 *  @struct s_config
 *  @brief struttura che serve per la configurazione del server
 *  @var   UnixPath        path utilizzato per la creazione del socket AF_UNIX
 *  @var   MaxConnections  numero massimo di connessioni concorrenti gestite dal server
 *  @var   ThreadsInPool   numero di thread nel pool
 *  @var   MaxMsgSize      dimensione massima di un messaggio
 *  @var   MaxFileSize     dimensione massima di un file accettato dal server (kilobytes)
 *  @var   MaxHistMsgs     numero massimo di messaggi che il server 'ricorda' per ogni client
 *  @var   DirName         directory dove memorizzare i files da inviare agli utenti
 *  @var   StatFileName    file nel quale verranno scritte le statistiche
 */
typedef struct s_config_{
  char *UnixPath;
  int MaxConnections;
  int ThreadsInPool;
  int MaxMsgSize;
  int MaxFileSize;
  int MaxHistMsgs;
  char *DirName;
  char *StatFileName;
}s_config;

/*
 * @function parse_config
 * @brief la funzione effettua il parsing del file di configurazione
 * @param  cnf -> puntatore alla struttura dati dove memorizzare la configurazione del serve
 * @param filename nome del file sul quale effettuare il parsing
 * @return la funzione restituisce 0 se la configurazione è andata a buon fine; -1 per segnalare un errore
 */

int parse_config(s_config *cnf, char *filename);
/*
 * @function print_config
 * @brief la funzione stampa la configurazione memorizzata
 * @param  cnf  puntatore alla struttura dati da cui stampare la configurazione
 */
void print_config(s_config cnf);

#endif /* PARSE_H_ */
