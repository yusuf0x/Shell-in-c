#include <stdio.h>
#include "parser.h"
#include <sys/wait.h>
#include <stdlib.h> 
#include <unistd.h>
#include <errno.h>
#include <wait.h> 
#include <string.h> 
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>


#define format "[%d]+ Running \t %s"
// header for linked list that contain background process
bg_process * process_list = NULL;
pid_t last_bg_process;
pid_t shell_pgid;
//tline  containing the information about the parsed string.
tline * line;


int main(int argc, char const *argv[])
{
	 printf("           _       _ __ _          _ _ \n"); 
    printf(" _ __ ___ (_)_ __ (_) _\\ |__   ___| | |\n"); 
    printf("| '_ ` _ \\| | '_ \\| \\ \\| '_ \\ / _ \\ | |\n"); 
    printf("| | | | | | | | | | |\\ \\ | | |  __/ | |\n"); 
    printf("|_| |_| |_|_|_| |_|_\\__/_| |_|\\___|_|_|\n"); 
    printf("Author: yusuf0x\n");
	run();
	return 0;
}

void run(){
	char buffer[1024];
	int i;
	pid_t pid;
	int **pipelines;
	int pids;
	static pid_t current;

	// dup creates a copy of a file descriptor
	// fileno return the id of file descriptor
	int input = dup(fileno(stdin));
	int output = dup(fileno(stdout));
	int errorData = dup(fileno(stderr));


	//SIGINT == Ctrl+C and SIGQUIT == Ctrl + \
	// ignore them 
	//SIG_IGN specifies that the signal should be ignored
	signal(SIGINT , SIG_IGN);
	signal(SIGQUIT, SIG_IGN);

	int status;
	printf("ysh> ");

	while(fgets(buffer,1024,stdin)){
	
		// signal(SIGINT,SIG_IGN);
		// signal(SIGQUIT,SIG_IGN);

		line  = tokenize(buffer);

		if (line==NULL)
		{
			printf("No commands\n");
			continue;
		}
		if (line->redirect_input!=NULL)
		{
			Redirection(line->redirect_input,NULL,NULL);
		}
		if (line->redirect_output!=NULL)
		{
			Redirection(NULL,line->redirect_output,NULL);
		}
		if (line->redirect_error!=NULL)
		{
			Redirection(NULL,NULL,line->redirect_error);
		}
		// if (line->background)
		// {
		// 	printf("Run in background\n");
		// 	//return (1);
		// 	insert_process(&process_list,new_process(current,strdup(buffer)));

		// }
		

		// -----------------
		// int execvp (const char *file , char *const argv[]); 
		if (line->ncommands == 1 )
		{
			if (strcmp(line->commands[0].argv[0],"cd")==0)
			{
				add_to_history(line->commands[0]);
				cd();
				// printf("cd\n");
			}else if(strcmp(line->commands[0].argv[0],"exit")==0){
				exit(EXIT_SUCCESS);
			}
			else if(strcmp("jobs", line->commands[0].argv[0]) == 0){
				jobs(&process_list);
			}else if(strcmp("fg", line->commands[0].argv[0]) == 0){
				if (line->commands->argc > 1) {
					foreground(atoi(line->commands[0].argv[1]), &process_list);
				}
				else {
					foreground(last_bg_process, &process_list);
				}
			}
			else if(strcmp(line->commands[0].argv[0],"history")==0){
				add_to_history(line->commands[0]);
				history();
			}
			else {
				pid = fork();
				current = pid;
				if (pid < 0 )
				{
					fprintf(stderr, "Fork() error \n%s\n",strerror(errno));
				}else if(pid == 0)
				{
					// printf("%s\n",line->commands[0].filename);
					if (line->background)
					{
						printf("Run in background\n");
						//return (1);
						insert_process(&process_list,new_process(current,strdup(buffer)));

					}
					else if (line->commands[0].filename!=NULL)
					{
						execvp(line->commands[0].filename,line->commands[0].argv);
						fprintf(stderr, "Error executing command: %s\n", strerror(errno));
					}
					else{
						fprintf(stderr, "%s :command not found\n" , line->commands[0].argv[0]);
						// printf("error\n");
					}
				}else{
					wait (&status);
					if (WIFEXITED(status) != 0) //child exited normally 
						if (WEXITSTATUS(status) != 0) //return code when child exits
							printf("The command was not executed correctly\n");
				}
			}

		}
		else if(line->ncommands!=1){
			pipelines = (int**)malloc((line->ncommands-1)*sizeof(int*));
			for (int i = 0; i < line->ncommands; ++i)
			{
				pipelines[i] = (int*)malloc(2*sizeof(int));
				pipe(pipelines[i]);
				//The first element of the pipefd array, pipelines[i][0] is used for reading data from the pipe.
				//The second element of the pipefd array, pipelines[i][1] is used for writing data to the pipe.
			}
			for (int i = 0; i < line->ncommands; ++i)
			{
				signal(SIGINT , SIG_DFL);
				signal(SIGQUIT, SIG_DFL);
				if (line->commands[i].filename !=NULL)
				{
					pids = fork();
					if (pids < 0 )
					{
						fprintf(stderr, "Fork() error \n%s\n",strerror(errno));
						exit(1);
					}else if(pids == 0)
					{
						if(i==0){ // first child
							close(pipelines[i][0]);
							dup2(pipelines[i][1],1); // stdout 
							execvp(line->commands[i].argv[0],line->commands[i].argv);
							// all output will be in piplines[i][1];
							fprintf(stderr, "Error executing command: %s\n", strerror(errno));
							exit(1);
						}	
						else if((i==line->ncommands-1)){ 
							close(pipelines[i-1][1]);
							dup2(pipelines[i-1][0],0);
							execvp(line->commands[i].argv[0], line->commands[i].argv);

							fprintf(stderr, "Error executing command: %s\n", strerror(errno));
							exit(1);
						}else{	// rest of children			
							close(pipelines[i][0]);	
							close(pipelines[i-1][1]);	
							dup2(pipelines[i-1][0],0);	
							dup2(pipelines[i][1],1);

							execvp(line->commands[i].argv[0], line->commands[i].argv);	

							fprintf(stderr, "Error al ejecutar el comando: %s\n", strerror(errno));
							exit(1);
							
						}
					}else{
						if(!(i==(line->ncommands-1))){ 
							close(pipelines[i][1]);
						}
					}
				}else { //
					fprintf(stderr, "%s : command not found\n" , line->commands[i].argv[0]); // El comando no es válido, mostrarlo
				}	
			}
			// Wait for everything to finish, 0 means wait for any child process
			waitpid(pids,&status,0); 
			for(i=0;i<line->ncommands-1;i++){
				free(pipelines[i]);
			}
			free(pipelines);

		}

		if(line->redirect_input != NULL ){
			dup2(input , fileno(stdin));	
		}
		if(line->redirect_output != NULL ){
			dup2(output , fileno(stdout));	
		}
		if(line->redirect_error != NULL ){
			dup2(errorData, fileno(stderr));	
		}
		printf("ysh> ");

	}


}

