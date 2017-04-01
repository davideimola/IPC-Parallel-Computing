#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "mylib.h"

void syserr(char *prog, char *msg){
	char text[2048];
    sprintf(text,"Errore in %s - %s\n",prog,msg);
	if(write(STDOUT,text,strlen(text)) == -1){
		exit(1);
	}
    exit(1);
}

//Funzione simil malloc per memoria condivisa.
void * xmalloc(key_t key,const size_t size){ 
	const int shmid=shmget(key,size+sizeof(XMem)-sizeof(char),0666|IPC_CREAT);
	if(shmid==-1) return NULL;
	XMem * ret = (XMem *)shmat(shmid,NULL,0);
	if(ret==(void *)-1) return NULL;
	ret->key = key;
	ret->shmid = shmid;
	return ret->buf;
}


//Funzione simil free per memoria condivisa.
void xfree(void * ptr){ 
	XMem tmp;
	XMem * mem = (XMem *)(((char *)ptr)-(((char *)&tmp.buf)-((char *)&tmp.key)));
	const int shmid = mem->shmid;
	shmdt(mem);
	shmctl(shmid,IPC_RMID,NULL);
}