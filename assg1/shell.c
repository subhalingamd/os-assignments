#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_ARGS  16
#define MAX_CHARS 1024
#define MAX_HIST  5


int main(int argc, char* argv[]) 
{
	void parse_input(char*,char*[]), update_history(char*,char*[],int*,int*,int*), serve_history(char*[],int,int,int);
	char *get_input(), *history[5];
	int history_start = -1, history_count = 0, history_global_count = 0;

	while (1){
		char *cargs[MAX_ARGS]; for(int i=0;i<MAX_ARGS;i++){cargs[i]=(char*)malloc(sizeof(char)*100);}
		
		printf("\033[32;1mMTL458:~$ \033[0m");

		char* inp = get_input();
		parse_input(inp,cargs);
		if (cargs[0]) update_history(inp,history,&history_start,&history_count,&history_global_count);

		// if (cargs[0]==NULL) {continue;}  // [NOT REQD] handle empty line as input

		int pid = fork();
		if (pid < 0){ // fork failed
			printf("Something went wrong.\n");
		}
		else if (pid == 0){ // fork
			//printf("I\'m the child (%d)\n", (int)getpid());	

			if (!strcmp(cargs[0],"history")) serve_history(history,history_start,history_count,history_global_count);
			else {
				int a = execvp(cargs[0],cargs);
				// printf("%s: command not found\n",cargs[0]);
				fprintf(stderr, "%s: command not found\n",cargs[0]);
				exit(1);
			}
		}
		else{ // parent
			wait(NULL); 
			//printf("I\'m the  parent(%d)\n", (int)getpid());
		}
	}

	return 0; 
}

char* get_input(){
	char *inp=(char*)malloc(sizeof(char)*MAX_CHARS);

	fgets(inp,MAX_CHARS,stdin);
	inp[strlen(inp)-1] ='\0';  // change the last \n to \0
	
	//scanf("%[^\n]%*c", inp);  // problems with empty string...

	return inp;
}

void parse_input(char *inp, char *cargs[]){	
	int w = 0,c = 0;
	
	// for(;*inp==' ';inp++);  // strip leading whitespace...
	// if (!*inp) {cargs[0]=NULL; return;} // handling empty line case seperately

	while (*inp){		  // != '\0'

		if (*inp==' '){
			inp++; c=0;
			if (strlen(cargs[w])>0) {w++;} //ignore if empty string
		}
		else {
			cargs[w][c++]=*inp++;
		}

	}
	if (cargs[w][0] == '\0') cargs[w]=NULL; else cargs[++w] = NULL;  // entry/exit with empty string test

}

void update_history(char* inp,char *history[],int* start,int* count,int* tot_count){
	// you can't get history w/o typing history...
	history[++*start%MAX_HIST] = inp;
	//if (*start >= MAX_HIST) *start%=MAX_HIST;
	if (*count < MAX_HIST) (*count)++;
	(*tot_count)++;
}

void serve_history(char *history[], int start, int count, int tot_count){
	for (int i=0; i<count; i++){
		printf("%5d  %s\n",tot_count-count+i+1,history[(start+i+1)%count]);
	}
}

		