/** @file
 *  @brief Calcolatrice in IPC, con comunicazione tra padre e figlio.
 *  
 *  Si vuole realizzare un programma in C che utilizzi le system call (IPC), ove possibile, per implementare lo scheletro di un simulatore di calcolo parallelo. Il progetto deve essere commentato in formato Doxygen, e corredato da un file di configurazione per Doxygen, e da uno script Makefile per la compilazione. 
 *  Inoltre si devono allegare al progetto anche eventuali file di supporto.
 *	Il programma dovrà leggere un file di configurazione, contenente:
 *	1. Il numero di processi di calcolo parallelo.
 *	2. I dati di computazione.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/time.h>
#include "mylib.h"

#define DATA "data.txt" /**< @brief File di configurazione. */
#define RESULT "results.txt" /**< @brief File di output dei risultati. */
#define STDIN 0 /**< @brief Standard di Input. */
#define STDOUT 1 /**< @brief Standard di Output. */
#define BUFLEN 4096 /**< @brief Grandezza del buffer per la lettura del file. */


/**
 * @brief      Struttura dati per l'invio delle operazioni.
 */
struct Operation
{
	char op; /**< @brief Operando o segnale di interruzione K. */
	double num1; /**< @brief Primo operatore. */
	double num2; /**< @brief Secondo operatore. */
	double res; /**< @brief Risultato. */
};


/**
 * @brief      Funzione principale del programma.
 *
 * @param      data.txt File di configurazione.
 * @return     results.txt File di output di risultati.
 */
