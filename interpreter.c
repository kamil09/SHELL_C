#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

char *wszystkieLinie[200];		//Wczytana linia zostaje podzielona na kilka lini, które były oddzielone średnikami. Linie te będą wykonywane sekwencyjnie
char *komendyRownolegle[200];	//Linia sekwencyjna zostaje podzielona na linie wykonywane współbierznie
int iloscLini = 0;				//Ilość lini
int iloscRownoleglych = 0;
int argc=0;						//ILOŚC ARGUMENTÓW
char *argv[200];				//ARGUMENTY
int terminal=0;
int stoppedProc[100];			//Tablica wstrzymanych procesów
int zatrzymanych=0;
int fgPID=-1;
pid_t ppid=-1;
char *dir;

//OBSŁUGA CTRL+C
void obsluga_CTRL_C(){}
//OBSŁUGA SYGNAŁU ZAKOŃCZENIA PROCESU
void obsluga(int signo);
//OBSŁUGA CTRL+Z
void obsluga_CTRL_Z(int signo);
//Funkcja która dzieli linię na kilka mniejszych wykonywanych sekwencyjnie
int podzielLinie(char *l);
//Funkcja która dzieli linie na komendy wykonywane współbierznie
int podzielLinieWspolbierzne(char *l);
//Wypisuje wrgumenty
void wypiszEcho(char *argvT[100],int argcT);
//PRZEKIEROWYJE STRUMIENIE WEJŚCIA I WYJSCIA
void przekierowanieStrumieni(int argcT, char *argvT[100]);
//Funkcja która zamienia pojedyńczą linię na listę argumentów ; Wyrzuca podwójne spacje ; Sprawdza, czy linia jest zakomentowana
int zamienLinie(char *lineT);
//Usuwa zmienne środowiskowe
void unSetVar(int argcT, char *argvT[100]);
//Ustawie zmienną środowiskową
int ustawZmienne(int argcT, char *argvT[100]){
	int i=0, k=0, przesuniecie=0;
	char *name=NULL, *val=NULL;
	for(k=0; k < argcT;k++){
		name=argvT[k];
		i=1;
		while( *(argvT[k]+i) != 0 ){
			if( (*(argvT[k]+i)=='=') && (*(argvT[k]+i-1)!='\\') ){
				*(argvT[k]+i)=0;
				val=(argvT[k]+i+1);
				przesuniecie++;
				setenv(name,val,1);
			}
			i++;
		}
	}
	for(i=przesuniecie; i<=argcT; i++ ) argvT[i-przesuniecie]=argvT[i];
	return argcT-przesuniecie;
}

void wykonajKomende(int iloscPotokow, int obecnyPotok, char *potokiArgv[30][100] ){
	int err;
	przekierowanieStrumieni(100, potokiArgv[iloscPotokow-obecnyPotok]);
	if(potokiArgv[iloscPotokow-obecnyPotok][0] && !strcmp(potokiArgv[iloscPotokow-obecnyPotok][0],"echo")) { 
		wypiszEcho(potokiArgv[iloscPotokow-obecnyPotok], 100); 
		exit(0);
	}
	err = execvp( potokiArgv[iloscPotokow-obecnyPotok][0],(char**) potokiArgv[iloscPotokow-obecnyPotok] );
	if(err==-1) {
		fprintf(stderr, "Błąd przy próbie wykonania komendy : %s\n", potokiArgv[iloscPotokow-obecnyPotok][0] );
		exit(1);
	}
	else exit(0);
}

//Rekurencyjna funkcja wykonująca polecenia potokowe
void wykonajPoleceniePotokowe(int iloscPotokow, int obecnyPotok, char *potokiArgv[30][100] ){
	int i=0, pid, stat;
	int fd[2];
	pipe(fd);
	if( obecnyPotok==iloscPotokow ) wykonajKomende(iloscPotokow,obecnyPotok,potokiArgv);
	else{
		if((pid=fork())==0){	//Dziecko - idziemy głębiej
			dup2(fd[1],1);
			wykonajPoleceniePotokowe(iloscPotokow,obecnyPotok+1,potokiArgv);
		}
		else{					//Rodzić - odpalamy polecenie
			dup2(fd[0], 0);
			close(fd[1]);
			wykonajKomende(iloscPotokow,obecnyPotok,potokiArgv);
		}
	}
}

