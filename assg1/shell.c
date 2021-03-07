#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_ARGS  16
#define MAX_ARGL  1024
#define MAX_CHARS 1024
#define MAX_PATHL 1024
#define MAX_HIST  5


int main(int argc, char* argv[]) 
{
	void parse_input(char*,char*[],char*), update_history(char*,char*[],int*,int*,int*), serve_history(char*[],int,int,int);
	char *path_resolver(char*,char*,char[],int), home_path[MAX_PATHL], *_home_path = getcwd(home_path,sizeof(home_path)), cwd[MAX_PATHL+MAX_CHARS+2], *curr_path=strdup("~"), *history[MAX_HIST];
	int get_input(char*), history_start = -1, history_count = 0, history_global_count = 0;

	for(int i=0;i<MAX_HIST;i++) {history[i]=(char*)calloc(MAX_CHARS+2,sizeof(char));}
	while (1){
		char *cargs[MAX_ARGS]; for(int i=0;i<MAX_ARGS;i++){cargs[i]=(char*)calloc(MAX_PATHL+MAX_ARGL+2,sizeof(char));}
		char *inp=(char*)calloc(MAX_CHARS+2,sizeof(char));

		printf("\033[32;1mMTL458:%s$ \033[0m",curr_path);

		if (!get_input(inp)) continue;	// skip if empty input/limit exceeded
		parse_input(inp,cargs,_home_path);
		
		update_history(inp,history,&history_start,&history_count,&history_global_count); // this should come before any command handling!!
		if (cargs[0]==NULL) {continue;}  // handle empty line as input except "\n" (eg. "  \n")

		if (!strcmp(cargs[0],"cd")) { if(cargs[1]) curr_path=path_resolver(cargs[1],_home_path,cwd,sizeof(cwd)); continue;}

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
			}
			exit(0);
		}
		else{ // parent
			wait(NULL); 
			//printf("I\'m the  parent(%d)\n", (int)getpid());

			for(int i=0;i<MAX_ARGS;i++){free(cargs[i]);}
			free(inp);
		}
	}

	return 0; 
}

int get_input(char *inp){

	if (fgets(inp,MAX_CHARS+2,stdin)){
		size_t len = strlen(inp); 
		if (len == 0){	// empty input
			return 0;
		}
		if (*inp=='\n'){	// user just presses '\n'
			return 0;
		}
		if (inp[len-1] != '\n'){ // buffer overflow
			int c;
			while ((c = getchar()) != '\n' && c != EOF);	// flush buffer
			fprintf(stderr,"Error: Input limit exceeded (Allowed: %d characters)\n",MAX_CHARS);
			return 0;
		}
		inp[len-1] ='\0'; // change the last \n to \0
		return 1;
	}
	return 0;	
	
	//scanf("%[^\n]%*c", inp);  // problems with empty string...
}

void parse_input(char *inp, char *cargs[], char *start){
	// for(;*inp==' ';inp++);  // strip leading whitespace...
	// if (!*inp) {cargs[0]=NULL; return;} // handling empty line case seperately

	int w = 0, c = 0;
	while (*inp){		  // != '\0'

		if (*inp==' '){
			inp++; c=0;
			if (strlen(cargs[w])>0) {w++;} //ignore if empty string
		}
		else {
			if (*inp=='\"'){	// start of quote
				inp++;			// skip the starting quote
				while (*inp && *inp!='\"'){	// until *inp != '\0' and '\"'
					cargs[w][c++]=*inp++;	// insert to the cargs
				}
				if (*inp) inp++; // skip the ending quote
			}
			else {
				if (c==0 && *inp=='~') {	// starts with ~ 
					char *t = start;
					while((cargs[w][c++]=*t++));
					c--; inp++;
				}
				else cargs[w][c++]=*inp++;	
			}
		}

	}
	if (cargs[w][0] == '\0') cargs[w]=NULL; else cargs[++w] = NULL;  // entry/exit with empty string test

}

char* path_resolver(char *path,char *start, char buff[], int size_buff){
	// if (!strcmp(start,"/")) start= (char*) ""; // ?? handle start with "/"
	/****** handled in parse_input()
	char t[MAX_PATHL+MAX_CHARS+2];
	strcpy(t,path);
	if (*path && *path=='~') {
		strcpy(t,start); strcat(t,++path); 
	}
	path=t;
	*********/
	if (chdir(path)) { fprintf(stderr,"cd: %s: ",path); perror(""); }
	char *cwd = getcwd(buff,size_buff);
	// if (!*cwd) { } // handle this later...

	while (*start&&*cwd){ start++; cwd++; }
	if (!*start) { *--cwd='~'; return cwd; } // handle???
	return buff;
}

void update_history(char* inp,char *history[],int* start,int* count,int* tot_count){
	// you can't get history w/o typing history...
	++*start; *start%=MAX_HIST; // if count<MAX_HIST, start<MAX_HIST
	strcpy(history[*start],inp); // copy each char in inp to history
	
	//int i = 0;
	//while((history[*start][i++] = *inp++)); // copy each char in inp to history
	
	//if (*start >= MAX_HIST) *start%=MAX_HIST;
	if (*count < MAX_HIST) (*count)++;
	(*tot_count)++;
}

void serve_history(char *history[], int start, int count, int tot_count){
	for (int i=0; i<count; i++){
		printf("%5d  %s\n",tot_count-count+i+1,history[(start+i+1)%count]);
	}
}

		