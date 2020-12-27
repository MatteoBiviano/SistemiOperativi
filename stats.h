/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/*
 * @file  stats.h
 * @brief Contiene la struttura e le funzioni per gestire le statistiche in mutuaesclusione
 * Si dichiara il contenuto del file "stats.h" è opera originale
 * dello studente Matteo Biviano (matr. 543933)
 */

#if !defined(MEMBOX_STATS_)
#define MEMBOX_STATS_

#include <stdio.h>
#include <time.h>

struct statistics {
    unsigned long nusers;                       // n. di utenti registrati
    unsigned long nonline;                      // n. di utenti connessi
    unsigned long ndelivered;                   // n. di messaggi testuali consegnati
    unsigned long nnotdelivered;                // n. di messaggi testuali non ancora consegnati
    unsigned long nfiledelivered;               // n. di file consegnati
    unsigned long nfilenotdelivered;            // n. di file non ancora consegnati
    unsigned long nerrors;                      // n. di messaggi di errore
};

/* aggiungere qui altre funzioni di utilita' per le statistiche */


/*
 * @function printStats
 * @brief Stampa le statistiche nel file passato come argomento
 *
 * @param fout descrittore del file aperto in append.
 * NOTA: il file va aperto prima di chiamare la funzione e chiuso dopo
 * @return 0 in caso di successo, -1 in caso di fallimento
 */
static inline int printStats(FILE *fout) {
  extern struct statistics chattyStats;
  if (fprintf(fout, "%ld - %ld %ld %ld %ld %ld %ld %ld\n",
      (unsigned long)time(NULL),
      chattyStats.nusers,
      chattyStats.nonline,
      chattyStats.ndelivered,
      chattyStats.nnotdelivered,
      chattyStats.nfiledelivered,
      chattyStats.nfilenotdelivered,
      chattyStats.nerrors) < 0){
      return -1;
  }
  fflush(fout);
  return 0;
}
/*
 * @function updateStats
 * @brief Aggiorna le statistiche
 * @param nusers              numero di nuovi utenti registrati
 * @param nonline             numero di nuovi utenti connessi
 * @param ndelivered          numero di nuovi messaggi consegnati
 * @param nnotdelivered       numero di nuovi messaggi non ancora consegnati
 * @param nfiledelivered      numero di nuovi file consegnati
 * @param nfilenotdelivered   numero di nuovi file non ancora consegnati
 * @param nerrors             numero di nuovi messaggi di errore
 * @discussion La mutuaesclusione deve essere gestita esternamente
 */
static inline void updateStats(int nusers, int nonline, int ndelivered,
                                int nnotdelivered, int nfiledelivered,
                                    int nfilenotdelivered, int nerrors){
    //Dobbiamo aggiornare la struttura presente in chatty.c
    extern struct statistics chattyStats;
    chattyStats.nusers += nusers;
    chattyStats.nonline += nonline;
    chattyStats.ndelivered += ndelivered;
    chattyStats.nnotdelivered += nnotdelivered;
    chattyStats.nfiledelivered += nfiledelivered;
    chattyStats.nfilenotdelivered += nfilenotdelivered;
    chattyStats.nerrors += nerrors;
}

#endif /* MEMBOX_STATS_ */
