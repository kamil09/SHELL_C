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
#include <pthread.h>

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

/**
 * Funcka do ubsługi CRTL+C
*/
void obsluga_CTRL_C(){
}
/**
 * Funkcja do obslugi sygnału zakończenia procesu
*/
void obsluga(int signo){
	pthread_t th;
	int s;
	char *dir;

	pid_t pidP = waitpid(-1,&s,WNOHANG);
 	
 	if(terminal==1 && pidP>0) {
 		dir = getcwd(dir,100);
		dir = strcat(dir," >>> ");
 		printf("\nProces zakończony. PID: %d Status: %d\n\n",pidP, s>>8);
 		printf("%s",dir);
 		fflush(stdout);
 	}
}

void obsluga_CTRL_Z(int signo){
	fflush(stdout);
	if( (fgPID > 0)  && ( fgPID!=ppid ) ){
		stoppedProc[zatrzymanych]=fgPID;
		zatrzymanych++;
		kill( fgPID,SIGSTOP );
		printf("Zatrzymany PID: %d\n",fgPID);
		fgPID=-1;
		fflush(stdout);
	}
}
/**
 * Funkcja która dzieli linię na kilka mniejszych wykonywanych sekwencyjnie
*/
int podzielLinie(char *l);
/**
 * Funkcja która dzieli linie na komendy wykonywane współbierznie
*/
int podzielLinieWspolbierzne(char *l);
/**
 * Funkcja która zamienia pojedyńczą linię na listę argumentów
 * Wyrzuca podwójne spacje
 * Sprawdza, czy linia jest zakomentowana
*/
int zamienLinie(char *lineT);

void wykonajKomende(int iloscPotokow, int obecnyPotok, char *potokiArgv[30][100] ){
	int err, i=0;
	int zapis = 0;		// 0 - normalnie ; 1 - zapis < ; 2 - dopisywanie >>
	int odczyt =0;
	char *inFILE;
	char *outFILE;

	for(i = 0; i < 100; i++){
			if(potokiArgv[iloscPotokow-obecnyPotok][i]){
				if(!strcmp(potokiArgv[iloscPotokow-obecnyPotok][i],"<")){
					odczyt=1;
					inFILE=potokiArgv[iloscPotokow-obecnyPotok][i+1];
					potokiArgv[iloscPotokow-obecnyPotok][i]=0;potokiArgv[iloscPotokow-obecnyPotok][i+1]=0;i++;
					continue;
				}
				if(!strcmp(potokiArgv[iloscPotokow-obecnyPotok][i],">")){
					zapis=1;
					outFILE=potokiArgv[iloscPotokow-obecnyPotok][i+1];
					potokiArgv[iloscPotokow-obecnyPotok][i]=0;potokiArgv[iloscPotokow-obecnyPotok][i+1]=0;i++;
					continue;
				}
				if(!strcmp(potokiArgv[iloscPotokow-obecnyPotok][i],">>")){
					zapis=2;
					outFILE=potokiArgv[iloscPotokow-obecnyPotok][i+1];
					potokiArgv[iloscPotokow-obecnyPotok][i]=0;potokiArgv[iloscPotokow-obecnyPotok][i+1]=0;i++;
					continue;
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
		
		err = execvp( potokiArgv[iloscPotokow-obecnyPotok][0],(char**) potokiArgv[iloscPotokow-obecnyPotok] );
		
		if(err==-1) {
			fprintf(stderr, "Błąd przy próbie wykonania komendy : %s\n", potokiArgv[iloscPotokow-obecnyPotok][0] );
			exit(1);
		}
		else exit(0);
}

/**
 * Rekurencyjna funkcja wykonująca polecenia potokowe
 */
void wykonajPoleceniePotokowe(int iloscPotokow, int obecnyPotok, char *potokiArgv[30][100] ){
	int i=0;
	int pid;
	int stat;
	
	int fd[2];
	int fd2[2];

	pipe(fd);

	if( obecnyPotok==iloscPotokow ){
		//Sprawdzanie przekiewowania wejscia wyjscia i równoległości
		wykonajKomende(iloscPotokow,obecnyPotok,potokiArgv);
	}
	else{
		if((pid=fork())==0){
			dup2(fd[1],1);
			wykonajPoleceniePotokowe(iloscPotokow,obecnyPotok+1,potokiArgv);
		}
		else{
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
/**
 * Wypisuje procesy wstrzymane
 */
void runJobs(){
	int i=0;
	puts("\nWSTRZYMANE PROCESY :\n");
	for(i=0; i< zatrzymanych; i++){
		printf("\tPID: %d\n",stoppedProc[i]);
	}
	printf("\n\tWykonaj \"fd <PID> \" aby uruchomić odpowiedni proces w trybie pierwszoplanowym");
	printf("\n\tWykonaj \"bg <PID> \" aby uruchomić odpowiedni proces w tle\n\n");
}

/**
 * Usuwa proces z listy procesów wstrzymanych
 */
void usunZListyBG(int pidT){
	int i=0;
	for(i=0;i < zatrzymanych ; i++ ){
		if(stoppedProc[i]==pidT ) {
			stoppedProc[i]=0;
			break;
		}
	}
	for(i; i < zatrzymanych ; i++){
		stoppedProc[i]=stoppedProc[i+1];
	}
	zatrzymanych--;
}

/**
 * Wznawia proces w trybie pierwszoplanowym
 */
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
		else if (WIFSTOPPED(stat))
    		break;
	}
	usunZListyBG(pidT);
}
/**
 * Wznawia proces w tle
 */
void runBG(int pidT){
	kill(pidT,SIGCONT);
	printf("w tle [PID: %d]\n", pidT);
	usunZListyBG(pidT);
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
	int wTle=1;
	char *dir;
	
	ppid=getpid();

	if(isatty(0)) terminal=1;					//WCZYTANA LINIA

	while(1){
		signal(SIGINT,obsluga_CTRL_C);							//Sygnał przerwania	
		signal(SIGTSTP, obsluga_CTRL_Z);
		dir = getcwd(dir,100);
		dir = strcat(dir," >>> ");

		if(terminal==1) line=readline(dir);	//Wczytanie lini
		else line=readline(NULL);	
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
					if(!strcmp(argv[0],"fg") && argv[1] ) { runFG(atoi(argv[1])); continue;}
					if(!strcmp(argv[0],"bg") && argv[1] ) { runBG(atoi(argv[1])); continue;}

					if( (argv[0]!=0) && (argv[0][0]>30 ) ) wykonajPolecenie(argc,argv,wTle);	//WYKONUJE POLECENIE
				}

			}
		}
		else if(line == NULL ) exit(0);
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
	if(r>=1) return 1;
	else return 0;

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
