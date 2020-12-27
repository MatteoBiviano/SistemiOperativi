/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file chatty.c
 * @brief File principale del server chatterbox
 * Si dichiara il contenuto del file "parse.h" è opera originale
 * dello studente Matteo Biviano (matr. 543933)
 */
#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* inserire gli altri include che servono */
#include "config.h"
#include "connections.h"
#include "icl_hash.h"
#include "message.h"
#include "parse.h"
#include "queue.h"
#include "stats.h"
#include "history.h"
#include "utility.h"
#include "server.h"
#include "list.h"

#include <sys/mman.h>
#include <sys/select.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h> //Usato per il timeout della select
#include <sys/types.h> //Usato ad esempio per il tipo "ssize"
#include <sys/un.h> //Usato per indirizzo AF_UNIX



/************************* RISORSE UTILIZZATE *********************/

/* Variabile usata per memorizzare il segnale ricevuto
 * La definizione come "volatile sig_atomic_t" permette l'accesso
 * signal safe alla variabile globale "sigalarm_flag"
 */
volatile sig_atomic_t sigalarm_flag;

//Variabile usata come FLAG di controllo per la terminazione dei thread
//int interrupted;

/************************* STRUTTURE UTILIZZATE *********************/
/* @struct chattyStats
 * @brief La struttura memorizza le statistiche del server
 *        è definita nel file stats.h
 */
struct statistics chattyStats = { 0,0,0,0,0,0,0 };

/* @struct configuration
 * @brief La struttura memorizza le configurazioni del server
 *        è definita nel file parse.h
 */
s_config configuration = {NULL, 0, 0, 0, 0, 0, NULL, NULL};

//Insieme dei file descriptor attivi
fd_set setfd;

/*
 * Coda dei descrittori pronti, usati per la gestione delle richieste dei client
 * Per esempio:
 * - usata nell'handler di terminazione
 * - usata nel ciclo della accept
 */
queue_t* queue_descriptor;

//Tabella degli utenti registrati
icl_hash_t *hash_user;

//Vettore dei client connessi
conn_user *vect_connected;

//Tabella dei gruppi registrati
///////// icl_hash_t *hash_group;

//ThreadPool
pthread_t *t_pool;


/************************* VARIABILI DI MUTUAESCLUSIONE *********************/
//Variabile di mutuaesclusione per l'accesso al bitset dei descrittori
pthread_mutex_t mtx_setfd = PTHREAD_MUTEX_INITIALIZER;

//Variabile di mutuaesclusione per l'accesso alle statistiche
pthread_mutex_t mtx_stat = PTHREAD_MUTEX_INITIALIZER;

//Variabile di mutuaesclusione per l'accesso agli utenti connessi
pthread_mutex_t mtx_conn = PTHREAD_MUTEX_INITIALIZER;

/************************* PROCEDURE USATE COME GESTORI/HANDLERS *********************/
/*
 *  @function gestore_segnali
 *  @brief La procedura rappresenta un gestore che salva il segnale arrivato
 *  @param segnale rappresenta il segnale da salvare, tra i seguenti:
 *        - SIGINT
 *        - SIGQUIT
 *        - SIGTERM
 *        - SIGUSR1
 *  @discussion la gestione vera e propria del segnale
 *            verrà fatta esplicitamente  all'interno del programma
 *            Questo per rendere "gestore_segnali" il più "signal safe" possibile
 */
static void gestore_segnali(int segnale);

/*
 *  @function handle_signals
 *  @brief La procedura si occupa di registrare i segnali
 */
static void handle_signals();

/************* FUNZIONI USATE PER IMPLEMENTARE LE OPERAZIONI DEL SERVER *************/

/*
 * @function free_user
 * @btrief La procedura libera la memoria occupata da un utente
 * @param ptr Puntatore alla struttura che bisogna deallocare
 */
void free_user(void *ptr);

/*
 * @function findClient
 * @brief La funzione restituisce l'indice del client associato al descrittore "descriptor"
 * @param descriptor Il descrittore da cercare
 * @return NULL se non troviamo un client connesso relativo a "descriptor"
 * @discussion il chiamante deve aver richiesto la mutuaesclusione prima di invocare la funzione
 */
static int findClient(long descriptor);

/*
 * @function free_msg
 * @brief La procedura libera la memoria occupata da un messaggio
 * @param msg Puntatore alla struttura che bisogna deallocare
 */
void free_msg(void *ptr);

/*
 * @funciton registerUser
 * @brief Registra un nuovo utente se non è già presente
 * @param nickname    nickname da registrare
 * @param descriptor  descrittore della connessione
 *
 */
void registerUser(char* nickname, long descriptor);

/*
 * @function connectUser
 * @brief  connette l'utente con nickname "nickname"
 * @param nickname	   nickname dell'utente da connettere
 * @param descriptor      descrittore della connessione
 */
void connectUser(char* nickname, long descriptor);
/*
 * @function postTxt
 * @brief  Invia un messaggio testuale ad un altro utente
 * @param msg         il messaggio da inviare
 * @param descriptor  descrittore della connessione del mittente
 *
 */
void postTxt(message_t msg, long descriptor);


/*
 * @function execute
 * @brief La procedura si occupa di gestire le richieste
 *                          inviate al server dai client
 * @param msg         messaggio da elaborare
 * @param client_fd   file descriptor del client che invia la richiesta
 * @discussion    Il chiamante deve aver fatto tutti i controlli del caso
 *                prima di chiamare questa procedura
 */
void execute(message_t msg, long client_fd);

/************* FUNZIONE USATA PER IMPLEMENTARE LE OPERAZIONI DEI THREAD *************/
/*
 * @function worker_thread
 * @brief La funzione eseguita dai thread del pool
 * @param arg parametro passato
 * @discussion Usando strutture globali non ho bisogno di argomenti per la funzione
 * @return NULL
 */
void* worker_thread(void* arg);


/*
 * @function usage
 * @brief La funzione stampa a video le modalità di esecuzione del progetto
 * @param progname  Nome del progetto
 */
static void usage(const char *progname) {
    fprintf(stderr, "Il server va lanciato con il seguente comando:\n");
    fprintf(stderr, "  %s -f conffile\n", progname);
}

