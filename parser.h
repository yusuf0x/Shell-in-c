#include <sys/types.h>
typedef struct {
	char * filename;
	int argc;
	char ** argv;
} tcommand;

typedef struct {
	int ncommands;
	tcommand * commands;
	char * redirect_input;
	char * redirect_output;
	char * redirect_error;
	int background;
} tline;

extern tline * tokenize(char *str);

/* we store the information associated with the processes created by the system */
typedef struct bg_process {
	int id;
	pid_t pgid;
	char * command;
	struct bg_process * next;
} bg_process;

// for history 
typedef struct history_command history_command;
struct history_command{
	tcommand command;
	history_command *next;
} ;




pid_t debug_wait(pid_t pid, int options);
void print_process(bg_process * process);
void insert_process(bg_process ** process_list_ptr, bg_process * new_process);
bg_process * new_process(pid_t pid, char * command) ;
void foreground(int id, bg_process ** process_list_ptr);
void jobs(bg_process ** process_list_ptr);



// prototype 
int Redirection ( char * input , char * output , char * error );
void cd ();
void run();


void add_to_history(tcommand command);
history_command * new_command(tcommand command);
void history();

