#include<stdio.h>
#include<readline/readline.h>
#include<readline/history.h>
#include<string.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/wait.h>

char *wszystkieLinie[200];	//Wczytana linia zostaje podzielona na kilka lini, które były oddzielone średnikami. Linie te będą wykonywane sekwencyjnie
char *komendyRownolegle[200];	//Linia sekwencyjna zostaje podzielona na linie wykonywane współbierznie
int iloscLini = 0;		//Ilość lini
int iloscRownoleglych = 0;
int argc=0;			//ILOŚC ARGUMENTÓW
char *argv[200];		//ARGUMENTY
//int wTle=0;
int terminal=1;

/**
 * Funkcja do obslugi sygnału zakończenia procesu
*/
void obsluga(int signo){
	int s;
	//pid_t pidP = wait(&s);
	//if(terminal==1) printf("\nProces zakończony. PID: %d Status: %d\n",pidP, s>>8);
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
 * Funkcja która dzieli linie na komendy wykonywane współbierznie
*/
int podzielLinieWspolbierzne(char *l){
	int i = 1;
	int k = 0;
	char c;
	for(i=0;i<200;i++) komendyRownolegle[i]=0;
	i=1;
	c=l[k];
	komendyRownolegle[0]=l;
	while( c!= 0 ){
		if(c=='&'){
			if(*(l+(k+1))!=0){
				*(l+(k+1))=0;	
				komendyRownolegle[i]=l+(k+2);
				i++;
				k++;
			}
		}
		k++;
		c=l[k];
	}
	if(komendyRownolegle[i-1]==0) i--;
	iloscRownoleglych=i;
}
/**
 * Funkcja która zamienia pojedyńczą linię na listę argumentów
 * Wyrzuca podwójne spacje
 * Sprawdza, czy linia jest zakomentowana
*/
int zamienLinie(char *lineT){
	char c=0;
	int k=0;
	int i=0;
	for(i=0;i<200;i++) argv[i]=0;
	argc=0;

	argc++;
	c=lineT[k];
	while(c==' '){
		k++;
		c=*(lineT+k);
	}
	if(*(lineT+k)=='#') return 5;
	argv[0]=lineT+k;
	while(c != 0) {
		if(c == '#' ) break;
		if(c==' '){
			if( *(lineT+(k+1))==' ' ){
				*(lineT+k)=0;
			}
			else{
				*(lineT+k)=0;
				if(*(lineT+(k+1))!=0 ) {
					argv[argc]=lineT+(k+1);
					//if(!strcmp(argv[argc],"&")) { 
					//	wTle=1;
					//	argv[argc]=0;
					//}
					argc++;
				}
			}
		}
		k++;
		c=lineT[k];
	}
//	for(i = 0; i<argc ;i++){
//		printf("%s ",argv[i]);
//	}
	return 0;
}
/**
 * Funcka która wykonuje pojedyńcze polecenie
*/
void wykonajPolecenie(int argcT, char *argvT[100]){
	int err;
	int i=0;
	int pid;
	int wTle=0;
	int stat=0;
	for(i = 0; i < argcT; i++){
		if(!strcmp(argvT[i],"&")) { 
			wTle=1;
			argvT[i]=0;
			break;
		}
	}	
	if((pid=fork())==0){
		err=execvp(argvT[0] , argvT);
		if(err==-1) {
			fprintf(stderr, "Błąd przy próbie wykonania komendy :%s\n", argvT[0] ); 
			exit(1);
		}
		else exit(0);
	}
	else{
		signal(SIGCHLD, obsluga);
		if( wTle == 0) {
		//	waitpid(pid,&stat,0);
		}
		else printf("w tle [PID: %d]\n", pid);
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
	int k = 0;
	int status=0;
	char *line;
	pid_t pidT;
	if(isatty(fileno(stdin))!=1 ) terminal=0;					//WCZYTANA LINIA
	
	while(1){
		//signal(SIGINT,obsluga_CTRL_C);					//Sygnał przerwania
		
		if(terminal==1) line=readline("Wprowadz komendę >>> ");	//Wczytanie lini
		else line=readline("");	
		if( (line != NULL) && (*line)){
			add_history(line);						//DODAJE LINIE DO HISTORII
			podzielLinie(line);						//ROZBIJA NA LINIE SEKWENCYJNE
			for(i = 0; i < iloscLini ; i++){
			//	printf("%s\n",wszystkieLinie[i]);
				podzielLinieWspolbierzne(wszystkieLinie[i]);		//ROZBIJA NA LINIE WSPOLBIERZNE
			//	printf("%d\n",iloscRownoleglych);
				for(k = 0 ; k<iloscRownoleglych; k++ ){	
					if(!strcmp(line,"exit")) exit(0);
					zamienLinie(komendyRownolegle[k]);				//ZAMIENIA LINIE NA KOMENDY I PARAMETRY W TABLICY
					if(argv[0]!=0) wykonajPolecenie(argc,argv);			//WYKONUJE POLECENIE
				}
				while ((pidT = wait(&status)) > 0){
					if(terminal==1) printf("\nProces zakończony. PID: %d Status: %d\n",pidT, status>>8);				
				};
				
			}
		}
		else if(line == NULL ) exit(0);
	}
	return 0;
}