int main(int argc, char *argv[]) {
  //Controlliamo gli argomenti con cui viene eseguito il programma
	if(argc<3 || strncmp(argv[1], "-f", 2) !=0){
		//Si usa la funzione "usage" implementata dal docente
		usage(argv[0]);
		return 0;
	}

	//Parso il file di configurazione controllandone il corretto funzionamento
	printf("\t -- Parsing del file di configurazione --\n");
 	handle_error_1(parse_config(&configuration, argv[2]), "Errore durante il parsing del file di configurazione");
	//Stampo le configurazioni lette
	printf("\t -- Stampa delle configurazioni lette --\n");
	print_config(configuration);
	unlink(configuration.UnixPath);
	/*******REGISTRAZIONE DEI SEGNALI - Installazione dei Signal Handler*******/
	printf("\t -- Registrazione dei segnali --\n");
  handle_signals();

	/******* INIZIALIZZAZIONE DELLE RISORSE *******/

  //Inizializzo la coda dei descrittori prelevati dai worker
	printf("-- Creazione della coda dei descrittori --\n");
	handle_error_NULL((queue_descriptor = initQueue()), "Errore nell'inizializzazione della coda");

  //Creazione della tabella degli utenti registrati
	printf("-- Creazione della tabella degli utenti registrati --\n");
	icl_hash_t *hash_user = (icl_hash_t*) icl_hash_create(HASH_BSIZE, NULL, NULL);

  //Creazione della tabella dei gruppi registrati
	printf("-- Creazione della tabella dei gruppi registrati --\n");
	icl_hash_t *hash_group = icl_hash_create(HASH_BSIZE, NULL, NULL);

  //Creazione del vettore di client connessi
	printf("--Creazione del vettore dei client connessi --\n");
  handle_error_NULL((vect_connected= calloc(configuration.MaxConnections, sizeof(conn_user))),"Errore nella creazione del vettore dei client connessi");
	//Inizializzo il Vettore
  int i;
	for(i = 0; i < configuration.MaxConnections; i++) {
		//Inizializzo a -1 i descrittori
		vect_connected[i].descriptor = -1;
	}

  //Creazione del ThreadPool
  printf("-- Creazione del ThreadPool --\n");
  handle_error_NULL((t_pool= calloc(configuration.ThreadsInPool, sizeof(pthread_t))), "Errore nella creazione del ThreadPool");

	/******* CREAZIONE DEL SOCKET *******/
  printf("-- Creazione del socket --\n");
  //Socket di connessione
  int fd_skt=socket(AF_UNIX, SOCK_STREAM,0);
  //Controllo la creazione avvenuta
  handle_error_1(fd_skt, "Errore: Creazione del socket\n");

  //Descrittore usato per la accept, che indica il descrittore di un nuovo client
  // In altri termini, "Socket di I/O con un client"
  int fd_client;
  //Max fd attivo
  int fd_num=0;
  /* NOTA: invece di usare fd_num potrei usare FD_SETSIZE, in quanto la select non
   * gestisce descrittori più grandi di questo valore
   */
  //Indice per verificare risultati della select
  long i_fd; //Da usare come primo parametro della FD_ISSET e nel for
  //Insieme dei file descriptor attesi in lettura - per la Select
  fd_set read_set;
  //Valore di ritorno della select
  int retval;
  /* @struct timeval -> Timeout della select:
   * timeout.tv_sec  = 0 secondi
   * timeout.tv_usec = 10000 microsecondi
   */
  struct timeval timeout = {0, 10000};

  /***** BIND E LISTEN DEL SOCKET ******/
  //Struct sigaction per creare l'indirizzo su cui bindare il connection socket
  struct sockaddr_un sa; //ind AF_UNIX
  //ripuliamo la struttura
  memset(&sa, '0', sizeof(struct sockaddr_un));
  sa.sun_family=AF_UNIX;
  strncpy(sa.sun_path, configuration.UnixPath, UNIX_PATH_MAX+1);
  //Bind del socket
  handle_error_1(bind(fd_skt, (struct sockaddr*)&sa, sizeof(sa)), "Bind del socket");
  //Listen del socket - la define LISTEN_BACKLOG è presente nel file config.h
  handle_error_1(listen(fd_skt, LISTEN_BACKLOG), "Listen del socket");
  //Mantengo il massimo indice di descrittore attivo in fd_num
  if(fd_skt > fd_num) fd_num=fd_skt;
  //Serie di Macro per manipolare i descrittori
  FD_ZERO(&setfd);         //inizializza l'insieme dei descrittori con l'insieme vuoto
  FD_ZERO(&read_set);       //inizializza l'insieme dei descrittori con l'insieme vuoto
  FD_SET(fd_skt, &setfd);  //aggiunge il descrittore sock_fd all'insieme
  /*Da adesso possiamo accettare connessioni in entrata, una alla volta, usando l'accept */

  /***** CREAZIONE DEI THREAD ******/
  //Inizializzo il pool di thread
  printf("-- Creazione dei Thread --\n");
  for(i=0; i<configuration.ThreadsInPool; i++){
    printf("-- Creato il Thread di indice %d --\n", i);
    //Usando strutture globali non ho bisogno di passare argomenti ai thread
    handle_error_0((pthread_create(&t_pool[i], NULL, worker_thread, NULL)),"Errore nella creazione del thread\n");
  }

	printf("Inizializzazioni avvenute con successo. Server in ascolto...	\n");

	/* Creiamo il messaggio di risposta da inviare ai client in caso di
 	 * connessione non accettata, se si supera il numero massimo di connessioni
 	 */
	message_t risposta;
	//Inizializziamo il messaggio di rifiuto da inviare
  //Il messaggio verrà liberato alla chiusura del server
	errorMSG(&risposta, OP_FAIL, "Il server non può accettare la connessione");

  /******* CICLO DI ASCOLTO *******/
  /*
   * Cicliamo finchè sigalarm_flag non viene settato con un segnale di chiusura:
   *    - SIGINT
   *    - SIGQUIT
   *    - SIGTERM
   */
  while(!((sigalarm_flag==SIGINT) || (sigalarm_flag==SIGQUIT) || (sigalarm_flag==SIGTERM))){
    //Gestiamo la stampa delle statistiche se arriva il segnale corrispondente
    if(sigalarm_flag == SIGUSR1) {
      printf("--Stampa delle statistiche sul file--\n");
      FILE *file_stat= fopen(configuration.StatFileName, "a");
      if (file_stat != NULL) {
      	//La modifica del file deve essere gestita in mutuaesclusione
      	pthread_mutex_lock(&mtx_stat);
      	printStats(file_stat);
      	pthread_mutex_unlock(&mtx_stat);
      }
      //Chiudiamo il file delle statistiche per evitare memory leak
      fclose(file_stat);
      //Azzeriamo la variabile che controlla i segnali
      sigalarm_flag = 0;
    }

		/****** Copiamo il set di file descriptor *******/
    //Richiediamo la lock della mutex mtx_fdset
    pthread_mutex_lock(&mtx_setfd);
    //Preparo maschera per la SELECT
    read_set=setfd;
    //Liberiamo la lock della mutex mtx_fdset
    pthread_mutex_unlock(&mtx_setfd);

    /****** INVOCHIAMO LA SELECT ******/
    //fd_num+1 perché vogliamo il numero di descrittori attivi, non l'indice massimo
    //NOTA: potrei usare FD_SETSIZE come detto su
    retval=select(fd_num+1, &read_set, NULL, NULL, &timeout);
    if(retval<0){
  		// Per Debug: perror("Errore select");
      continue;
			/*  La select ha fallito:
       *  - forse per interruzione = quindi usciamo dal while con interrupetd=0;
       *  - forse per la richiesta della stampa delle statistiche
       *  In caso contrario, rieseguo il ciclo e ritento la select
       */
		}
		else{	//SELECT OK
      //Scansiono la maschera
      for(i_fd=0; i_fd < fd_num+1; i_fd++){
        /* FD_ISSET:
         * = 1 se il bit corrispondente al file descriptor i_fd nella maschera
         *        read_set era a 1
         * = 0 altrimenti
         */
				if(FD_ISSET(i_fd, &read_set)){
          //è stata ricevuta una richiesta di connessione su i_fd
  				if(i_fd==fd_skt){
            //Socket di connessione pronto
  					//Connetto un nuovo client
  					handle_error_1((fd_client=accept(fd_skt, NULL, 0)), "Errore nell'accept");
            printf("Accetto il client con descrittore %d\n" ,fd_client);

  					/* Devo controllare di non aver superato il numero di connessioni
  					 * uso un FLAG di controllo
  		  	   */
  					int val=0;
  					//Devo controllare nelle statistiche -> accedendo in mutuaesclusione
  					pthread_mutex_lock(&mtx_stat);
  					if(configuration.MaxConnections <= chattyStats.nonline){
  						//Dobbiamo rifiutare la connessione perché superiamo il tetto massimo
  						fprintf(stdout, "La connessione di %ld è stata rifiutata\n", i_fd);
  						//Settiamo il flag di Controllo
  						val=1;
  						//Inviamo il messaggio di rifiuto della connessione
  						sendRequest(fd_client, &risposta); //Proteggere con la mutex del conn
  						//Incrementiamo il numero di errori nelle Statistiche
  						chattyStats.nerrors++;

  					}
  					pthread_mutex_unlock(&mtx_stat);
  					if(val) continue;
  					else{
              //Altrimenti non ho ancora raggiunto il numero massimo di connessioni da poter accettare

              //Accettiamo la connessione
  						//printf("La connessione è stata accettata\n");
              // Aggiungiamo alla lista del set di ascolto
  						pthread_mutex_lock(&mtx_setfd);
  						FD_SET(fd_client, &setfd);
  						pthread_mutex_unlock(&mtx_setfd);

              if(fd_client > fd_num) fd_num=fd_client;
  					}
  				}
  				else{
            //Socket di I/O pronto - un client già connesso è pronto
  					printf("Ho ricevuto una nuova richiesta dal client con descrittore %d\n", fd_client);
  					int* new_fd=NULL;
  					handle_error_NULL((new_fd =malloc(sizeof(int))), "Errore nella malloc del nuovo descrittore");
  					*new_fd=i_fd;
  					//Devo togliere il descrittore dalla lista d'ascolto
  					pthread_mutex_lock(&mtx_setfd);
  					FD_CLR(i_fd, &setfd);
  					pthread_mutex_unlock(&mtx_setfd);
  					//Inserisco il file descriptor nella coda
  					handle_error_1((push(queue_descriptor, new_fd)), "Errore nella push nella coda");
  				}
        }
        else // !FD_ISSET(i_fd, &read_set)
          continue;
			}
		}
	}
  printf("\n \t-- Ho ricevuto un segnale di terminazione del server --\n");
  /* GESTISCO ESPLICITAMENTE COSA FARE ALL'ARRIVO DEL SEGNALE DI TERMINAZIONE
   * Dato che il server deve essere terminato, devo:
   * 1) Svuotare la coda se ha ancora elementi
   * 2) Inserire un descrittore =-1 nella coda per
   *            gestire la terminazione dei thread
   */
  //chiudo i file descriptor
  close(fd_client);
  //Scorro la maschera
  for (i_fd = 0; i_fd < fd_num+1; i_fd++){
   //Mi occupo della connessione full-double
   shutdown(i_fd, SHUT_RDWR);
   //SHUT_RDWR: disabiliamo le operazioni di invio o ricezioni
  }
  //Creo il descrittore
  long * terminatore;
  handle_error_NULL((terminatore = malloc(sizeof(long))),"Errore nella creazione del descrittore");
  //Setto il terminatore a -1
  *terminatore = -1;
  //poi la riempio con i segnali di terminazione
  //tanti quanti sono i thread piu' quanti messaggi sono in coda
  for(i=0;i<configuration.ThreadsInPool;i++){
    handle_error_1((push(queue_descriptor, terminatore)), "Errore nella push nella coda");
  }

  /*********** FASE DI CHIUSURA ***********/
	printf("\n \t *** FASE DI CHIUSURA DEL SERVER ***\n");

  printf(">> Passo 1: Aspettiamo che i thread del pool terminino\n");
  // Devo aspettare che i thread che terminino
  for (int i = 0; i < configuration.ThreadsInPool; i++) {
    //Devo chiamare la "pthread_join" per attendere che "t_pool[i]" termini
    //Ritorna immediatamente se "t_pool[i]" è già terminato
    pthread_join(t_pool[i], NULL);
  }
  //Libero il pool di thread
  printf(">> Passo 2: Liberiamo il pool di thread 't_pool' \n");
  free(t_pool);

  //Chiudo il socket
	printf(">> Passo 3: Chiudiamo il socket\n");
	unlink(configuration.UnixPath);

  //Elimino la coda dei descrittori
	printf(">> Passo 4: Eliminiamo la coda dei descrittori 'queue_descriptor' \n");
	deleteQueue(queue_descriptor);

  //Libero la Tabella degli utenti registrati
	printf(">> Passo 5: Eliminiamo la tabella degli utenti registrati 'hash_user'\n");
	handle_error_1(icl_hash_destroy(hash_user, free_hashtable_key, free_hashtable_data),
									"Errore nell'eliminazione della tabella utenti");

	//Libero la Tabella dei gruppi registrati
	printf(">> Passo 6: Eliminiamo la tabella dei gruppi registrati 'hash_group'\n");
	handle_error_1(icl_hash_destroy(hash_group, free_hashtable_key, free_hashtable_group_data),
									"Errore nell'eliminazione della tabella gruppi");
  //Liberiamo il messaggio
  printf(">> Passo 7: Liberiamo il messaggio di rifiuto\n");
  free(risposta.data.buf);
  risposta.data.buf=NULL;

  //Liberiamo il vettore di client connessi
  printf(">> Passo 8: Liberiamo il vettore di client connessi\n");
  free(vect_connected);

  //Eliminiamo le configurazioni allocate
  printf(">> Passo 9: Eliminiamo le configurazioni allocate\n");
  free(configuration.DirName);
  configuration.DirName = NULL;
  free(configuration.UnixPath);
  configuration.UnixPath = NULL;
  free(configuration.StatFileName);
  configuration.StatFileName = NULL;

  //Distruggo le variabili di mutuaesclusione usate
  printf(">> Passo 10: Distruggo le mutex utilizzate\n");
  handle_error_1(pthread_mutex_destroy(&(mtx_setfd)), "Errore nella distruzione della mutex 'mtx_setfd'");
  handle_error_1(pthread_mutex_destroy(&(mtx_stat)), "Errore nella distruzione della mutex 'mtx_stat'");
  handle_error_1(pthread_mutex_destroy(&(mtx_conn)), "Errore nella distruzione della mutex 'mtx_conn'");

  printf("\n \t *** CHIUSURA TERMINATA ***\n");
	fflush(stdout);
	fflush(stdin);
  return 0;
}

