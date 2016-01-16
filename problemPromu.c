/**
	Ten plik nie ma nic wspólnego z interpreterem,
	jednak powstał na tym samym przedmiocie

	Program : rozwiązanie problemu promu (synchronizacja procesów)
	Autor: Kamil Piotrowski
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h>
//Liczba pojazdow na promie i ich poszczególne wielkości
int idProm, *promArgv;
struct sembuf semWolneL;
struct sembuf semWolneP;
struct sembuf semPojazd;
struct sembuf semDane;
int wolneL;
int wolneP;
int pojazd;
int dane;

void obsluga_CTRL_C(){
	promArgv[0]=0;
	semctl(wolneL, 0, IPC_RMID, 0);
    semctl(wolneP, 0, IPC_RMID, 0);
    semctl(pojazd, 0, IPC_RMID, 0);
    exit(0);
}

wyladuj(int *promArgv){
	int i=0;
	puts("Prom dotarł do brzegu, następuje wyładowanie pojazdów o wartościach: ");
	for(i=2;i<=promArgv[1]+1;i++){ 
		printf("%d, ", promArgv[i] );
	}
	puts("");
	promArgv[1]=0;
}

int main(){

	int i=0;
	int idPojazdu;
	int strona;
	int masa;

    signal(SIGINT,obsluga_CTRL_C);	
    
    idProm = shmget(0x128, 1024, 0600 | IPC_CREAT);
    promArgv = (int *)shmat(idProm, 0,0);
        
    wolneL = semget(0x125, 1, 0600 | IPC_CREAT);
    wolneP = semget(0x126, 1, 0600 | IPC_CREAT);
    pojazd = semget(0x127, 1, 0600 | IPC_CREAT);
    dane = semget(0x129, 1, 0600 | IPC_CREAT);
    
    promArgv[0]=1;
    promArgv[1]=0;

    semWolneL.sem_num=0;
    semWolneL.sem_flg=0;
    semWolneP.sem_num=0;
    semWolneP.sem_flg=0;
    semPojazd.sem_num=0;
    semPojazd.sem_flg=0;
    semDane.sem_num=0;
    semDane.sem_flg=0;
    semctl(wolneL, 0, SETVAL, 20);
    semctl(wolneP, 0, SETVAL, 0);
    semctl(pojazd, 0, SETVAL, 0);
    semctl(dane, 0, SETVAL, 1);
    //POJAZDY
    
    if(fork()==0){
    	sleep(3);
	   	for(i=0;i<1000;i++){
		    if(fork()==0){
		        strona=i%2;
		        idPojazdu=i;
		       	masa = ((i-strona)%20)+1;
		        while(promArgv[0]){
		        	if(strona==0){
		        		semWolneL.sem_op = -masa;
		    			semop(wolneL, &semWolneL, 1);    		
		    			printf("Pojazd %d o wartości: %d wchodzi na pokład\n",idPojazdu, masa);
		    			semDane.sem_op = -1;
		    			semop(dane, &semDane, 1); 
		    			promArgv[1]++;
	    				promArgv[promArgv[1]+1]=masa;
	    				semDane.sem_op = 1;
		    			semop(dane, &semDane, 1);
		    			semPojazd.sem_op = masa;
	    				semop(pojazd, &semPojazd, 1);
		        		strona=1-strona;
		        	}
		        	else{
		        		semWolneP.sem_op = -masa;
		        		semop(wolneP, &semWolneP, 1);
		        		printf("Pojazd %d o wartości: %d wchodzi na pokład\n",idPojazdu, masa);
		        		semDane.sem_op = -1;
		    			semop(dane, &semDane, 1);
		        		promArgv[1]++;
	    				promArgv[promArgv[1]]=masa;
	    				semDane.sem_op = 1;
		    			semop(dane, &semDane, 1);
		        		semPojazd.sem_op = masa;
	    				semop(pojazd, &semPojazd, 1);
		        		strona=1-strona;
		        	}
		        	sleep(2);
		        }
		    }
		    else sleep(1);
		}
	}
	else{
	    //PROM
	    while(promArgv[0]){
	    	puts("Prom oczekuje na pojazdy...");
	    	semPojazd.sem_op = -20;
	    	semop(pojazd, &semPojazd, 1);
	    	puts("Prom płynie na drugi brzeg...");
	    	sleep(5);
	    	wyladuj(promArgv);

	    	semWolneP.sem_op = 20;
	    	semop(wolneP, &semWolneP, 1);
	    	
	    	puts("Prom oczekuje na pojazdy...");
	    	semPojazd.sem_op = -20;
	    	semop(pojazd, &semPojazd, 1);
	    	puts("Prom wraca na pierwszy brzeg...");
	    	sleep(5);
	    	wyladuj(promArgv);

	    	semWolneL.sem_op = 20;
	    	semop(wolneL, &semWolneL, 1);
	    }
	}
    
	return 0;
}