/**
 * Funcka która wykonuje pojedyńcze polecenie lub cały potok
*/
void wykonajPolecenie(int argcT, char *argvT[100], int wTle){
	int err, i=0, k=0, l=0, pid, stat=0;
	int potoki=0;
	char *potokiArgv[30][100];
	char argvCopy[argcT+1][200];
	signal(SIGCHLD, obsluga);
	if((pid=fork())==0){
	    signal(SIGTSTP, SIG_DFL);
		//DOKONUJEMY KOPI DANYCH
		//DANE Z LINI MOGĄ ZOSTAĆ NADPISANE, A MOGĄ BYĆ POTRZEBNE DLA POTOKU KTÓRY BĘDZIE SIĘ WYKONYWAŁ RÓWNOLEGLE
		for(i=0;i< argcT;i++)
		strcpy(argvCopy[i],argvT[i]);
		k=0;
		for(i = 0; i < argcT; i++){			
			if(argvCopy[i]){
				if(!strcmp(argvCopy[i], "|" )){
					potoki++;
					argv[i]=0;
					potokiArgv[k][l+1] = 0;
					k++;
					l=0;
					i++;
				}
			}
			potokiArgv[k][l]=argvCopy[i];
			l++;
		}
		wykonajPoleceniePotokowe(potoki,0,potokiArgv);
	}
	else{
		if( wTle == 0) {
			fgPID=pid;
			waitpid(pid, &stat , WUNTRACED | WCONTINUED);
			if (WIFEXITED(stat)){
				if(terminal==1) {
					printf("\nProces zakończony. PID: %d Status: %d\n\n",pid, stat>>8);
					fflush(stdout);
				}
			}
			else if (WIFSIGNALED(stat)){
				if(terminal==1) {
					printf("\nProces ubity. PID: %d Status: %d\n\n",pid, stat>>8);
					fflush(stdout);
				}
			}
		}
		else {
			printf("w tle [PID: %d]\n", pid);
		}
	}
}

// Wypisuje procesy wstrzymane
void runJobs();
//Usuwa proces z listy procesów wstrzymanych
void usunZListyBG(int pidT);
//Wznawia proces w trybie pierwszoplanowym
void runFG(int pidT);
//Wznawia proces w tle
void runBG(int pidT);