int main (void){

	// DICHIARAZIONE VARIABILI LOCALI
	int nProc; 	//Numero di processi.
	int nOp=0; 	//Numero di Operazioni.
	int semid;	//ID Identificativo dei semafori.
	int prid; 	//Identificativo del processo a cui inviare l'operazione.
	
	key_t key;	//Dichiarazione chiave univoca.
	char buffer[BUFLEN];	//Buffer di lettura dal file.
	char *token;	//Utilizzata per la lettura riga per riga del file di configurazione.
	char text[BUFLEN];	//Array utilizzato per la sprintf e la write.
	struct sembuf p1,v1,p2,v2;	//Dichiarazione delle strutture di tipo sembuf utilizzate nei semafori.
	//FINE DICHIARAZIONE DELLE VARIABILI LOCALI
	
	// INIZIALIZZAZIONE DELL'OPERAZIONE CHE DEVONO EFFETTUARE I SEMAFORI.
	p1.sem_op=-1;
	v1.sem_op=1;
	p2.sem_op=-1;
	v2.sem_op=1;


	int isOpen = open(DATA,O_RDONLY,S_IRUSR); //Apertura file di configurazione.
	//Apro e leggo il file di configurazione, in caso di errore termino il programma.
	if(isOpen == -1){
		syserr(DATA,"open failure");
		return -1;
	}

	int x=read(isOpen,buffer,BUFLEN); //Lettura file.

	if(x<0){
		syserr(DATA,"read failure");
		return -1;
	}

	//Leggo il numero di operazioni da eseguire.
	for(int i=0;i<(int)(strlen(buffer));i++){
		if(buffer[i]=='\n'){
			nOp++;
		}
	}
			
	//Ricavo il numero di processi che devo creare.
	token = strtok(buffer,"\n");
	if(token != NULL){
		nProc=atoi(token); //Converto in intero il numero di processi figlio da creare.
	}else{
		return 0; //In caso di file vuoto termino.
	}
	
	double results[nOp]; //Array dei risultati.
	pid_t sons[nProc]; //Array dei pid dei processi figli.
	int first[nProc]; //Array usato per cambiare il valore dei semafori.
	int opExec[nProc]; //Array per vedere se vi è un operazione nel figlio e capire quale è.
	struct Operation *sharedMem[nProc];

	//Inizializzazione di first e opExec e allocazione memoria condivisa.
	for(int i=0;i<nProc;i++){
		first[i]=0;
		opExec[i]=-1;
		sharedMem[i] = (struct Operation*) xmalloc(ftok(DATA,i),sizeof(struct Operation));
	}

	
	if((key=ftok("myapp.x",'a')) == -1)
		syserr("ftok","ftok failure"); //Errore nella ftok.
	                  
	if((semid=semget(key, 2*nProc, IPC_CREAT|IPC_EXCL|0666)) == -1) //Creazione semafori.
		syserr("Semaphore","error in create semaphore"); //Errore nella creazione dei semafori.

	for(int i=0;i<nProc;i++){ //Creo tramite fork i vari processi figlio che verranno utilizzati per effettuare i calcoli.
		if((sons[i]=fork()) == -1){
			syserr("Figlio","fork failure");
		}

		if(sons[i]==0){
			i=nProc;
		}
	}

	int padre=1;

	for(int i=0;i<nProc;i++){
		if(sons[i] == 0){
			// INIZIO CORPO PROCESSO FIGLIO
			//Inizializzazione semafori.
			p1.sem_num=i;
			v2.sem_num=nProc+i;
			int save=i;
			i=nProc;
			sprintf(text,"Figlio %i creato! Padre:%d Io:%d\n",save+1,getppid(),getpid());
			if(write(STDOUT,text,strlen(text)) == -1){
				syserr(RESULT,"write failure");
			}
			padre=0;
			while(1){
				semop(semid,&p1,1); //Semaforo bloccante in attesa di una operazione.
				
				//Se op == 'K' allora termino il figlio.
				//Altrimenti eseguo l'operazione contenuta nella struttura in memoria condivisa.
				if(sharedMem[save]->op == 'K'){
					sprintf(text,"******** Terminazione figlio %i eseguita con successo!\n",save+1);
					if(write(STDOUT,text,strlen(text)) == -1){
						syserr(RESULT,"write failure");
					}
					exit(0);
				}else{
					switch(sharedMem[save]->op){
						case '+': sharedMem[save]->res=sharedMem[save]->num1+sharedMem[save]->num2; break;
						case '-': sharedMem[save]->res=sharedMem[save]->num1-sharedMem[save]->num2; break;
						case '*': sharedMem[save]->res=sharedMem[save]->num1*sharedMem[save]->num2; break;
						case '/': sharedMem[save]->res=sharedMem[save]->num1/sharedMem[save]->num2; break;
					}
				}
				semop(semid,&v2,1); //Semaforo che sblocca il padre.
			}
			
			exit(0);
			// FINE CORPO PROCESSO FIGLIO
		}
	}

	if (padre == 1){
		// INIZIO CORPO PROCESSO PADRE
		//Estraggo dal file per ogni riga l'id del processo, e l'operazione da eseguire.
		token = strtok(NULL, "\n");
		int pos=-1;
		int righe=0;
			
		while(token != NULL){
			//Estrazione ID processo.
			for (int i=0;i<strlen(token) && pos==-1;i++){
				if((char)token[i]==' ') pos=i;
			}

			if(pos == -1) syserr(DATA,"Error in the file of configuration.");

			char id[pos+1];
			memcpy(id,token,pos);
			id[pos]='\0';
			int rest=(int)strlen(token)-pos;
			char resto[rest];
			memcpy(resto,token+pos+1,rest);
			resto[rest]='\0';
			pos=-1;
			//Process ID estratto.
				
			prid=atoi(id);
				
			if(0>prid){
				syserr("Error","id failure"); //Errore in caso l'id sia minore di 0.
			}else if(nProc<prid){
				syserr("Error","id failure"); //Errore in caso l'id non sia accettabile.
			}
			char saveOp;
			//Estrazione operazioni.
			for (int i=0;i<strlen(resto) && pos==-1;i++){
				if(resto[i]=='+' || resto[i]=='-' || resto[i]=='*' || resto[i]=='/'){
					if(i!=0){
						pos=i;
						saveOp=resto[i];
					}
				}
			}

			if (pos == -1) syserr(DATA,"found an incorrect operation"); //Errore nel caso si trovi un'operazione scritta nel modo sbagliato.

			char primo[pos+1];
			memcpy(primo,resto,pos);
			primo[pos]='\0';
			rest=(int)strlen(resto)-pos;
			char secondo[rest];
			memcpy(secondo,resto+pos+1,rest);
			secondo[rest]='\0';
			//Fine estrazione operazioni.

			if( prid!=0 ){ 
				//Nel caso in cui l'ID è diverso da 0, invio al determinato processo l'operazione da eseguire.
				v1.sem_num=prid-1;
				p2.sem_num=nProc+prid-1;
				v2.sem_num=nProc+prid-1;
				//Nel caso in cui sia la prima volta che deve eseguire l'operazione sblocco il semaforo.
				if(first[prid-1] == 0 && semctl(semid,nProc+prid-1,GETVAL) == 0){
					first[prid-1]++;
					semop(semid,&v2,1);
				}else if(first[prid-1] == 0 && semctl(semid,nProc+prid-1,GETVAL) == 1){
					first[prid-1]++;
				}

				semop(semid,&p2,1); //Semaforo del padre. Bloccato se il figlio sta già eseguendo un'operazione.
				
				sprintf(text,"Invio al figlio %i l'operazione da eseguire.\n",prid);
				if(write(STDOUT,text,strlen(text)) == -1){
					syserr(RESULT,"write failure");
				}

				if(opExec[prid-1] != -1){ //Controllo se era stata fatta un'operazione precedentemente nel processo. 
					results[opExec[prid-1]]=sharedMem[prid-1]->res;
				}

				//Invio in memoria condivisa le operazioni.
				sharedMem[prid-1]->num1=atof(primo);
				sharedMem[prid-1]->num2=atof(secondo);
				sharedMem[prid-1]->op=saveOp;

				opExec[prid-1]=righe;	//Posizione dell'operazione che ho appena inviato.
				semop(semid,&v1,1); //Sblocco il semaforo del figlio.
			}else{
				//Nel caso id è 0. Scelgo il primo processo libero a caso.
				
				//Sblocco il semaforo di ogni processo. Nel caso sia la prima volta che vengono usati.
				for(int i=0;i<nProc;i++){
					v2.sem_num=nProc+i;
					if(first[i] == 0 && semctl(semid,nProc+i,GETVAL) == 0){
						first[i]++;
						semop(semid,&v2,1);
					}else if(first[i] == 0 && semctl(semid,nProc+i,GETVAL) == 1){
						first[i]++;
					}
				}

				int catch=-1;
				//Selezione del processo libero.
				for(int j=0;catch == -1;j++){
					if(j == nProc){
						j=0;
					}

					if(semctl(semid,nProc+j,GETVAL) == 1){
						catch=j;
					}
				}


				v1.sem_num=catch;
				p2.sem_num=nProc+catch;

				semop(semid,&p2,1);

				sprintf(text,"ID 0 - Attivato processo %i\n",catch+1);
				if(write(STDOUT,text,strlen(text)) == -1){
					syserr(RESULT,"write failure");
				}

				if(opExec[catch] != -1){ //Controllo se era stata fatta un'operazione precedentemente nel processo. 
					results[opExec[catch]]=sharedMem[catch]->res;
				}

				//Invio in memoria condivisa le operazioni.
				sharedMem[catch]->num1=atof(primo);
				sharedMem[catch]->num2=atof(secondo);
				sharedMem[catch]->op=saveOp;

				opExec[catch]=righe;	//Posizione dell'operazione che ho appena inviato.
				semop(semid,&v1,1);	//Sblocco il semaforo del figlio.
			}

			//Prelevo la prossima riga.
			pos=-1;
			token = strtok(NULL, "\n");
			righe++;


		}

		//Fine della lettura del file di configurazione.
		//Nel caso siano state fatte operazioni salvo i risultati nell'array.
		for(int i=0;i<nProc;i++){
			p2.sem_num=nProc+i;
			v2.sem_num=nProc+i;
			v1.sem_num=i;
			if(first[i] == 0 && semctl(semid,nProc+i,GETVAL) == 0){
				first[i]++;
				semop(semid,&v2,1);
			}else if(first[i] == 0 && semctl(semid,nProc+i,GETVAL) == 1){
				first[i]++;
			}

			semop(semid,&p2,1);
			if(opExec[i] != -1){
				results[opExec[i]]=sharedMem[i]->res;
			}
			sharedMem[i]->op='K'; //Invio segnale di terminazione al figlio i-esimo.
			semop(semid,&v1,1); //Sblocco il figlio.
		}

		for(int i=0;i<nProc;i++){
			wait(&sons[i]); //Attendo che tutti i figli terminino.
		}

		//Scrittura dei risultati nel file results.txt
		isOpen = open(RESULT,O_RDWR|O_TRUNC,S_IRUSR|S_IWUSR); //Apertura file dei risultati.
		
		if(isOpen == -1){
			syserr(RESULT,"open failure");
			return -1;
		}

		for(int i=0;i<nOp;i++){	
			sprintf(text,"%f\n",results[i]);
			if(write(isOpen,text,strlen(text)) == -1){
				syserr(RESULT,"write failure");
			}
		}

		semctl(semid,0,IPC_RMID); //Eliminazione semafori.

		for(int i=0;i<nProc;i++){
			xfree(sharedMem[i]); //Libero la memoria.
		}

	}
	return 0;
}