/**************Implementazioni degli handlers**********/
/*
 *  @function gestore_segnali
 *  @brief La procedura rappresenta un gestore che salva il segnale arrivato
 *  @param segnale rappresenta il segnale da salvare, tra i seguenti:
 *        - SIGINT
 *        - SIGQUIT
 *        - SIGTERM
 *        - SIGUSR1
 *  @discussion la gestione vera e propria del segnale
 *            verrà fatta esplicitamente  all'interno del programma
 */
static void gestore_segnali(int segnale) {
  //Salvo il segnale in modo signal safe
  sigalarm_flag = segnale;
  //Scrivo in modo signal safe
  write(1, "\n \t-- Ho catturato un nuovo segnale --\n", 39);
}

/*
*  @function handle_signals
*  @brief La procedura si occupa di registrare i segnali
*/
static void handle_signals(){
  //Inizializzo della variabile di gestione dei segnali
  sigalarm_flag=0;
  //Struttura usata per personalizzare la gestione dei Segnali
  struct sigaction sg;
  //Azzero il contenuto della struttura sigaction
  memset(&sg, 0, sizeof(sg));
  //Installazione del gestore dei segnali
  sg.sa_handler=gestore_segnali;
  //Registro i segnali per la terminazione del server
  handle_error_1(sigaction(SIGINT, &sg, NULL), "Errore sigaction: SIGINT");
  handle_error_1(sigaction(SIGQUIT, &sg, NULL), "Errore sigaction: SIGQUIT");
  handle_error_1(sigaction(SIGTERM, &sg, NULL), "Errore sigaction: SIGTERM");
  // Controllo se nel file di configurazione è stato specificato StatFileName
  if (configuration.StatFileName != NULL && strlen(configuration.StatFileName) > 0) {
    // Registro il segnale SIGUSR1
    handle_error_1(sigaction(SIGUSR1, &sg, NULL), "Errore sigaction: SIGUSR1");
  }

  //Installazione del gestore per ignorare i Segnali
  sg.sa_handler = SIG_IGN;
  //Registro i segnali da ignorare- SIGPIPE
  handle_error_1(sigaction(SIGPIPE, &sg, NULL), "Errore sigaction: SIGPIPE");

}
/**************Implementazioni delle funzioni utilizzate**********/
/*
 * @function worker_thread
 * @brief funzione eseguita dai thread del pool
 * @discussion Usando strutture globali non ho bisogno di argomenti per la funzione
 * @return NULL
 */
