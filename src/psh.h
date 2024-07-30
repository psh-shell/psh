// psh.h
#ifndef PSH_H
#define PSH_H

// Our Headers
#include "builtin.h"
#include "colors.h"

// C Headers
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <glob.h>
#include <time.h>
#include <stdbool.h>
#include <time.h>
#include <limits.h>
#include <fcntl.h>
#include <termios.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>


#define MAX_VARS 100
#define ARROW_UP 'A'
#define ARROW_DOWN 'B'
#define ARROW_LEFT 'D'
#define ARROW_RIGHT 'C'



#define MAX_VARS 100
#define PATH_MAX 4096
#define ARROW_UP 'A'
#define ARROW_DOWN 'B'
#define ARROW_LEFT 'D'
#define ARROW_RIGHT 'C'
#define MAX_LINE_LENGTH 1024
#define BACKSPACE 127
#define HASHMAP_SIZE 256


// Defining Structs to hold variables and functions
struct Variable
{
    char var_name[64];
    char var_value[256];
};
// struct Func              // function declarations
// {
//     char func_name[64];
//     char func_def[1024];
// };

typedef struct Alias 
{
    char *name;
    char *command;
    struct Alias *next; 
} Alias;

typedef struct HashMap
{
    Alias **buckets;
    int size;
} HashMap;

// extern char **token_arr;

// Global Variables
extern char cwd[PATH_MAX];
extern char *builtin_str[];
extern int (*builtin_func[])(char **);
extern int size_builtin_str;
extern char PREV_DIR[PATH_MAX];
extern char PATH[PATH_MAX];
extern char COPY_PATH_PSHRC[PATH_MAX];
extern char path_memory[PATH_MAX];
extern char session_id[32];
extern int last_command_up;
extern char path_memory[];
extern volatile int SIGNAL;
extern struct Variable global_vars[MAX_VARS]; // Global array to store variables
extern int num_vars; 
extern char *history[PATH_MAX];
extern int history_count;
extern int current_history;

// Function Declarations

// main.c functions
int PSH_SCRIPT(const char *);
int PSH_LOOP(void);
// int PSH_READ(void);      //now split up to be more modular

// execute.c functions
char **PSH_TOKENIZER(char *);
int PSH_EXEC_EXTERNAL(char **);
void handle_input(char **, size_t *, const char *);
void save_history(const char *, const char*);
void process_commands(char *, int *);
void execute_command(char **, int *);
int kbhit(void);

// helper functions
void free_double_pointer(char **array);
void remove_last_component(char *);
void get_last_path_component(const char *, char *);
void delete_file(const char *);
void read_lines(const char *, int, int);
void read_lines_wo_no(const char *, int, int);
int count_lines(const char *);
void read_lines_reverse(const char *, int, int);
void remove_line(const char *, size_t);
void clear_session_history(void);
void read_lines_reverse_wo_no(const char *, int, int);
char *expand_history(const char *, FILE *);
int compare_strings(const void *, const void *);
void sort_strings(char **, int);
char **split_commands(char *);
int size_token_arr(char **);
bool contains_wildcard(char **);
int handle_wildcard(char *);
char *find_closing_done(char *);
void process_nested_loops(char *, int *);
char *process_for_loop(char *, int *);
void get_last_line(char **);
unsigned int hash(const char *, int );
HashMap *create_map(int);
void delete_all_aliases(HashMap *);
void insert_alias(HashMap *, const char *, const char *);
void load_aliases(HashMap *, const char *);
void save_aliases(HashMap *, const char *);
void free_map(HashMap *);
void delete_alias(HashMap *, const char *);
Alias *find(HashMap *, const char *);
const char *get_alias_command(HashMap *, const char *);
char **split_strings(const char *);
char **replace_alias(HashMap *, char **);
void generate_session_id(void);
void initialize_paths(const char *);
void get_session_path(char *, size_t, const char *);
void initialize_shell(const char *);
void print_prompt(const char *);
void load_history(void);
void free_history(void);
void enableRawMode(void);
void disableRawMode(void);
char *trim_whitespace(char *);
void parse_ps1(const char *, const char *);
char *remove_quotes(char *);
char *expand_variables(char *);
void handle_env_variable(char *[]);
void get_alias_path(char *, size_t, const char *);
void sigint_handler(int sig);

#endif
