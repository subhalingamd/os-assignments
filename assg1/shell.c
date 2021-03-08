/****************************************************************
Program:	A simple shell in C
Author: 	Subhalingam D
****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_CHARS 1024					// max num of characters in the input command
#define MAX_ARGS  ((int)MAX_CHARS/2)+2	// max space seperated arguments (words) in command [worst is (MAX_CHARS/2+1)+1]
#define MAX_ARGL  MAX_CHARS				// max length of an argument in command [worst is MAX_CHARS]
#define MAX_PATHL 1024					// max length of absolute path of starting directory (HOME)
#define MAX_HIST  5						// max number of inputs to keep a track of (size of history)


int main(int argc, char* argv[]) 
{
	// variables and local functions declarations
	void parse_input(char*,char*[],char*), update_history(char*,char*[],int*,int*,int*), serve_history(char*[],int,int,int);
	char *path_resolver(char*,char*,char[],int), home_path[MAX_PATHL], *_home_path = getcwd(home_path,sizeof(home_path)), cwd[MAX_PATHL+MAX_CHARS+2], *curr_path=strdup("~"), *history[MAX_HIST]; // (+2) used in size to account for the extra '\n' and '\0' in string input
	int get_input(char*), history_start = -1, history_count = 0, history_global_count = 0;

	// allocate memory for history and initialise with '\0'
	for(int i=0;i<MAX_HIST;i++) {history[i]=(char*)calloc(MAX_CHARS+2,sizeof(char));}
	
	while (1){
		// declare variables for user input and args for execvp() and initialise them
		char *cargs[MAX_ARGS]; for(int i=0;i<MAX_ARGS;i++){cargs[i]=(char*)calloc(MAX_PATHL+MAX_ARGL+2,sizeof(char));}
		char *inp=(char*)calloc(MAX_CHARS+2,sizeof(char));

		// curr_path stores the current working directory path to display on shell
		printf("\033[32;1mMTL458:%s$ \033[0m",curr_path);

		if (!get_input(inp)) continue;		// skip if empty input/limit exceeded
		parse_input(inp,cargs,_home_path);	// parse the given input to prepare for execvp()
		
		update_history(inp,history,&history_start,&history_count,&history_global_count); // this should come before any command handling!!
		if (cargs[0]==NULL) {continue;}  // handle empty line as input except "\n" (eg. "  \n")

		// serve request for cd
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
	/*
	Description:	Get input from stdin
	Input:
		- inp: stores the input
	Returns:
		- Returns 1 if the input is valid or 0 otherwise
	Notes:
		- An input is considered invalid/unnecessary if it is empty, has got only '\n' or the input length exceeded the maximum limit MAX_CHARS (excluding the '\n' and '\0')
	*/
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
	/*
	Description:	Parse the user input to serve it to execvp()
	Input:
		- inp: the user input
		- cargs: the array of strings in the format expected by execvp()
		- start: the path of the directory from where the shell was started (also referred to as HOME)
	Returns:
	Notes:
	*/

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
					while((cargs[w][c++]=*t++));	// replace ~ with HOME (starting directory path)
					c--; inp++;		// the last character copied from start would be '\0', so go back one step (c--) <and> go to next char in input (inp++)
				}
				else cargs[w][c++]=*inp++;	// just copy the character otherwise
			}
		}

	}
	if (cargs[w][0] == '\0') cargs[w]=NULL; else cargs[++w] = NULL;  // execvp() expects the last entry to be NULL

}

char* path_resolver(char *path,char *start, char buff[], int size_buff){
	/*
	Description:	Serve cd command
	Input:
		- inp: the user input
		- start: the path of the directory from where the shell was started (also referred to as HOME)
		- buff: initially empty, contains the path to be displayed on shell
		- size_buff: size allocated for the buffer
	Returns:
		- a pointer to buff
	Notes:
		- along with changing the current working directory (cwd), the path that has to be displayed in the shell (with changes to ~) is modified in this function
		- only the first (at position 0) ~ is replaced with home path (like in shell)
			- e.g.  cd ~/~  : would be translated to  cd /path/to/home/~
			- e.g.  cd "~"  : would reamin  cd "~"  as the ~ is inside double quotes
	*/

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

	while (*start&&*cwd&&*start==*cwd){ start++; cwd++; }
		// To resolve ~ ::
		// while loop breaks if: \0 encountered in start or cwd or *cwd!=*start..
		// if start has ended (ie \0 reached) and [ cwd has also reached end (\0) (=> ~) or cwd has / (=> ~/...) ]
		// (otherwise) if start has not ended (=> out of ~) or start has ended but cwd has some char other than \0 and / (=> starting is okay... but the ending folder name in cwd is prefix of last dir in start --> so not under ~) ==> show the abs path
	if (!*start&&(!*cwd||*cwd=='/')) { *--cwd='~'; return cwd; } // *(cwd-1) should exist as cwd is non-empty and starts with '/' (which is common for all paths)
	return buff;
}

void update_history(char* inp,char *history[],int* start,int* count,int* tot_count){
	/*
	Description:	Update history for each valid input (inc history command)
	Input:
		- inp: the user input
		- history: array that stores the history
		- start: the position at which the previous command was inserted in the history array
		- count: max(the number of commands entered till now, MAX_HIST) - used for serving history
		- tot_count: total number of inputs by user (this number is used to display the serial number of the previous inputs when history command is used)
	Returns:
	Notes:
		- the history array is implemented in a circular fashion to optimise the space consumed
			- the size of the array always remains as MAX_HIST
			- the new input is inserted at (prev_index+1)%MAX_HIST starting from 0
			- so the inputs are inserted in the order -> 0,1,2,3,...,MAX_HIST-1,0,1,2,3,...
		- the start, count, tot_count are passed by reference
		- everything except "\n" as input is part of the history (incl incorrect commands, combinations of spaces like "  \n")
			- in this assignemnt, if the input, excluding the last "\n\0", exceeds MAX_CHARS, then such input is also excluded from the history
		- the input "history" to access the history is also added to history array (as seen in a Linux shell)
	*/

	// you can't get history w/o typing history...
	++*start; *start%=MAX_HIST; // if count<MAX_HIST, start<MAX_HIST
	strcpy(history[*start],inp); // copy each char in inp to history
	
	/// same thing below was achieved using strcpy()
	//int i = 0;
	//while((history[*start][i++] = *inp++)); // copy each char in inp to history
	
	//if (*start >= MAX_HIST) *start%=MAX_HIST;
	if (*count < MAX_HIST) (*count)++;
	(*tot_count)++;
}

void serve_history(char *history[], int start, int count, int tot_count){
	/*
	Description:	Print history when requested
	Input:
		- history: array that stores the history
		- start: the position at which the most recent command was inserted in the history array
		- count: max(the number of commands entered till now, MAX_HIST) - used for getting the indices of the history array
		- tot_count: total number of inputs by user (this number is used to display the serial number of the previous inputs)
	Returns:
	Notes:
		- the history array is implemented in a circular fashion to optimise the space consumed (as mentioned in update_history())
			- the most recent input will be stored in start
			- (start+1)%count will have the desired oldest input (MAX_HIST-th from bottom)
			- the order indices for listing the history will be -> (start+1)%count, (start+2)%count, ..., start%count
			- the serial number for the input can be computed accordingly
	*/
	for (int i=0; i<count; i++){
		printf("%5d  %s\n",tot_count-count+i+1,history[(start+i+1)%count]);
	}
}

		