# SistemiOperativi
Il progetto prevede lo sviluppo di un server concorrente che implementa 
una chat: chatterbox. Il nome dato al server e' chatty.
  
------------------------------------------------  
Estrarre il materiale dal KIT  
------------------------------------------------  
Creare una directory temporanea, copiare kit_chatty.tar    
nella directory e spostarsi nella directory appena creata. 

Es.  
$$ mkdir Progetto
$$ tar xvf kit_chatty18.tar -C Progetto  
  
questo comando crea nella directory Progetto una directory "chatterbox" con i seguenti file:

chatterbox18.pdf       : specifiche del progetto

client.c	       : client fornito dai docenti che deve essere utilizzato
		       	 per effettuare i tests
			 (NON MODIFICARE)

connections.h
message.h
config.h
ops.h
stats.h                
chatty.c	       : files di riferimento forniti dai docenti.
		         Tale file vanno modificati dallo studente come meglio ritiene.

testconf.sh
testfile.sh
testleaks.sh
teststress.sh
Makefile  
		       : programmi di test e Makefile. Nessuno dei file di test va modificato.
                         Il Makefile va modificato solo nelle parti indicate.
  
DATI/*			: file di dati necessari per i tests
                          (NON MODIFICARE)  
  
README.txt		: questo file  
README.doxygen		: informazioni sul formato doxygen dei commenti alle funzioni 
			  (FACOLTATIVO)
  
------------------  
Come procedere :  
-----------------  
  
1) Leggere attentamente le specifiche. Capire bene il funzionamento del client e di tutti gli 
   script di test forniti dai docenti. Porre particolare attenzione al formato del messaggio
   (vedere file message.h)   
  
2) Implementare le funzioni richieste ed effettuare testing preliminare utilizzando 
   semplici programmi sviluppati allo scopo.
  
3) Testare il software sviluppato con i test forniti dai docenti. Si consiglia di effettuare 
   questa fase solo dopo aver eseguito un buon numero di test preliminari (punto 2)).
  
       bash:~$ make test1  
       bash:~$ make test2  
       bash:~$ make test3
       bash:~$ make test4
       bash:~$ make test5
       bash:~$ make test6
  
  
4) preparare la documentazione: ovvero commentare adeguatamente il/i file che  
   contengono la soluzione  ed inserire un'intestazione contenente il nome  
   dello sviluppatore ed una dichiarazione di originalita'   
  
   /** \file pippo.c  
       \author Giuseppe Garibaldi 456789 
       Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
       originale dell'autore  
     */  
  
  
5) Inserire nel Makefile il nome dei file da consegnare 
   (variabile FILE_DA_CONSEGNARE) il nome del tarball da produrre
   (variabile TARNAME) ed il corso di appartenenza (variabile CORSO).
   Compilare anche il campo MAIL.
  
8) eseguire il test 
  
      bash:~$ make consegna  
  
   e seguire le istruzioni del sito del progetto per la consegna.