void* worker_thread(void* arg){

	//Conterrà il messaggio preso dal descrittore
  message_t msg;
	//Azzero i campi del messaggio
  memset(&msg, 0, sizeof(message_t));

	//Itero finché non viene richiesta la terminazione
  while(!((sigalarm_flag==SIGINT) || (sigalarm_flag==SIGQUIT) || (sigalarm_flag==SIGTERM))){
    //Numero di bytes letti
    int n_bytes;
    //File descriptor da cui leggere. Verrà estratto dalla coda
  	long des = (long) pop(queue_descriptor);
 /////  CODICE USATO SE: in fase di chiusura inserisco solo un descrittore a -1
 /////  //Copio il file descriptor per dereferenziare il puntatore
 /////   long control=*des; => testo control nell'if
    /*
     * Controllo se il descrittore =-1
     * In quel caso, vuol dire che il Server è stato terminato ed abbiamo inserito
     * un descrittore uguale a "-1" per indicare ai thread di terminare
     */
    if (des == -1){

  /////  CODICE USATO SE: in fase di chiusura inserisco solo un descrittore a -1
  /////    //Reinserisco in coda il descrittore per far terminare anche gli altri thread
  /////    //Inserisco il file descriptor nella coda
  /////    handle_error_1((push(queue_descriptor, des)), "Errore nella push nella coda");
      //Termino il thread this
      return NULL;
    }
  /////  CODICE USATO SE: in fase di chiusura inserisco solo un descrittore a -1
  /////  //Libero il puntatore
  /////  free(des);
		n_bytes = readMsg(control, &msg);
    /*
     * controllo se n_bytes <0 è un errore dovuto alla disconnessione di un client
     * cioè se errno è settato in un particolar modo:
     * errno = EBADF          => Bad file descriptor
     * errno = ECONNABORTED   => La connessione è stata abortita
     * errno = ECONNREFUSED   => La connessione è stata rifiutata
     * errno = ECONNRESET     => Reset della connessione
     * errno = EPIPE          => Broken pipe
     */
    if(n_bytes <0 && (errno == EBADF        \
                    ||errno == ECONNABORTED \
                    ||errno == ECONNREFUSED \
                    ||errno == ECONNRESET   \
                    ||errno == EPIPE )){
			//Stampo l'errore usando strerror:
			fprintf(stderr, "Abbiamo ricevuto l'errore '%s'", strerror(errno));
			int ind=0; //indice del client

			//Cerco il client da disconnettere
			pthread_mutex_lock(&mtx_conn);
			//---------> ind = findClient(control, vect_connected, configuration.MaxConnections);
			pthread_mutex_unlock(&mtx_conn);
			//Controllo se ho trovato un client corrispondente
			if(ind!=-1){
				//Disconnetto il client
				pthread_mutex_lock(&mtx_conn);
				//logOut(des, NULL, ind); //??????????????????????????
				/*
						TO IMPLEMENT => DISCONNETTERE IL CLIENT

					*/
				pthread_mutex_unlock(&mtx_conn);
				pthread_mutex_lock(&mtx_stat);
				//Aggiorno il numero di errori nelle statistiche
				chattyStats.nerrors++;
				//Aggiorno il numero di client connessi
				if(chattyStats.nonline>0) chattyStats.nonline--;
				pthread_mutex_unlock(&mtx_stat);
			}
			continue;
    }	//Altrimenti se n_bytes < 0 allora ho un errore (errno settato)
		else handle_error_min(n_bytes, "Errore nella lettura del messaggio");
		if(n_bytes==0){
	 		//Connessione chiusa da parte del client
			pthread_mutex_lock(&mtx_conn);
//				disconnect_user(des, data, NULL); //Probabilmente serve solo control, poiché data non lo usiamo
/*
		TO IMPLEMENT => DISCONNETTERE IL CLIENT

	*/
			pthread_mutex_unlock(&mtx_conn);
      //Aggiorno le statistiche
  		pthread_mutex_lock(&mtx_stat);
			//Aggiorno il numero di errori nelle statistiche
			chattyStats.nerrors++;
			//Aggiorno il numero di client connessi
			if(chattyStats.nonline>0) chattyStats.nonline--;
			pthread_mutex_unlock(&mtx_stat);
		}//Altrimenti, ho letto n_bytes>0
		else{
			//Controllo se l'operazione richiesta è valida:
			//cioè se è un'operazione che il server può svolgere per il client
			if(msg.hdr.op>DELGROUP_OP){
				/*
				 * vuol dire che stiamo richiedendo un operazione con indice>12
				 * tuttavia le operazioni con indice>12 sono messaggi di ritorno
				 * che il server può mandare al client
				 */
				 fprintf(stdout, "Abbiamo ricevuto un operazione non valida dal client %ld", control);
				 pthread_mutex_lock(&mtx_conn);
				 /**
					 TO IMPLEMENT send_error_message(control, OP_FAIL, data, NULL, "L'operazione richiesta non è valida", NULL);
				 */
				 pthread_mutex_unlock(&mtx_conn);
				 //Aggiorno il numero di errori nelle statistiche
				 pthread_mutex_lock(&mtx_stat);
				 chattyStats.nerrors++;
				 pthread_mutex_unlock(&mtx_stat);
			}
			else{
				//Controllo il nickname del mittente
				if(msg.hdr.sender[0]=='\0'){
					//Ignoriamo la richiesta di messaggio
					fprintf(stdout, "Abbiamo ricevuto dal client %ld un messaggio che abbiamo ignorato", des);
          //Aggiorno il numero di errori nelle statistiche
					pthread_mutex_lock(&mtx_stat);
					chattyStats.nerrors++;
					pthread_mutex_unlock(&mtx_stat);
				}
				else{
					//Adesso posso eseguire l'operazione dell'utente
					// Eseguo l'operazione richiesta
					/***
						TO IMPLEMENT switch_op(msg.hdr.op, msg, control)
						chatty.c rem 287
						chatty_handlers.c bert 1259
            chatty.c bert 243

					**/
					pthread_mutex_lock(&mtx_setfd);
					//Rimetto nell'insieme dei descrittori se il client non si è disconnesso durante l'operazione
					//NOTA: Come controllo se non si è disconnesso??
					FD_SET(control, &setfd);
				 	pthread_mutex_unlock(&mtx_setfd);
				}
			}
		}
		free(msg.data.buf);
  }
	return NULL;
}

