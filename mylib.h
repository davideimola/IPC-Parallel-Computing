/** @file
 *  @brief File Header della Libreria.
 */

#ifndef MYLIB_H
#define MYLIB_H

#define STDIN 0 /**< @brief Standard di Input. */
#define STDOUT 1 /**< @brief Standard di Output. */

/**
 * @brief      Struttura dati per la memorizzazione in memoria condivisa.
 */
typedef struct XMem {
	key_t key;
	int shmid;
	char buf[1];
} XMem;

/**
 * @brief      Funzione di errore di sistema.
 *
 * @param      *prog Puntatore a char del programma che ha causato l'errore.
 * @param      *msg Puntatore a char del messaggio da stampare.
 */
void syserr (char *prog, char *msg);

/**
 * @brief      Funzione che assegna memoria condivisa. (Simile a malloc)
 *
 * @param      key Chiave di sistema generata da ftok().
 * @param      size Grandezza dello spazio che deve essere assegnato.
 * @return     void* Puntatore alla prima cella dello spazio di memoria condivisa assegnato.
 */
void * xmalloc(key_t key,const size_t size);

/**
 * @brief      Funzione che libera memoria condivisa. (Simile a free)
 *
 * @param      *ptr Puntatore alla prima cella dello spazio di memoria condivisa assegnato.
 */
void xfree(void * ptr);

#endif /* myib_h */