pid_t debug_wait(pid_t pid, int options){
	//
	pid_t result;
	int status;
	options |= WUNTRACED;
	while (1) {
		result = waitpid(pid, &status, options);
		if (result == 0) {
			return result;
		}
		else{
					wait (&status);
					if (WIFEXITED(status) != 0) //child exited normally 
						if (WEXITSTATUS(status) != 0) //return code when child exits
							printf("The command was not executed correctly\n");
						return result;
		}
	}
}
void print_process(bg_process * process) {
	printf(format, process->id, process->command);
}
// insert to linked list a process 
void insert_process(bg_process ** process_list_ptr, bg_process * new_process) {
	bg_process * current = *process_list_ptr;
	if (current == NULL) {
		new_process->id = 0;
		*process_list_ptr = new_process;
		current = new_process;
	}
	else {
		current = *process_list_ptr;
		while (current->next != NULL) {
			current = current->next;
		}
		new_process->id = current->id + 1;
		current->next = new_process;
	}
	print_process(new_process);
}
// create new process 
bg_process * new_process(pid_t pid, char * command) {
	bg_process * process = (bg_process *)malloc(sizeof(bg_process *));
	process->pgid = pid;
	process->command = command;
	process->next = NULL;
	return process;
}

void foreground(int id, bg_process ** process_list_ptr) {
	pid_t pgid;
	bg_process * current;
	if (process_list_ptr != NULL) {
		current = *process_list_ptr;
		/* check that it is running in the backgroud */
		while (current != NULL && id > current->id) {
			current = current->next;
		}
		if (current != NULL && current->id == id) {
			pgid = current->pgid;
			free(current->command);
			free(current);
			tcsetpgrp(STDIN_FILENO, pgid);
			debug_wait(pgid, 0);
			tcsetpgrp(STDIN_FILENO, shell_pgid);
		}
	}
}