/*
 * @function execute
 * @brief La procedura si occupa di gestire le richieste
 *        inviate al server dai client, scegliendo che funzione eseguire
 * @param msg         messaggio da elaborare
 * @param client_fd   file descriptor del client che invia la richiesta
 * @param connesso    ci dice se il client si disconnette durante l'operazione (connesso=0)
 * @discussion    Il chiamante deve aver fatto tutti i controlli del caso
 *                prima di chiamare questa procedura
 */
void execute(message_t msg, long client_fd, int *connesso){
  //Prendo l'operazione dal messaggio
  op_t operation = msg.hdr.op;
  //Creo il messaggio di risposta
  message_t ret;
  //Uso delle funzioni ausiliarie in base all'operazione richiesta
  switch(operation){
    case REGISTER_OP:
      //Si richiede la registrazione di un nuovo utente (nickname)
      registerUser(msg.hdr.sender, client_fd);
      break;
    case CONNECT_OP:
      //Si richiede la connessione di un utente già registrato
      connectUser(msg.hdr.sender, client_fd);
      break;
    case POSTTXT_OP:
      //Si richiede l'invio di un messaggio di testo
      postTxt(&msg, client_fd, connesso);
      break;
    case POSTTXTALL_OP:
      postTxtToAll(msg, client_fd);
      break;
    case POSTFILE_OP:
      postFile(msg, client_fd);
      break;
    case GETFILE_OP:
      getFile(msg, client_fd);
      break;
    case GETPREVMSGS_OP:
      getPrevMsg(msg.hdr.sender, client_fd);
      break;
    case USRLIST_OP:
      usersList(msg.hdr.sender, client_fd);
      break;
    case UNREGISTER_OP:
      unregisterUser(msg.hdr.sender, client_fd);
      break;
    case DISCONNECT_OP:
      disconnectUser(client_fd);
      break;
    default:{
            printf("\n \t REPLY: Operazione sconosciuta\n");
            //Invio il messaggio di errore
            setHeader(&(ret.hdr), OP_FAIL, "Operazione Sconosciuta");
            sendHeader(client_fd, &(ret.hdr));
    }
  }
}
/*
 * @function free_user
 * @btrief La procedura libera la memoria occupata da un utente
 * @param ptr Puntatore alla struttura che bisogna deallocare
 */