/**
 * Główna funkcja programu
*/
int main(int argcM, char **argvM){
	int i = 0, k = 0, status=0, plikW, defIN, wTle=1;
	char *line;
	pid_t pidT;
	ppid=getpid();
	if(isatty(0)) terminal=1;					//WCZYTANA LINIA
	if(argcM>1) {
		terminal=0;
		close(0);
		plikW = open(argvM[1], O_RDONLY);
	}
	while(1){
		signal(SIGINT,obsluga_CTRL_C);							//Sygnał przerwania	
		signal(SIGTSTP, obsluga_CTRL_Z);
		dir = getcwd(dir,100);
		dir = strcat(dir," >>> ");
		if(terminal==1) line=readline(dir);	//Wczytanie lini
		else {
			defIN=dup(1);
			close(1);
			line=readline(NULL);
			dup2(defIN,1);
			close(defIN);
		}	
		if( (line != NULL) && (*line)){
			add_history(line);						//DODAJE LINIE DO HISTORII
			podzielLinie(line);						//ROZBIJA NA LINIE SEKWENCYJNE
			for(i = 0; i < iloscLini ; i++){
				wTle=1;
				wTle=podzielLinieWspolbierzne(wszystkieLinie[i]);								//ROZBIJA NA LINIE WSPOLBIERZNE
				for(k = 0 ; k<iloscRownoleglych; k++ ){	
					if(!strcmp(line,"exit")) exit(0);
					if(!strcmp(line,"jobs")) {
						runJobs();
						continue;
					}
					zamienLinie(komendyRownolegle[k]);											//ZAMIENIA LINIE NA KOMENDY I PARAMETRY W TABLICY
					if(argv[0]) argc=ustawZmienne(argc,argv);
					if(argv[0] && !strcmp(argv[0],"unset") ){ unSetVar(argc,argv) ;continue;}
					if(argv[0] && !strcmp(argv[0],"fg") && argv[1] ) { runFG(atoi(argv[1])); continue;}
					if(argv[0] && !strcmp(argv[0],"bg") && argv[1] ) { runBG(atoi(argv[1])); continue;}
					if( (argv[0]!=0) && (argv[0][0]>30 ) ) wykonajPolecenie(argc,argv,wTle--);	//WYKONUJE POLECENIE
				}
			}
		}
		else if(line == NULL) exit(0);
	}
	return 0;
}




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
int podzielLinieWspolbierzne(char *l){
	int i = 1;
	int k = 0;
	char c;
	int r=0;
	for(i=0;i<200;i++) komendyRownolegle[i]=0;
	i=1;
	c=l[k];
	komendyRownolegle[0]=l;
	while( c!= 0 ){
		if(c=='&'){
			*(l+k)=0;
			r++;
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
	return r;
}
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
					argc++;
				}
			}
		}
		k++;
		c=lineT[k];
	}
	return 0;
}
void obsluga_CTRL_Z(int signo){
	if( (fgPID > 0)  && ( fgPID!=ppid ) ){
		stoppedProc[zatrzymanych]=fgPID;
		zatrzymanych++;
		kill( fgPID,SIGSTOP );
		printf("Zatrzymany PID: %d\n",fgPID);
		fgPID=-1;
	}
	fflush(stdout);
}
void obsluga(int signo){
	int s;
	pid_t pidP = waitpid(-1,&s,WNOHANG);
 	if(terminal==1 && pidP>0) {
 		printf("\nProces zakończony. PID: %d Status: %d\n\n %s",pidP, s>>8,dir);
 		fflush(stdout);
 	}
}
void runJobs(){
	int i=0;
	puts("\nWSTRZYMANE PROCESY :\n");
	for(i=0; i< zatrzymanych; i++) printf("\tPID: %d\n",stoppedProc[i]);
	printf("\n\tWykonaj \"fd <PID> \" aby uruchomić odpowiedni proces w trybie pierwszoplanowym");
	printf("\n\tWykonaj \"bg <PID> \" aby uruchomić odpowiedni proces w tle\n\n");
}
void usunZListyBG(int pidT){
	int i=0;
	for(i=0;i < zatrzymanych ; i++ )
		if(stoppedProc[i]==pidT ) {
			stoppedProc[i]=0;
			break;
		}
	for(i; i < zatrzymanych ; i++) stoppedProc[i]=stoppedProc[i+1];
	zatrzymanych--;
}
void runBG(int pidT){
	kill(pidT,SIGCONT);
	printf("w tle [PID: %d]\n", pidT);
	usunZListyBG(pidT);
}
void runFG(int pidT){
	int stat;
	kill(pidT,SIGCONT);
	fgPID=pidT;
	while(1){
		waitpid(pidT, &stat , WUNTRACED | WCONTINUED);
		if (WIFEXITED(stat)){
			if(terminal==1) {
				printf("\nProces zakończony. PID: %d Status: %d\n\n",pidT, stat>>8);
				fflush(stdout);
			}
			break;
		}
		else if (WIFSIGNALED(stat)){
			if(terminal==1) {
				printf("\nProces ubity. PID: %d Status: %d\n\n",pidT, stat>>8);
				fflush(stdout);
			}
			break;
		}
		else if (WIFSTOPPED(stat)) break;
	}
	usunZListyBG(pidT);
}
void wypiszEcho(char *argvT[100], int argcT){
	int i=0;
	for(i=1; i<argcT ; i++ )
		if(argvT[i]) printf("%s ",argvT[i]);
	printf("\n");
}
void przekierowanieStrumieni(int argcT, char *argvT[100]){
	int i=0;
	int zapis = 0;		// 0 - normalnie ; 1 - zapis < ; 2 - dopisywanie >>
	int odczyt =0;
	char *inFILE, *outFILE;
	for(i = 0; i < argcT; i++){
		if(argvT[i]){
			if(!strcmp(argvT[i],"<")){
				odczyt=1;
				inFILE=argvT[i+1];
				argvT[i]=0;argvT[i+1]=0;i++;
			}
			else if(!strcmp(argvT[i],">")){
				zapis=1;
				outFILE=argvT[i+1];
				argvT[i]=0;argvT[i+1]=0;i++;
			}
			else if(!strcmp(argvT[i],">>")){
				zapis=2;
				outFILE=argvT[i+1];
				argvT[i]=0;argvT[i+1]=0;i++;
			}
		}
	}		
	if(zapis>0){
		close(1);
		if(zapis==1) {
			zapis = open( outFILE, O_WRONLY | O_CREAT, 0644);
			ftruncate(zapis, 0);
		}
		else zapis = open( outFILE, O_WRONLY | O_APPEND | O_CREAT, 0644);
	}
	if(odczyt>0){
		close(0);
		odczyt = open(inFILE, O_RDONLY);
	}
}
void unSetVar(int argcT, char *argvT[100]){
	int i=0;
	for(i=1;i<argcT;i++) unsetenv(argvT[i]);
}