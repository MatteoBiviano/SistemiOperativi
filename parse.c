/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */

/*
 * @file parse.c
 * @brief File contenente la definizione della funzione per la configurazione del server
 *
 * Si dichiara il contenuto del file "parse.c" è opera originale
 * dello studente Matteo Biviano (matr. 543933)
 */
#if !defined(PARSE_C_)
#define PARSE_C_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse.h"
#include "utility.h"
#include "config.h"
/*
 * @function parse_config
 * @brief la funzione effettua il parsing del file di configurazione
 * @param  cnf       puntatore alla struttura dati dove memorizzare la configurazione del serve
 * @param filename  nome del file sul quale effettuare il parsing
 * @return la funzione restituisce 0 se la configurazione è andata a buon fine; -1 per segnalare un errore
 */

int parse_config(s_config *cnf, char *fileName){
	//Apertura del file di configurazione, in lettura
	FILE *stream = fopen(fileName, "r");
	//Controllo corretta apertura del file
	//handle_error_NULL(stream, "Non è possibile aprire il file di configurazione");
	if(stream==NULL) return -1;
	/* Per determinare la dimensione del file
	   ci posizioniamo alla fine e usiamo l'istruzione ftell
     */
	//fseek(stream, 0, SEEK_END); //sposta di 0 rispetto alla fine del file
	//int size = ftell(stream);
	//fseek(stream, 0, SEEK_SET); //sposta di 0 rispetto all'inizio del file

	//Definizione riga
	char *line = (char*)malloc(MAX_LINE_LENGTH * sizeof(char));
	//Definizione di chiave
	char *key = (char*)malloc(MAX_LINE_LENGTH*sizeof(char));
	//Definizione di valore
	char *val = (char*)malloc(MAX_LINE_LENGTH*sizeof(char));


	/* Guardia del while: "finchè si hanno righe da leggere"
	 * la funzione fgets ha 3 argomenti:
	 * 	1) il vettore di caratteri "line" nel quale memorizzare la riga del file
	 * 	2) il numero massimo di caratteri che si vogliono mettere in questa riga
	 * 	3) il descrittore del file da leggere
	 */
	while(fgets(line, MAX_LINE_LENGTH, stream)!=NULL){
		//Acquisisco la dimensione della riga
		int size_line = strlen(line);

		//Controllo se è vuota o è una riga di commento,
		//in modo da saltare questa iterazione
		if(line[0]== '#' || size_line<=1) continue;

		sscanf(line, "%s = %s", key, val);
		int size_key = strlen(key);
		int size_val = strlen(val);
		if(size_key==0 || size_val==0) continue;
		if (strcmp(key, "UnixPath") == 0) {
        	cnf->UnixPath = (char *)malloc((size_val + 1) * sizeof(char));
      		strncpy(cnf->UnixPath, val, size_val + 1);
        }
        else if ( strcmp(key, "DirName") == 0) {
        	cnf->DirName = (char *)malloc((size_val + 1) * sizeof(char));
	    	strncpy(cnf->DirName, val, size_val + 1);
	    }
	    else if ( strcmp(key, "StatFileName") == 0) {
        	cnf->StatFileName = (char *)malloc((size_val + 1) * sizeof(char));
	    	strncpy(cnf->StatFileName, val, size_val + 1);
    	}
    	else if (strcmp(key, "MaxConnections") == 0) {
      		cnf->MaxConnections = atoi(val);
    	}
    	else if (strcmp(key, "ThreadsInPool") == 0) {
      		cnf->ThreadsInPool = atoi(val);
    	}
    	else if (strcmp(key, "MaxMsgSize") == 0) {
      		cnf->MaxMsgSize = atoi(val);
    	}
    	else if (strcmp(key, "MaxFileSize") == 0) {
      		cnf->MaxFileSize = atoi(val);
    	}
    	else if (strcmp(key, "MaxHistMsgs") == 0) {
      		cnf->MaxHistMsgs = atoi(val);
    	}
	}
	free(line);
	free(key);
	free(val);
	fclose(stream);
	return 0;
}
/*
 * @function print_config
 * @brief la funzione stampa la configurazione memorizzata
 *
 * @param  cnf  puntatore alla struttura dati da cui stampare la configurazione
 */
void print_config(s_config cnf){
	// Stampo le configurazioni salvate nella struttura dati cnf
  	printf("-- Le Configurazioni lette sono: --\n");
  	if (cnf.UnixPath != NULL)
      printf("\t 1) %s  = %s\n", "UnixPath", cnf.UnixPath);
    printf("\t 2) %s  = %d\n", "MaxConnections", cnf.MaxConnections);
  	printf("\t 3) %s  = %d\n", "ThreadsInPool", cnf.ThreadsInPool);
  	printf("\t 4) %s  = %d\n", "MaxMsgSize", cnf.MaxMsgSize);
  	printf("\t 5) %s  = %d\n", "MaxFileSize", cnf.MaxFileSize);
  	printf("\t 6) %s  = %d\n", "MaxHistMsgs", cnf.MaxHistMsgs);
  	if (cnf.DirName != NULL)
    	printf("\t 7) %s  = %s\n", "DirName", cnf.DirName);
  	if (cnf.StatFileName != NULL)
    	printf("\t 8) %s  = %s\n", "StatFileName", cnf.StatFileName);
}
#endif /* PARSE_C_ */