void free_user(void *ptr){
	  if(ptr != NULL){
			register_user* u= (register_user*) ptr;
			//Devo liberare la lista dei messaggi di u
			int n =deleteBuff(u->history);
      assert(n>0);
			//Dopo aver deallocato tutti i messaggi, liberiamo il buffer
			free(u->history);
			//Liberiamo l'utente u
			free(u);
			ptr=NULL;
		}
		else{
			//Se l'utente è già NULL esco
			return;
		}
}
/*
 * @function findClient
 * @brief La funzione restituisce l'indice del client associato al descrittore "descriptor"
 * @param descriptor Il descrittore da cercare
 * @return NULL se non troviamo un client connesso relativo a "descriptor"
 * @discussion il chiamante deve aver richiesto la mutuaesclusione prima di invocare la funzione
 */
static int findClient(long descriptor){
  int j;
  //Scansioniamo il vettore di client connessi
  for(j = 0; j < configuration->MaxConnections; j++) {
    conn_user *cu = vect_connected[j];
    if(cu->descriptor == descriptor) {
      //Ho trovato l'indice del client connesso corrispondente
      return j;
    }
  }
  //Non ho trovato nessuna corrispondenza
  return -1;
}
/*
 * @function free_msg
 * @brief La procedura libera la memoria occupata da un messaggio
 * @param msg Puntatore alla struttura che bisogna deallocare
 */
void free_msg(void *ptr) {
	if(ptr != NULL){
		message_t*msg=(message_t*) ptr;
		//Libero il buffer allocato
		free(msg->data.buf);
		//Dopo aver deallocato il buffer, libero il messaggio
		free(msg);
		ptr=NULL;
	}
	else{
		//Se l'utente è già NULL esco
		return;
	}
}
/*
 * @funciton registerUser
 * @brief Registra un nuovo utente se non è già presente
 * @param nickname    nickname da registrare
 * @param descriptor  descrittore della connessione
 *
 */
void registerUser(char* nickname, long descriptor){

  //Creo la chiave da inserire nell'hashtable
  char* key;
  handle_error_NULL(key=calloc((MAX_NAME_LENGTH+1), sizeof(char)), "Errore nella creazione della chiave");
  strncpy(key, nick, MAX_NAME_LENGTH+1);
	//Creo il messaggio di risposta da inviare al client che ha richiesto l'operazione
  message_hdr_t risposta_hdr;

	if(icl_hash_find(hash_user, key)==NULL){
    //Allora il nickname non è già registrato => possiamo registrarlo
		//Creo la struttura utente
		register_user* new_utente=NULL;
		handle_error_NULL(new_utente = (user*)malloc(sizeof(user)), "Errore nella creazione dell'utente");
		//Inizializzo i campi della struttura utente
		new_utente->descriptor=-1;
		new_utente->history=initBuff(configuration->MaxHistMsgs);
		//Inserisco l'utente nella tabella hash
    icl_entry_t *ins = icl_hash_insert(hash_user, key, (void *)new_utente);
    if (ins != NULL){
			//L'utente è stato registrato con successo
			//Aggiorno le statistiche
			pthread_mutex_lock(&mtx_stat);
			//Aggiorno il numero di utenti registrati
			chattyStats.nusers++;
			pthread_mutex_unlock(&mtx_stat);
			//Connetto l'utente appena registrato
			connectUser(key, descriptor);
		}
		else{
			/*L'utente non è stato registrato con successo, quindi:
			 * 1) setto l'header del messaggio di risposta
			 * 2) invio l'header del messaggio di risposta
			 * 3) Aggiorno le statistiche
			 * 4) Libero la risorsa allocata
			 */
			setHeader(&risposta_hdr, OP_FAIL, "Server");
			sendHeader(descriptor, &risposta_hdr);
			pthread_mutex_lock(&mtx_stat);
			//Aggiorno il numero di errori nelle statistiche
			chattyStats.nerrors++;
			pthread_mutex_unlock(&mtx_stat);
			//Libero la risorsa allocata
			free_user(new_utente);
		}
  }
	else{
		/* L'utente è già registrato, quindi:
		 * 1) setto l'header del messaggio di risposta
		 * 2) invio l'header del messaggio di risposta
		 * 3) Aggiorno le statistiche
		 * 4) Libero la risorsa allocata
		 */
		setHeader(&risposta_hdr, OP_NICK_ALREADY, "Server");
		sendHeader(descriptor, &risposta_hdr);
		pthread_mutex_lock(&mtx_stat);
		//Aggiorno il numero di errori nelle statistiche
		chattyStats.nerrors++;
		pthread_mutex_unlock(&mtx_stat);
		//Libero la risorsa allocata
		free(key);
	}

}
/*
 * @function connectUser
 * @brief  connette l'utente con nickname "nickname"
 * @param nickname	   nickname dell'utente da connettere
 * @param descriptor      descrittore della connessione
 */
void connectUser(char* nickname, long descriptor){
  //Creo il messaggio di risposta da inviare al client che ha richiesto l'operazione
  message_hdr_t risposta_hdr;
  register_user *us=icl_hash_find(hash_user, nickname);
  //Controlliamo che l'utente sia effettivamente già registrato
  if(us==NULL){
    //L'utente non è già registrato
    setHeader(&risposta_hdr, OP_NICK_UNKNOWN, "Server");
    sendHeader(descriptor, &risposta_hdr);
    pthread_mutex_lock(&mtx_stat);
    //Aggiorno il numero di errori nelle statistiche
    chattyStats.nerrors++;
    pthread_mutex_unlock(&mtx_stat);
  }
  else{
    pthread_mutex_lock(&mtx_conn);
    us->fd=descriptor;
    //Devo inserire nel vettore di connessi alla prima posizione libera
    int j;
    for(j = 0; j < configuration->MaxConnections; j++) {
      conn_user* cu= vect_connected[i];
      //Controllo se la posizione è libera o meno
      if(cu->fd<0){
        //Se non era connesso lo connetto inserendo nome e fd
        cu->fd=descriptor;
        memset(cu->nickname, 0, MAX_NAME_LENGTH+1);
        strncpy(cu->nickname, nickname, MAX_NAME_LENGTH);
        pthread_mutex_lock(&mtx_stat);
        //Aggiorno il numero di utenti online
        chattyStats.nonline++;
        pthread_mutex_unlock(&mtx_stat);
        pthread_mutex_unlock(&mtx_conn);
        return;
      }
    }
    pthread_mutex_unlock(&mtx_conn);
  }
}
/*
 * @function postTxt
 * @brief  Invia un messaggio testuale ad un altro utente
 * @param msg         il messaggio da inviare
 * @param descriptor  descrittore della connessione del mittente
 * @param connesso    dice se il client si disconnette durante l'operazione
 */
