#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_ARGS  16
#define MAX_CHARS 1024


int main(int argc, char* argv[]) 
{
	void get_input_and_cook(char*[]);
	while (1){
		char *cargs[MAX_ARGS]; for(int i=0;i<MAX_ARGS;i++){cargs[i]=(char*)malloc(sizeof(char)*100);}
		
		printf("\033[32;1mMTL458:~$ \033[0m");
		get_input_and_cook(cargs);		

		int pid = fork();
		if (pid < 0){
			printf("Fork failed!");
		}
		else if (pid == 0){ // fork
			//printf("I\'m the child (%d)\n", (int)getpid());		
			int a = execvp(cargs[0],cargs);
			// printf("%s: command not found\n",cargs[0]);
			fprintf(stderr, "%s: command not found\n",cargs[0]);
			exit(1);
		}
		else{ // parent
			wait(NULL); 
			//printf("I\'m the  parent(%d)\n", (int)getpid());
		}
	}

	return 0; 
}

void get_input_and_cook(char *cargs[]){	
	char *inp=(char*)malloc(sizeof(char)*MAX_CHARS); 
	int w = 0,c = 0;

	//fgets(inp,100,stdin);
	//inp[strlen(inp)-1] ='\0';
	scanf("%[^\n]%*c", inp);

	while (*inp){		  // != '\0'

		if (*inp==' '){
			inp++; c=0;
			if (strlen(cargs[w])>0) {w++;} //ignore if empty string
		}
		else {
			cargs[w][c++]=*inp++;
		}
		
	}
	cargs[++w] = NULL;
	
}

		