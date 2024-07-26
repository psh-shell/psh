#include "psh.h"
#include <stdio.h>
char path_memory[PATH_MAX]="";
int last_command_up = 0;
char session_id[32];

char *history[PATH_MAX];
int history_count = 0;
int current_history = -1;

// Helper function to split the input line by ';'
char **split_commands(char *input) {
    size_t bufsize = 64;
    size_t position = 0;
    char **commands = malloc(bufsize * sizeof(char *));
    char *command_start = input;
    int in_single_quote = 0;
    int in_double_quote = 0;

    if (!commands) {
        fprintf(stderr, "psh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    for (char *c = input; *c != '\0'; c++) {
        // Handle quotes
        if (*c == '\'' && !in_double_quote) {
            in_single_quote = !in_single_quote;
        } else if (*c == '\"' && !in_single_quote) {
            in_double_quote = !in_double_quote;
        }
        
        // Handle end of command
        if (!in_single_quote && !in_double_quote && *c == ';') {
            commands[position] = malloc((c - command_start + 1) * sizeof(char));
            if (!commands[position]) {
                fprintf(stderr, "psh: allocation error\n");
                exit(EXIT_FAILURE);
            }
            strncpy(commands[position], command_start, c - command_start);
            commands[position][c - command_start] = '\0';
            position++;

            if (position >= bufsize) {
                bufsize += 64;
                commands = realloc(commands, bufsize * sizeof(char *));
                if (!commands) {
                    fprintf(stderr, "psh: allocation error\n");
                    exit(EXIT_FAILURE);
                }
            }

            command_start = c + 1;
        }
    }

    // Handle the last command
    if (command_start != input + strlen(input)) {
        commands[position] = strdup(command_start);
        if (!commands[position]) {
            fprintf(stderr, "psh: allocation error\n");
            exit(EXIT_FAILURE);
        }
        position++;
    }

    commands[position] = NULL;
    return commands;
}
char **PSH_TOKENIZER(char *line)
{
    size_t bufsize = 64, position = 0, i;
    char **token_arr = malloc(bufsize * sizeof(char *));
    char *token;
    int qstring = 0, has_quote = 0;
    const char *delimiters = " \t\n";
    char quote = '\0';

    if (!token_arr)
    {
        fprintf(stderr, "psh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, " ");
    while (token != NULL)
    {
        size_t len = strlen(token);
        for (i = 0; i < strlen(token); i++)
        {
            if ((has_quote == 0) && (token[i] == '"' || token[i] == '\''))
            {
                quote = token[i];
                has_quote = 1;
                break;
            }
            else if (token[i] == quote)
            {
                has_quote = 1;
                break;
            }
            else
            {
                has_quote = 0;
            }
        }

        if (has_quote == 1)
        {
            for (size_t j = i + 1; j < strlen(token); j++)
            {
                token[j - 1] = token[j];
            }
            token[strlen(token) - 1] = '\0';
            len = strlen(token);
        }

        if (qstring == 0)
        {
            token_arr[position] = malloc((len + 1) * sizeof(char));
            strcpy(token_arr[position], token);
            position++;
        }
        else
        {
            size_t prev_len = strlen(token_arr[position - 1]);
            token_arr[position - 1] = realloc(token_arr[position - 1], (prev_len + len + 2) * sizeof(char));
            if (token[len - 1] == quote)
            {
                token[len - 1] = '\0';
            }
            strcat(token_arr[position - 1], " ");
            strcat(token_arr[position - 1], token);
        }

        if (position >= bufsize)
        {
            bufsize += 64;
            token_arr = realloc(token_arr, bufsize * sizeof(char *));
            if (!token_arr)
            {
                fprintf(stderr, "psh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        if (has_quote == 1)
        {
            qstring = !(qstring);
        }
        token = strtok(NULL, delimiters);
    }
    token_arr[position] = NULL;
    return token_arr;
}

int PSH_EXEC_EXTERNAL(char **token_arr)
{
    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0)
    {
        if (execvp(token_arr[0], token_arr) == -1)
        {
            fprintf(stdout, "psh: No command found: %s\n", token_arr[0]);
        }
        exit(EXIT_FAILURE);
    }
    else if (pid < 0)
    {
        perror("psh error");
    }
    else
    {
        do
        {
            wpid = waitpid(pid, &status, WUNTRACED);
            if (wpid == -1)
            {
                perror("waitpid");
                exit(EXIT_FAILURE);
            }
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 1;
}

void handle_input(char **inputline, size_t *n, const char *PATH) {

    // printf("inputline is %s\n",*inputline);

    if (history_count == 0) {
        load_history();
    }

    print_prompt(PATH);

    *n = 0;
    if (*inputline != NULL) {
        free(*inputline);
        *inputline = NULL;
    }

    enableRawMode();

    char buffer[PATH_MAX] = {0};
    size_t pos = 0;
    size_t cursor = 0;
    current_history = -1;


    while (1) {
        if (kbhit()) {
            char ch = getchar();
            if (ch == '\033') { // ESC character
                getchar(); // skip the [
                ch = getchar();
                if (ch == ARROW_UP || ch == ARROW_DOWN) {
                    if (ch == ARROW_UP && current_history < history_count - 1) {
                        current_history++;
                    } else if (ch == ARROW_DOWN && current_history > -1) {
                        current_history--;
                    }
                    
                    if (current_history >= 0) {
                        strncpy(buffer, history[history_count - 1 - current_history], MAX_LINE_LENGTH - 1);
                        buffer[MAX_LINE_LENGTH - 1] = '\0';
                        pos = strlen(buffer);
                        cursor = pos;
                    } else {
                        buffer[0] = '\0';
                        pos = 0;
                        cursor = 0;
                    }
                    
                    printf("\r\033[K"); // Clear the current line
                    print_prompt(PATH);
                    printf("%s", buffer);
                    fflush(stdout);
                
                } else if (ch == ARROW_LEFT) {
                    if (cursor > 0) {
                        cursor--;
                        printf("\b");
                        fflush(stdout);
                    }
                } else if (ch == ARROW_RIGHT) {
                    if (cursor < pos) {
                        printf("%c", buffer[cursor]);
                        cursor++;
                        fflush(stdout);
                    }
                }
            } else if (ch == BACKSPACE) {
                if (cursor > 0) {
                    memmove(&buffer[cursor-1], &buffer[cursor], pos - cursor + 1);
                    pos--;
                    cursor--;
                    printf("\b\033[K%s", &buffer[cursor]);
                    for (size_t i = pos; i > cursor; i--) {
                        printf("\b");
                    }
                    fflush(stdout);
                }
            } else if (ch == '\n') {
                buffer[pos] = '\0';
                printf("\n");
                break;
            } else {
                if (pos < MAX_LINE_LENGTH - 1) {
                    memmove(&buffer[cursor+1], &buffer[cursor], pos - cursor + 1);
                    buffer[cursor] = ch;
                    pos++;
                    printf("%s", &buffer[cursor]);
                    cursor++;
                    for (size_t i = pos; i > cursor; i--) {
                        printf("\b");
                    }
                    fflush(stdout);
                }
            }
        }
    }
    disableRawMode();
    char *trimmed_input = trim_whitespace(buffer);
    *inputline = strdup(trimmed_input);
    if (*inputline == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    *n = strlen(*inputline);

    char *comment_pos = strchr(*inputline, '#');
    if (comment_pos) {
        *comment_pos = '\0';
    }

    if (strlen(*inputline) > 0) {
        if (history_count == PATH_MAX) {
            free(history[0]);
            for (int i = 1; i < PATH_MAX; i++) {
                history[i-1] = history[i];
            }
            history_count--;
        }
        history[history_count] = strdup(*inputline);
        if (history[history_count] == NULL) {
            perror("Memory allocation failed");
            exit(EXIT_FAILURE);
        }
        history_count++;
    }
}

void save_history(const char *inputline, const char* path_session)
{
    FILE *fp_memory, *fp_session;
    time_t timestamp;

    fp_memory = fopen(path_memory, "a");

    fp_session = fopen(path_session, "a");

    if (fp_memory == NULL || fp_session == NULL)
    {
        perror("Error:");
        if (fp_memory)
            fclose(fp_memory);
        if (fp_session)
            fclose(fp_session);
        exit(EXIT_FAILURE);
    }
    else
    {
        // timestamp = time(NULL);
        fprintf(fp_memory, "%s\n", inputline);
        fprintf(fp_session, "%s\n", inputline);
        fclose(fp_memory);
        fclose(fp_session);
    }
}

void process_commands(char *inputline, int *run){
    char **commands = split_commands(inputline);

    for (int i = 0; commands[i] != NULL; i++)
    {
        char **token_arr = PSH_TOKENIZER(commands[i]);

        if (token_arr[0] != NULL)
        {
            if(!strcmp(token_arr[0],"exit"))
            {
                free(inputline);
                free_double_pointer(commands);
                free_history();
            }

            // color_check(token_arr);
            
            execute_command(token_arr, run);
        }

        free_double_pointer(token_arr);
    }

    free_double_pointer(commands);
}

void execute_command(char **token_arr, int *run){
    HashMap *map = create_map(HASHMAP_SIZE);
    char ALIAS[PATH_MAX];
    char path_memory[PATH_MAX];
    if (!getcwd(path_memory, sizeof(path_memory)))
    {
        perror("Failed to get current Wworking directory\n");
        return ;
    }
    snprintf(ALIAS, sizeof(ALIAS), "%s/.files/ALIAS", path_memory);
    load_aliases(map, ALIAS);


    if (find(map, token_arr[0]))
    {
        replace_alias(map, token_arr);
    }
    free_map(map);
    if (strchr(token_arr[0], '='))
    {
        handle_env_variable(token_arr);
        return;
    }
    for (int j = 0; j < size_builtin_str; j++)
    {
        if (strcmp(token_arr[0], builtin_str[j]) == 0)
        {

            *run = (*builtin_func[j])(token_arr);
            return;
        }
    }


    if (!contains_wildcard(token_arr))
    {
        *run = PSH_EXEC_EXTERNAL(token_arr);
    }

    else
    {
        if (strchr(token_arr[0], '?') || strchr(token_arr[0], '?'))
        {
            fprintf(stdout, "psh: No command found: %s\n", token_arr[0]);
        }
        else
        {
            // func to handle wildcards
            handle_wildcard(token_arr[1]);
        }
    }
}