void postTxt(message_t* msg, long descriptor, int* connesso){
  //Creo il messaggio di risposta da inviare al client che ha richiesto l'operazione
  message_t ack;
  //Cerco in mutuaesclusione l'indice del client corrispondete a "descriptor"
  int index;
  pthread_mutex_lock(&mtx_conn);
  //Invoco la funzione findClient
  index=findClient(descriptor);       //Da controllare con  find_connected_client(fd, pl); riga 548
  pthread_mutex_unlock(&mtx_conn);

  //Controllo se l'indice di connessione è valido o meno
  if(index==-1){
    /* L'indice di connessione non è valido:
     * Un client non connesso ha cercato di mandare il messaggio
     */
    //Richiedo la m.e
    pthread_mutex_lock(&mtx_conn);
    *connesso = dispatchError(OP_NOT_CONNECT, NULL, descriptor, "Il client non è connesso", NULL);
    //Rilascio la m.e
    pthread_mutex_unlock(&mtx_conn);
    return;
  }
  //Ho trovato una corrispondenza tra i client connessi
  //Controllo la dimensione del messaggio
  if(msg->data.hdr.len > configuration->MaxMsgSize){
    //Il messaggio non rientra nella dimensione prestabilia
    //Richiedo la m.e
    pthread_mutex_lock(&mtx_conn);
    *connesso = dispatchError(OP_MSG_TOOLONG, NULL, descriptor, "Testo troppo lungo", NULL);
    //Rilascio la m.e
    pthread_mutex_unlock(&mtx_conn);
    return;
  }
  //Accedo in m.e.
  pthread_mutex_lock(&mtx_conn);
  //Se è permesso tento di inviare prima il messaggio ad un gruppo
  list* gr =NULL;
  if((gr=(list*) icl_hash_find(hash_group, msg->data.hdr.receiver))!=NULL){
    //Se esiste un gruppo corrispondete, invio il messaggio a tutta la lista di utenti
    dispatchGroup(msg->data.hdr.receiver, gr, msg->hdr.sender, descriptor, connesso);
  }
  else{
    //Se non esiste un gruppo corrispondente, tento di inviare il messaggio ad un utente registrato
    register_user* ru=NULL;
    if((ru= (register_user) icl_hash_find(hash_user, msg->data.hdr.receiver))!=NULL){
      //Se esiste un utente corrispondente, invio il messaggio a quell'utente
      dispatchUser


      DA COMPLETARE GUARDA B 582
            DA COMPLETARE GUARDA B 582
                  DA COMPLETARE GUARDA B 582
                        DA COMPLETARE GUARDA B 582
                              DA COMPLETARE GUARDA B 582
                                    DA COMPLETARE GUARDA B 582
                                          DA COMPLETARE GUARDA B 582
                                                DA COMPLETARE GUARDA B 582
                                                      DA COMPLETARE GUARDA B 582
                                                            DA COMPLETARE GUARDA B 582
                                                                  DA COMPLETARE GUARDA B 582
                                                                        DA COMPLETARE GUARDA B 582
                                                                              DA COMPLETARE GUARDA B 582
                                                                                    DA COMPLETARE GUARDA B 582
                                                                                          DA COMPLETARE GUARDA B 582
                                                                                                DA COMPLETARE GUARDA B 582
                                                                                                      DA COMPLETARE GUARDA B 582
                                                                                                      
    }
  }

  pthread_mutex_unlock(&mtx_conn);
}

/*
 * @function dispatchError
 * @brief La funzione invia un messaggio di errore con i dovuti controlli di connessione
 * @param op          operazione di errore
 * @param cl      il client da disconnettere (se disponibile)
 * @param descriptor  file descriptor del client
 * @param txt         testo del messaggio
 * @param dest        destinatario del messaggio
 * @return =0 in caso di disconnessione
 * @discussion Il chiamante deve aver richiesto la m.e. su "mtx_conn"
 */
int dispatchError(op_t op, conn_user cl, long descriptor, const char* txt, const char*dest){
  // ==========> INIZIO "make_error_message"
  //Inizializzo il messaggio di errore
  errorMsg(&ack, OP_NOT_CONNECT, txt, NULL);
  // ==========> FINE "make_error_message"

  // ==========> INIZIO "send_handle_disconnect"
  //Invia il body del messaggio
  int ret_val=  sendRequest(descriptor, ack);
  //Controllo eventuali errori
  /*
   * Controllo se ret_val ==0 oppure se è <0
   * ed è un errore dovuto alla disconnessione di un client
   * cioè se errno è settato in un particolar modo:
   * errno = EBADF          => Bad file descriptor
   * errno = ECONNABORTED   => La connessione è stata abortita
   * errno = ECONNREFUSED   => La connessione è stata rifiutata
   * errno = ECONNRESET     => Reset della connessione
   * errno = EPIPE          => Broken pipe
   * oppure se è ==0
   */
  if(ret_val ==0 ||
                  (ret_val <0 && (errno == EBADF        \
                                 ||errno == ECONNABORTED \
                                 ||errno == ECONNREFUSED \
                                 ||errno == ECONNRESET   \
                                 ||errno == EPIPE ))){
    //Disconnetto il client
    disconnectUser(descriptor, NULL);
    //Setto ret_val=0 nel caso in cui era <0
    if(ret_val<0) ret_val=0;
  }
  // ==========> FINE "send_handle_disconnect"
  handle_error_min(ret_val, "Errore: stiamo notificando un errore");
  //Aggiorno le statistiche in m.e.
  pthread_mutex_lock(&mtx_stat);
  //Aggiorno il numero di errori nelle statistiche
  chattyStats.nerrors++;
  pthread_mutex_unlock(&mtx_stat);

  //Libero il buffer del messaggio se non è vuoto
  if(ack.data.buf) free(ack.data.buf);
  return ret_val;
}

