#include<stdio.h>
#include<readline/readline.h>
#include<readline/history.h>
#include<string.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/wait.h>

char *line;		//WCZYTANA LINIA
int argc=0;		//ILOŚC ARGUMENTÓW
char *argv[200];	//ARGUMENTY

void zamienLinie(){
	char c=0;
	int k=0;
	int i=0;
	for(i=0;i<100;i++) argv[i]=0;
	argc=0;

	argc++;
	c=line[k];
	while(c==' '){
		k++;
		c=*(line+k);
	}
	argv[0]=line+k;
	while( (c!=0) ){
		if(c==' '){
			if( *(line+(k+1))==' ' ){
				*(line+k)=0;
			}
			else{
				*(line+k)=0;
				argv[argc]=line+(k+1);
				argc++;
			}
		}
		k++;
		c=line[k];
	}
	for(i =0; i<argc ;i++){
		printf("%s ",argv[i]);
	}
}

void wykonajPolecenie(){
	if(fork()==0){
		execvp(argv[0] , argv);
	}
}

int main(){
	while((line=readline("\npisz >>> "))!=NULL){
		if(*line){
			zamienLinie();
			wykonajPolecenie();
			add_history(line);	//DODAJE LINIE DO HISTORII
		}

	}
}