/* check if there is a finished task in the background and show the ones that haven't finished */
void jobs(bg_process ** process_list_ptr) {
	bg_process * current = NULL;
	bg_process * next = NULL;
	if (process_list_ptr != NULL) {
		while (*process_list_ptr != NULL && current == NULL) {
			// if task is finished free memory 
			if (debug_wait((*process_list_ptr)->pgid, WNOHANG) != 0) {
				current = (*process_list_ptr)->next;
				free((*process_list_ptr)->command);
				free(*process_list_ptr);
				*process_list_ptr = current;
				current = NULL;
			}
			else {
				current = *process_list_ptr;
			}
		}
		if (*process_list_ptr != NULL) {
			current = *process_list_ptr;
			while (current != NULL) {
				next = current->next;
				//if task finished free memory 
				if (debug_wait(current->pgid, WNOHANG) != 0) {
					free((current)->command);
					free(current);
				}
				else {
					print_process(current);
				}
				current = next;
			}
		}
	}
}

//- ---------------------------------------/
int Redirection ( char * input , char * output , char * error ){
	
	int file; // helper variable to redirect

	// If it is from input. Requirements to put to let you open and edit the document
	if(input != NULL ) {
		file = open (input , O_CREAT | O_RDONLY); 
		if(file == -1){
			fprintf( stderr , "%s : Error. %s\n" , input , strerror(errno)); // show error , -1 equals NULL
			return 1;
		} else { 
			dup2(file,fileno(stdin)); //Redirection to 0 stdin, standard input
		}	
	}
	
	// If it is from output
	if(output != NULL ) {
		//file = open(output, O_WRONLY | O_CREAT | O_TRUNC);
		file = creat (output ,  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH ); //
		if(file == -1){
			fprintf( stderr , "%s : Error. %s\n" , output , strerror(errno)); // 
			return 1;
		} else { 
			dup2(file,fileno(stdout));
		}	
	}
	
	// If it is from errorData
	if(error != NULL ) {
		file = creat (error ,  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
		if(file == -1){
			fprintf( stderr , "%s : Error. %s\n" , error , strerror(errno)); //
			return 1;
		} else { 
			dup2(file,fileno(stderr)); // 
		}	
	}
	
	return 0;
}


void add_to_history(tcommand command){

}
void history(){
	printf("----------- History ---------\n");
}

void cd(){
	char *dir;
	char buffer[512];
	if(line->commands[0].argc > 2) // can't make a cd to 2 directories
		{
		  fprintf(stderr,"Usage: %s directory\n", line->commands[0].argv[0]);
		}
	if (line->commands[0].argc == 1) // If it is 1 , nothing happens to me, that is, the name of the program.
	{
		dir = getenv("HOME");
		if(dir == NULL)
		{
			fprintf(stderr,"There is no $HOME variable\n");	
		}
	}else {
		dir = line->commands[0].argv[1];
	}
	if (chdir(dir) != 0) { // If it is not different from 0, the chdir makes it normal
		fprintf(stderr,"Error changing directory: %s\n", strerror(errno)); 
	}
	printf( "The current directory is: %s\n", getcwd(buffer,-1));


}