/*
 * @function dispatchGroup
 * @brief La funzione invia un messaggio ad un gruppo
 * @param nick    Il nome del gruppo verso cui instradare il messaggio
 * @param gr      Il puntatore alla lista di utenti del gruppo
 * @param client  Il nome di chi invia il messaggio
 * @param descriptor File descriptor del client
 * @param connesso  Dice se il client si è disconnesso durante l'operazione
 * @discussion Il chiamante deve aver controllato gr!=NULL
 */
void dispatchGroup(const char* key, list* gr, const char* client, long descriptor, int* connesso){
  //Dichiaro il valore di ritorno
  int ret_val;
  int j=0;
  //Dichiaro la variabile usata per verificare se l'utente fa parte del gruppo
  int not_in=1;
  //Dichiaro un array di stringhe dove salvare gli utenti in lista
  char** utenti;
  int n_utenti;
  //Mi salvo il numero di utenti in lista
  handle_error_min(n_utenti=listToArray(list, &utenti), "Errore nella listToArray");
  /*** CONTROLLO CHE L'UTENTE FACCIA EFFETTIVAMENTE PARTE DEL GRUPPO ***/
  while(j<n_utenti && not_in){
    //Controllo se i nickname coincidono
    if(strncmp(utenti[j], client, MAX_NAME_LENGTH) ==0){
      //Sono uguali quindi l'utente è nel gruppo
      not_in=0;
    }
    j++;
  }
  if(not_in){
    //Il mittente non è nel gruppo
    for(j = 0; j<n_utenti; j++) {
      //Libero l'array di stringhe
      free(utenti[j]);
    }
    ret_val=dispatchError(OP_NICK_UNKNOWN, NULL, descriptor, "Il client non è presente nel gruppo", client);
    handle_error_min(ret_val, "Errore: dispatchError");
    *connesso=ret_val;
  }
  if(!not_in) {
    //Se mittente è nel gruppo
    for(j = 0; j < n_utenti; j++) {
      //Perogni elemento nella tabella hash uguale ad utenti[j], applico dispatchUser
      //controllo che il valore di ritorno sia >0
/*  ======> TO IMPLEMENT

         ret = chash_get(pkt->pl->registered_clients, users[i], route_message_to_client, pkt);
      HANDLE_FATAL(ret, "chash_get");
*/
      free(utenti[j]);
    }
    //Mando l'header del messaggio di operazione andata a buon fine
    ret_val=dispatchHeader(OP_OK, NULL, descriptor);
    handle_error_min(ret_val, "Errore: dispatchHeader");
    //Aggiorno il flag di connessione
    *connesso = ret_val;
  }
  free(utenti);
  //Messaggio inviato con successo
}

/*
 * @function dispatchHeader
 * @brief La funzione  invia l'header di un messaggio al descrittore
 * @param op          operazione da inserire
 * @param cl          il client da disconnettere (se disponibile)
 * @param descriptor  file descriptor del client
 * @return 0 se il client si è disconnesso,
          -1 in caso di errore
 * @discussion Il chiamante deve aver richiesto la m.e. su "mtx_conn"
 */
int dispatchHeader(op_t op, conn_user cl, long descriptor){
  //Creo l'header del messaggio
  message_hdr_t msg;
  memset(&msg, 0, sizeof(message_hdr_t));
  //Dico che è andato tutto a buon fine
  msg.op = op;
  //Mando il messaggio
  int ret_val=sendHeader(descriptor, &msg);
  //Controllo eventuali errori
  /*
   * Controllo se ret_val ==0 oppure se è <0
   * ed è un errore dovuto alla disconnessione di un client
   * cioè se errno è settato in un particolar modo:
   * errno = EBADF          => Bad file descriptor
   * errno = ECONNABORTED   => La connessione è stata abortita
   * errno = ECONNREFUSED   => La connessione è stata rifiutata
   * errno = ECONNRESET     => Reset della connessione
   * errno = EPIPE          => Broken pipe
   * oppure se è ==0
   */
  if(ret_val ==0 ||
                  (ret_val <0 && (errno == EBADF        \
                                 ||errno == ECONNABORTED \
                                 ||errno == ECONNREFUSED \
                                 ||errno == ECONNRESET   \
                                 ||errno == EPIPE ))){
    //Disconnetto il client
    disconnectUser(descriptor, cl);
    //Setto ret_val=0 nel caso in cui era <0
    if(ret_val<0) ret_val=0;
  }
  return ret_val;
}


/**
 * @function unregisterUser
 * @brief  deregistra l'user
 *
 * @param nick     nickname da deregistrare
 * @param fd       descrittore della connessione
 *
 */
void unregisterUser(char* nick, long fd);



/**
 * @function sendTextToAll
 * @brief  invia un messaggio a tutti gli user diversi dal mittente
 *
 * @param msg      il messaggio da inviare a tutti
 * @param fd       descrittore della connessione del mittente
 *
 */
void sendTextToAll(message_t msg, long fd);

/**
 * @function sendFile
 * @brief invia il file ad un utente diverso dal mittente
 *
 * @param msg      messaggio con le informazioni relative alla richiesta
 * @param fd       descrittore della connessione del mittente
 *
 */
void sendFile(message_t msg, long fd);

/**
 * @function getFile
 * @brief richiede un file al server
 *
 * @param msg      messaggio con le informazioni relative alla richiesta
 * @param fd       descrittore della connessione del mittente
 *
 */
void getFile(message_t msg, long fd);

/**
 * @function listHistory
 * @brief  richiede la lista degli ultimi messaggi inviati ad un user
 *
 * @param nick     nickname dell'user che richiede la lista
 * @param fd       descrittore della connessione
 *
 */
void listHistory(char* nick, long fd);

/**
 * @function listOnlineUsers
 * @brief richiede la lista degli utenti connessi
 *
 * @param nick     nickname dell'user che richiede la lista
 * @param fd       descrittore della connessione
 *
 */
void listOnlineUsers(char* nick, long fd);

/*
 * @function disconnectUser
 * @brief  disconnette l'user
 *
 * @param fd       descrittore della connessione
 *
 */
void disconnectUser(long fd);
