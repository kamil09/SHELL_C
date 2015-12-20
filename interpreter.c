#include<stdio.h>
#include<readline/readline.h>
#include<readline/history.h>
#include<string.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/wait.h>

char *wszystkieLinie[200];	//Wczytana linia zostaje podzielona na kilka lini, które były oddzielone średnikami. Linie te będą wykonywane sekwencyjnie
int iloscLini = 0;		//Ilość lini
int argc=0;				//ILOŚC ARGUMENTÓW
char *argv[200];		//ARGUMENTY
int wTle=0;
int terminal=1;				

/**
 * Funkcja do obslugi sygnału zakończenia procesu
*/
void obsluga(int signo){
	int s;
	pid_t pidP = wait(&s);
	if(terminal==1) printf("\nProces zakończony. PID: %d Status: %d\n",pidP, s>>8);
}
/**
 * Funkcja która dzieli linię na kilka mniejszych wykonywanych sekwencyjnie
*/
int podzielLinie(char *l){
	int i = 1;
	int k = 0;
	char c;
	for(i=0;i<200;i++) wszystkieLinie[i]=0;
	i=1;
	c=l[k];
	wszystkieLinie[0]=l;
	
	while( c!= 0 ){
		if(c==';'){
			*(l+k)=0;
			wszystkieLinie[i]=l+(k+1);
			i++;
		}		
		k++;
		c=l[k];
	}
	iloscLini=i;	
}
/**
 * Funkcja która zamienia pojedyńczą linię na listę argumentów
 * Wyrzuca podwójne spacje
 * Sprawdza, czy linia jest zakomentowana
*/
int zamienLinie(char *line){
	char c=0;
	int k=0;
	int i=0;
	for(i=0;i<200;i++) argv[i]=0;
	argc=0;

	argc++;
	c=line[k];
	while(c==' '){
		k++;
		c=*(line+k);
	}
	if(*(line+k)=='#') return 5;
	argv[0]=line+k;
	while(c != 0) {
		if(c == '#' ) break;
		if(c==' '){
			if( *(line+(k+1))==' ' ){
				*(line+k)=0;
			}
			else{
				*(line+k)=0;
				if(*(line+(k+1))!=0 ) {
					argv[argc]=line+(k+1);
					if(!strcmp(argv[argc],"&")) { 
						wTle=1;
						argv[argc]=0;
					}
					argc++;
				}
			}
		}
		k++;
		c=line[k];
	}
//	for(i = 0; i<argc ;i++){
//		printf("%s ",argv[i]);
//	}
	return 0;
}
/**
 * Funcka która wykonuje pojedyńcze polecenie
*/
void wykonajPolecenie(char *argv[100]){
	int err;
	int pid;
	if((pid=fork())==0){
		err=execvp(argv[0] , argv);
		if(err==-1) {
			fprintf(stderr, "Błąd przy próbie wykonania komendy :%s\n", argv[0] ); 
			exit(1);
		}
		else exit(0);
	}
	else{
		signal(SIGCHLD, obsluga);
		if( wTle == 0) pause();
		else{
			printf("w tle [PID: %d]\n", pid);
		}
	}
}
/**
 * Funcka do ubsługi CRTL+C
*/
void obsluga_CTRL_C(){
}
/**
 * Główna funkcja programu
*/
int main(){
	int i = 0; 
	char *line;
	if(isatty(fileno(stdin))!=1 ) terminal=0;					//WCZYTANA LINIA
	
	while(1){
		//signal(SIGINT,obsluga_CTRL_C);					//Sygnał przerwania
		
		if(terminal==1) line=readline("Wprowadz komendę >>> ");	//Wczytanie lini
		else line=readline("");	
		if( (line != NULL) && (*line)){
			add_history(line);						//DODAJE LINIE DO HISTORII
			podzielLinie(line);
			for(i = 0; i < iloscLini ; i++){
				//printf("%s\n",wszystkieLinie[i] );
				if(!strcmp(line,"exit")) exit(0);
				zamienLinie(wszystkieLinie[i]);				//ZAMIENIA LINIE NA KOMENDY I PARAMETRY W TABLICY
				if(argv[0]!=0) wykonajPolecenie(argv);			//WYKONUJE POLECENIE
			}
		}
		else if(line == NULL ) exit(0);
	}
}
