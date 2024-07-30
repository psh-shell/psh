#include "psh.h"
// variables

//char cwd[PATH_MAX];
char *builtin_str[] = {"exit", "cd", "echo", "pwd", "fc", "export", "for", "type","read", "alias", "unalias"};
int (*builtin_func[])(char **) = {&PSH_EXIT, &PSH_CD, &PSH_ECHO, &PSH_PWD, &PSH_FC, &PSH_EXPORT, &PSH_FOR, &PSH_TYPE, &PSH_READ_SHELL, &PSH_ALIAS, &PSH_UNALIAS};

int size_builtin_str = sizeof(builtin_str) / sizeof(builtin_str[0]);
struct Variable global_vars[MAX_VARS];
int num_vars = 0;
char PATH[PATH_MAX];

int PSH_EXIT(char **token_arr)
{
    char path_session[PATH_MAX];
    get_session_path(path_session, sizeof(path_session), cwd);

    if (!token_arr[1])
    {
        printf("bye bye PSH :D\n");
        delete_file(path_session);
        free_double_pointer(token_arr); // now handled in process commands
        exit(0);
    }
    printf("bye bye PSH :D\n");
    int exit_code = atoi(token_arr[1]);
    free_double_pointer(token_arr);
    delete_file(path_session);
    exit(exit_code);
}

int PSH_CD(char **token_arr)
{
    char *localdir = malloc(PATH_MAX);
    char *home = NULL;                   // for ~ and empty cases
    static char PREV_DIR[PATH_MAX] = ""; // for - cases
    char *pathtoken = malloc(PATH_MAX);

    strcpy(localdir, PATH);
    if (localdir == NULL)
    {
        perror("PSH: malloc() error");
        return 1;
    }
    if (token_arr[1] != NULL)
    {
        if ((strcmp(token_arr[1], "-L") == 0 || strcmp(token_arr[1], "-P") == 0))
        {
            if (token_arr[2] != NULL)
                strcpy(pathtoken, token_arr[2]);
            else
                pathtoken = NULL;
        }
        else
        {
            if (token_arr[1] != NULL)
                strcpy(pathtoken, token_arr[1]);
        }
    }
    else
    {
        pathtoken = NULL;
    }

    if (pathtoken == NULL || strcmp(pathtoken, "~") == 0)
    {
        home = getenv("HOME");
        if (home == NULL)
        {
            fprintf(stderr, "PSH: HOME environment variable not set\n");
            free(localdir);
            return 1;
        }
        strncpy(localdir, home, PATH_MAX - 1);
        localdir[PATH_MAX - 1] = '\0';
    }
    else if (strcmp(pathtoken, "-") == 0)
    {
        if (PREV_DIR[0] == '\0')
        {
            fprintf(stderr, "PSH: No previous directory\n");
            free(localdir);
            return 1;
        }
        strncpy(localdir, PREV_DIR, PATH_MAX - 1);
        localdir[PATH_MAX - 1] = '\0';
    }
    else if (strcmp(pathtoken, "..") == 0)
    {
        if (strcmp(localdir, "/") == 0) // For path inside / or root
        {
            strcpy(localdir, "/");
        }
        else
        {
            remove_last_component(localdir);
            if (!strcmp(localdir, ""))
            {
                strcpy(localdir, "/");
            }
        }
    }
    else if (strcmp(pathtoken, "./") == 0)
    {
        if (strcmp(localdir, "/") == 0) // For path inside / or root
        {
            strcpy(localdir, "/");
        }
        else
        {
            if (!strcmp(localdir, ""))
            {
                strcpy(localdir, "/");
            }
        }
    }

    else
    {
        if (pathtoken[0] != '/')
        {
            if (strcmp(PATH, "/") == 0)
                snprintf(localdir, PATH_MAX, "/%s", pathtoken);
            else
                snprintf(localdir, PATH_MAX, "%s/%s", PATH, pathtoken);
        }
        else
        {
            strncpy(localdir, pathtoken, PATH_MAX - 1);
            localdir[PATH_MAX - 1] = '\0';
        }
    }
    char *rpath = malloc(PATH_MAX);
    realpath(localdir, rpath);
    if (chdir(rpath) == -1)
    {
        // perror("PSH: chdir() error");
        fprintf(stderr, "PSH:No such Directory \n");
        free(localdir);
        free(rpath);
        free(pathtoken);
        return 1;
    }
    int lastindex = strlen(localdir);
    if (localdir[lastindex - 1] == '/' && strlen(localdir) != 1)
    {
        localdir[lastindex - 1] = '\0';
    }
    strncpy(PREV_DIR, PATH, PATH_MAX - 1);
    PREV_DIR[PATH_MAX - 1] = '\0';
    if (token_arr[1] != NULL && strcmp(token_arr[1], "-P") == 0)
        strcpy(PATH, rpath);
    else
        strcpy(PATH, localdir);
    free(localdir);
    free(rpath);
    free(pathtoken);
    return 1;
}

int PSH_ECHO(char **token_arr)
{
    bool newline = true;
    bool interpret_escapes = false;
    FILE *output = stdout;
    int arg_index = 1;

    for (; token_arr[arg_index] != NULL; arg_index++)
    {
        if (token_arr[arg_index][0] != '-')
        {
            break;
        }
        for (int i = 1; token_arr[arg_index][i] != '\0'; i++)
        {
            switch (token_arr[arg_index][i])
            {
            case 'n':
                newline = false;
                break;
            case 'e':
                interpret_escapes = true;
                break;
            case 'E':
                interpret_escapes = false;
                break;
            default:
                break;
            }
        }
    }

    char **output_tokens = token_arr;
    int output_token_count = 0;
    while (output_tokens[output_token_count] != NULL)
    {
        if (strcmp(output_tokens[output_token_count], ">") == 0)
        {
            if (output_tokens[output_token_count + 1] != NULL)
            {
                output = fopen(output_tokens[output_token_count + 1], "w");
                if (output == NULL)
                {
                    perror("Error opening file");
                    return 1;
                }
                output_tokens[output_token_count] = NULL;
            }
            break;
        }
        output_token_count++;
    }

    for (int i = arg_index; token_arr[i] != NULL; i++)
    {
        char *arg = token_arr[i];

        // Remove all quotes from the argument
        char *src = arg;
        char *dst = arg;
        while (*src)
        {
            if (*src != '\'' && *src != '"')
            {
                *dst++ = *src;
            }
            src++;
        }
        *dst = '\0';

        if (interpret_escapes)
        {
            char expanded_arg[1024];
            char *exp_write_pos = expanded_arg;

            for (char *read_pos = arg; *read_pos != '\0'; read_pos++)
            {
                if (*read_pos == '\\' && *(read_pos + 1) != '\0')
                {
                    switch (*(++read_pos))
                    {
                    case 'n':
                        *exp_write_pos++ = '\n';
                        break;
                    case 't':
                        *exp_write_pos++ = '\t';
                        break;
                    case '\\':
                        *exp_write_pos++ = '\\';
                        break;
                    default:
                        *exp_write_pos++ = '\\';
                        *exp_write_pos++ = *read_pos;
                        break;
                    }
                }
                else
                {
                    *exp_write_pos++ = *read_pos;
                }
            }
            *exp_write_pos = '\0';
            strcpy(arg, expanded_arg);
        }

        // Variable expansion
        if (arg[0] == '$')
        {
            const char *var_name = arg + 1; // Skip the '$' sign
            const char *var_value = getenv(var_name);
            
            if (var_value != NULL)
            {
                // mallocing so that huge variables get enough space and seg fault gets avoided
                size_t len = strlen(var_value);
                char *buffer = (char *)malloc(len+1);
                strcpy(buffer, var_value);
                strcpy(arg, buffer);
                // printf("%s", buffer);
                free(buffer);
            }
            else
            {
                // Variable not found, handle accordingly (e.g., print nothing)
                arg[0] = '\0'; // Empty string
            }
        }

        if (i > arg_index)
        {
            printf(" ");
        }
        fputs(arg, output);
    }

    if (newline)
    {
        fputc('\n', output);
    }

    if (output != stdout)
    {
        fclose(output);
    }

    return 1;
}

int PSH_PWD(char **token_arr)
{

    // printf("%s\n", token_arr[1]); //debugging
    // printf("Printing current working directory\n"); //debugging
    char *buffer = malloc(PATH_MAX);
    char rpath[1024];
    // printf("%d\n", PATH_MAX);

    if (token_arr[1] == NULL ||
        strcmp(token_arr[1], "-L") == 0) // Default pwd and pwd -L
    {
        // Write better check, statement always evaluates to true.
        if (true) // if PATH exists
        {
            printf("%s\n", PATH);
        }
        else
        {
            perror("PSH: PATH error");
        }
    }
    else if ((strcmp(token_arr[1], "-P")) == 0)
    {
        if (realpath(PATH, rpath) != NULL) // pwd -P
        {
            printf("%s\n", rpath);
        }
        else
        {
            perror("PSH: realpath() error");
        }
    }
    else
    {
        fprintf(stderr, "Unknown option: %s\n", token_arr[1]);
    }
    free(buffer);
    return 1;
}

int PSH_FC(char **token_arr)
{
    int n = -1;

    char MEMORY_HISTORY_FILE[PATH_MAX];
    char SESSION_HISTORY_FILE[PATH_MAX];

    // char path_memory[PATH_MAX];
    // strcpy(path_memory, cwd);
    // strcat(path_memory, "/.files/MEMORY_HISTORY_FILE");
    strcpy(MEMORY_HISTORY_FILE, path_memory);

    // char path_session[PATH_MAX];
    // strcpy(path_session, cwd);
    // strcat(path_session, "/.files/SESSION_HISTORY_FILE");
    // strcpy(SESSION_HISTORY_FILE, path_session);

    get_session_path(SESSION_HISTORY_FILE, sizeof(SESSION_HISTORY_FILE), cwd);

    if (token_arr[1] == NULL)
    {
        n = 0;
    }
    else if ((strcmp(token_arr[1], "-l") == 0) &&
             (token_arr[2] == NULL ||
              (strcmp(token_arr[2], "-n") != 0 &&
               strcmp(token_arr[2], "-r") != 0)))
    { // fc -l 5 10
        n = 1;
    }
    else if (((strcmp(token_arr[1], "-ln")) == 0) ||
             ((strcmp(token_arr[1], "-nl")) == 0) ||
             (((strcmp(token_arr[1], "-l")) == 0) &&
              ((strcmp(token_arr[2], "-n")) == 0)) ||
             (((strcmp(token_arr[1], "-n")) == 0) &&
              ((strcmp(token_arr[2], "-l")) ==
               0)))
    { // fc -ln, -nl, -l -n, -n -l
        n = 2;
    }
    else if (((strcmp(token_arr[1], "-lr")) == 0) ||
             ((strcmp(token_arr[1], "-rl")) == 0) ||
             (((strcmp(token_arr[1], "-l")) == 0) &&
              ((strcmp(token_arr[2], "-r")) == 0)) ||
             (((strcmp(token_arr[1], "-r")) == 0) &&
              ((strcmp(token_arr[2], "-l")) ==
               0)))
    { // fc -lr, -rl, -l -r, -r -l
        n = 3;
    }
    else if ((strcmp(token_arr[1], "-e")) == 0)
    {
        n = 4;
    }
    else if ((strcmp(token_arr[1], "-s")) == 0)
    {
        n = 5;
    }
    else if ((strcmp(token_arr[1], "-d")) == 0)
    {
        n = 6;
    }
    else if ((strcmp(token_arr[1], "-c")) == 0)
    {
        n = 7;
    }
    else if ((strcmp(token_arr[1], "-p")) == 0)
    {
        n = 8;

        // } else if ((((strcmp(token_arr[1], "-l")) == 0) &&
        //             ((strcmp(token_arr[2], "-r")) == 0) &&
        //             ((strcmp(token_arr[3], "-n")) == 0)) || // -l -r -n

        //            (((strcmp(token_arr[1], "-r")) == 0) &&
        //             ((strcmp(token_arr[2], "-n")) == 0) &&
        //             ((strcmp(token_arr[3], "-l")) == 0)) || // -r- -n -l

        //            (((strcmp(token_arr[1], "-n")) == 0) &&
        //             ((strcmp(token_arr[2], "-l")) == 0) &&
        //             ((strcmp(token_arr[3], "-r")) == 0)) || // -n -l -r

        //            (((strcmp(token_arr[1], "-n")) == 0) &&
        //             ((strcmp(token_arr[2], "-r")) == 0) &&
        //             ((strcmp(token_arr[3], "-l")) == 0)) || // -n -r -l

        //            (((strcmp(token_arr[1], "-r")) == 0) &&
        //             ((strcmp(token_arr[2], "-l")) == 0) &&
        //             ((strcmp(token_arr[3], "-n")) == 0)) || // -r -l -n

        //            (((strcmp(token_arr[1], "-l")) == 0) &&
        //             ((strcmp(token_arr[2], "-n")) == 0) &&
        //             ((strcmp(token_arr[3], "-r")) == 0))) { // -l -n -r
        //   n = 9;
    }
    else if (((strcmp(token_arr[1], "-lnr")) == 0) ||
             ((strcmp(token_arr[1], "-nrl")) == 0) ||
             ((strcmp(token_arr[1], "-rln")) == 0) ||
             ((strcmp(token_arr[1], "-rnl")) == 0) ||
             ((strcmp(token_arr[1], "-nlr")) == 0) ||
             ((strcmp(token_arr[1], "-lrn")) == 0))
    {
        n = 10;
    }
    else
    {
        printf("psh: fc: missing history argument\n");
        return 1;
    }
    switch (n)
    {

    case 0:
    {
        printf("psh: fc: missing history argument\n");
        break;
    }

    case 1:
    { // fc -l 20 50
        if (token_arr[2] == NULL)
        {
            int total_lines = count_lines(MEMORY_HISTORY_FILE);
            read_lines(MEMORY_HISTORY_FILE, 1, total_lines);
        }
        else if (token_arr[3] == NULL)
        { // fc -l 5 10
            int total_lines = count_lines(MEMORY_HISTORY_FILE);
            read_lines(MEMORY_HISTORY_FILE, atoi(token_arr[2]), total_lines);
        }
        else
        {
            read_lines(MEMORY_HISTORY_FILE, atoi(token_arr[2]), atoi(token_arr[3]));
        }
        break;
    }

    case 2:
    { // fc -l -n 20 50 (works)      fc -ln 20 50 (works)
        if (token_arr[2] == NULL)
        {
            int total_lines = count_lines(MEMORY_HISTORY_FILE);
            read_lines_wo_no(MEMORY_HISTORY_FILE, 1, total_lines);
        }

        else if (token_arr[2] != NULL && token_arr[3] == NULL)
        {
            int total_lines = count_lines(MEMORY_HISTORY_FILE);
            read_lines_wo_no(MEMORY_HISTORY_FILE, atoi(token_arr[2]), total_lines);
        }
        else if (token_arr[4] == NULL)
        {
            int total_lines = count_lines(MEMORY_HISTORY_FILE);
            read_lines_wo_no(MEMORY_HISTORY_FILE, atoi(token_arr[3]), total_lines);
        }
        else if (token_arr[3] != NULL && token_arr[4] != NULL)
        {
            read_lines_wo_no(MEMORY_HISTORY_FILE, atoi(token_arr[3]),
                             atoi(token_arr[4]));
        }
        else if (token_arr[2] != NULL && token_arr[3] != NULL &&
                 token_arr[4] == NULL)
        {
            read_lines_wo_no(MEMORY_HISTORY_FILE, atoi(token_arr[3]),
                             atoi(token_arr[4]));
        }

        break;
    }

    case 3:
    { // fc -l -r 20 50 (works)      fc -lr 20 50 (works)
        if (token_arr[2] == NULL)
        {
            int total_lines = count_lines(MEMORY_HISTORY_FILE);
            read_lines_reverse(MEMORY_HISTORY_FILE, 1, total_lines);
        }

        else if (token_arr[2] != NULL && token_arr[3] == NULL)
        {
            int total_lines = count_lines(MEMORY_HISTORY_FILE);
            read_lines_reverse(MEMORY_HISTORY_FILE, atoi(token_arr[2]), total_lines);
        }
        else if (token_arr[4] == NULL)
        {
            int total_lines = count_lines(MEMORY_HISTORY_FILE);
            read_lines_reverse(MEMORY_HISTORY_FILE, atoi(token_arr[3]), total_lines);
        }
        else
        {
            read_lines_reverse(MEMORY_HISTORY_FILE, atoi(token_arr[3]),
                               atoi(token_arr[4]));
        }

        break;
    }

    case 4:
    { // fc -e
        if (token_arr[2] == NULL)
        {
            printf("psh: fc: -e requires an editor name\n");
            return 1;
        }

        const char *editor = token_arr[2];
        const char *temp_file = "/tmp/psh_fc_edit.tmp";
        char command[1024];
        int start, end;

        // Determining range of commands to edit
        if (token_arr[3] == NULL)
        {
            // Edit last command by default
            int total_lines = count_lines(MEMORY_HISTORY_FILE);
            start = end = total_lines;
        }
        else if (token_arr[4] == NULL)
        {
            // Editing single specified command
            start = end = atoi(token_arr[3]);
        }
        else
        {
            // Editing range of commands
            start = atoi(token_arr[3]);
            end = atoi(token_arr[4]);
        }

        // Writing a specified range to temporary file
        FILE *temp = fopen(temp_file, "w");
        if (temp == NULL)
        {
            perror("Error creating temporary file");
            return 1;
        }
        read_lines_wo_no(MEMORY_HISTORY_FILE, start, end);
        fclose(temp);

        // Opening editor with temporary file
        snprintf(command, sizeof(command), "%s %s", editor, temp_file);
        system(command);

        // Reading edited commands from temporary file
        FILE *edited = fopen(temp_file, "r");
        if (edited == NULL)
        {
            perror("Error opening edited file");
            return 1;
        }

        char *line = NULL;
        size_t len = 0;
        while (getline(&line, &len, edited) != -1)
        {
            // Removing newline character
            line[strcspn(line, "\n")] = 0;

            // Executing the edited command
            system(line);

            // Adding the executed command to history
            FILE *fp = fopen(MEMORY_HISTORY_FILE, "a");
            if (fp != NULL)
            {
                fprintf(fp, "%s\n", line);
                fclose(fp);
            }
        }

        fclose(edited);
        if (line)
        {
            free(line);
        }

        // Clean up temporary file
        remove(temp_file);

        break;
    }

    case 5:
    { // fc -s
        char *old_word = NULL;
        char *new_word = NULL;
        int line_number = -1;
        int total_lines = count_lines(MEMORY_HISTORY_FILE);

        // Parse arguments
        if (token_arr[2] != NULL)
        {
            if (strchr(token_arr[2], '=') != NULL)
            {
                // String replacement specified
                old_word = strtok(token_arr[2], "=");
                new_word = strtok(NULL, "=");
            }
            else
            {
                // Command number specified
                line_number = atoi(token_arr[2]);
            }
        }

        if (token_arr[3] != NULL &&
            strchr(token_arr[3], '=') != NULL)
        { // Needs to be checked with echo
            old_word = strtok(token_arr[3], "=");
            new_word = strtok(NULL, "=");
        }

        // If no command number specified, use the last command
        if (line_number == -1)
        {
            line_number = total_lines;
        }

        // Reading the specified command from history
        FILE *history = fopen(path_memory, "r");
        if (history == NULL)
        {
            perror("Error opening history file");
            return 1;
        }

        char *line = NULL;
        size_t len = 0;
        ssize_t read;
        int current_line = 0;

        while ((read = getline(&line, &len, history)) != -1)
        {
            current_line++;
            if (current_line == line_number)
            {
                // Removing newline character
                line[strcspn(line, "\n")] = 0;
                break;
            }
        }

        fclose(history);

        if (current_line != line_number)
        {
            printf("fc: no command found\n");
            free(line);
            return 1;
        }

        // Performing string replacement if specified
        if (old_word != NULL && new_word != NULL)
        {
            char *replaced_line = malloc(len * 2); // Allocate more space for potential expansion
            if (replaced_line == NULL)
            {
                perror("Memory allocation error");
                free(line);
                return 1;
            }

            char *pos = strstr(line, old_word);
            if (pos != NULL)
            {
                // Performing the replacement
                strncpy(replaced_line, line, pos - line);
                replaced_line[pos - line] = '\0';
                strcat(replaced_line, new_word);
                strcat(replaced_line, pos + strlen(old_word));

                free(line);
                line = replaced_line;
            }
            else
            {
                free(replaced_line);
            }
        }

        // Displaying the command
        printf("%s\n", line);

        // Executing the command
        int result = system(line);

        // Adding the executed command to history
        history = fopen(MEMORY_HISTORY_FILE, "a");
        if (history != NULL)
        {
            fprintf(history, "%s\n", line);
            fclose(history);
        }

        free(line);
        return result;
    }

    case 6: // -d option
    {
        FILE *fp1 = fopen(MEMORY_HISTORY_FILE, "r");

        if (fp1 == NULL)
        {
            perror("Error opening history files");
            if (fp1)
                fclose(fp1);
            return -1;
        }

        if (token_arr[2] == NULL)
        {
            printf("psh: history -d requires a line number\n");
            fclose(fp1);
            return 1;
        }

        // Counting total lines in global history
        int total_lines = count_lines(MEMORY_HISTORY_FILE);
        // printf("Total lines %d\n", total_lines);

        // Parsing the offset
        long offset = strtol(token_arr[2], NULL, 10);
        size_t line_to_remove;

        if (offset > 0)
        {
            line_to_remove = (size_t)offset;
        }
        else if (offset < 0)
        {
            // Handling negative offset
            if ((int)(-offset) > (int)total_lines)        // Intially it was (size_t)(-offset) > total_lines
            {
                printf("psh: history -d: %ld: history position out of range\n", offset);
                fclose(fp1);
                return 1;
            }
            line_to_remove = total_lines + offset + 1;
        }
        else
        {
            printf("psh: history -d: 0: history position out of range\n");
            fclose(fp1);
            return 1;
        }

        // Checking if line_to_remove is within range
        if ((int)line_to_remove > (int)total_lines)
        {
            printf("psh: history -d: %zu: history position out of range\n",
                   line_to_remove);
            fclose(fp1);
            return 1;
        }

        // Removing the line the memory history file
        remove_line(MEMORY_HISTORY_FILE, line_to_remove);

        printf("Removed line %zu from history.\n", line_to_remove);

        fclose(fp1);
        break;
    }

    case 7:
        clear_session_history();
        break;

    case 8: // history -p !line_no
    {
        FILE *global_history = fopen(MEMORY_HISTORY_FILE, "r");
        FILE *session_history = fopen(SESSION_HISTORY_FILE, "r");

        if (!global_history || !session_history)
        {
            perror("Error opening history files");
            return 1;
        }

        // Skipping the "history" and "-p" tokens
        int i = 2;
        while (token_arr[i] != NULL)
        {
            char *expanded = expand_history(token_arr[i], global_history);
            if (!expanded)
            {
                expanded = expand_history(token_arr[i], session_history);
            }

            printf("%s\n", expanded);
            free(expanded);
            i++;
        }

        // case 11: // fc -l -n -r 20 50
        // {
        //   if (token_arr[3] != NULL && token_arr[4] == NULL) {
        //     printf("TESTING %s\n", token_arr[4]);
        //     int total_lines = count_lines(MEMORY_HISTORY_FILE);
        //     read_lines_reverse_wo_no(MEMORY_HISTORY_FILE, 1, total_lines);
        //   }

        //   else if (token_arr[5] == NULL) {
        //     int total_lines = count_lines(MEMORY_HISTORY_FILE);
        //     read_lines_reverse_wo_no(MEMORY_HISTORY_FILE, atoi(token_arr[4]),
        //                              total_lines);
        //   }

        //   else if (token_arr[4] != NULL && token_arr[5] != NULL) {
        //     read_lines_reverse_wo_no(MEMORY_HISTORY_FILE, atoi(token_arr[4]),
        //                              atoi(token_arr[5]));
        //   } else {
        //     printf("fc: too many arguments");
        //   }
        //   break;
        // }

    case 10: // fc -lnr 20 50
    {
        if (token_arr[2] == NULL)
        {
            int total_lines = count_lines(MEMORY_HISTORY_FILE);
            read_lines_reverse_wo_no(MEMORY_HISTORY_FILE, 1, total_lines);
        }

        else if (token_arr[3] == NULL)
        {
            int total_lines = count_lines(MEMORY_HISTORY_FILE);
            read_lines_reverse_wo_no(MEMORY_HISTORY_FILE, atoi(token_arr[2]),
                                     total_lines);
        }

        else if (token_arr[2] != NULL && token_arr[3] != NULL)
        {
            read_lines_reverse_wo_no(MEMORY_HISTORY_FILE, atoi(token_arr[2]),
                                     atoi(token_arr[3]));
        }
        else
        {
            printf("fc: too many arguments");
        }
        break;
    }

        fclose(global_history);
        fclose(session_history);
        return 1;

        break;
    }

    default:
        printf("psh: history: option not implemented\n");
        break;
    }
    return 1;
}

int PSH_EXPORT(char **token_arr)
{
    if (token_arr[1] == NULL ||
        strcmp(token_arr[1], "-p") == 0) // default null or -p option
    {
        extern char **environ;

        // find no of entries in environ
        int env_length = 0;
        while (environ[env_length] != NULL)
        {
            env_length++;
        }

        sort_strings(environ, env_length); // sort environment variables

        int i = 0;
        while (environ[i] != NULL)
        {
            printf("declare -x %s\n", environ[i]); // print all env variables
            i++;
        }
    }
    else if (strcmp(token_arr[1], "-f") == 0) // functions
    {
        // int found = 0;
        // for (int i = 0; i < num_funcs; i++)
        // {
        //     printf("%s\n%s\n", global_funcs[i].func_name,
        //     global_funcs[i].func_def); if (strcmp(token_arr[2],
        //     global_funcs[i].func_name) == 0)
        //     {
        //         if (setenv(global_funcs[i].func_name, global_funcs[i].func_def,
        //         1) != 0)
        //         {
        //             perror("PSH: setenv() error");
        //         }
        //         found = 1;
        //         break;
        //     }
        // }

        // if (!found)
        // {
        //     if (strchr(token_arr[2], '{'))
        //     {
        //         char *func_name = strtok(token_arr[2], "{");
        //         char *func_value_start = strchr(token_arr[2], '{');

        //         if (func_value_start != NULL)
        //         {
        //             char *func_value_end = strrchr(token_arr[2], '}');
        //             if (func_value_end != NULL)
        //             {
        //                 size_t func_value_length = func_value_end -
        //                 func_value_start + 1; char *func_value =
        //                 malloc(func_value_length); strncpy(func_value,
        //                 func_value_start, func_value_length - 1);
        //                 func_value[func_value_length - 1] = '\0';

        //                 // Set the environment variable
        //                 if (setenv(func_name, func_value, 1) != 0)
        //                 {
        //                     perror("PSH: setenv() error");
        //                 }

        //                 free(func_value);
        //             }
        //             else
        //             {
        //                 fprintf(stderr, "PSH: Invalid function definition\n");
        //             }
        //         }
        //         else
        //         {
        //             fprintf(stderr, "PSH: Invalid function definition\n");
        //         }
        //     }
        //     else
        //     {
        //         fprintf(stderr, "PSH: Invalid function definition\n");
        //     }
        // }
    }
    else if (strcmp(token_arr[1], "-n") == 0) // removing and env variable
    {
        if (token_arr[2] != NULL)
        {
            if (unsetenv(token_arr[2]) != 0)
            {
                perror("PSH: unsetenv() error");
            }
        }
    }
    else
    {
        // setting a variable as an env variable
        int found = 0;
        for (int i = 0; i < num_vars; i++)
        {
            if (strcmp(token_arr[1], global_vars[i].var_name) == 0)
            {
                if (setenv(global_vars[i].var_name, global_vars[i].var_value, 1) != 0)
                {
                    perror("PSH: setenv() error");
                }
                found = 1;
                break;
            }
        }

        if (!found)
        {
            if (strchr(token_arr[1], '='))
            {
                char *var_name = strtok(token_arr[1], "=");
                char *var_value = strtok(NULL, "=");

                if (var_value != NULL)
                {
                    // Set the environment variable
                    if (setenv(var_name, var_value, 1) != 0)
                    {
                        perror("PSH: setenv() error");
                    }
                }
                else
                {
                    fprintf(stderr, "PSH: Invalid variable assignment\n");
                }
            }
            else
            {
                fprintf(stderr, "Unknown option: %s\n", token_arr[1]);
            }
        }
    }

    return 1;
}

// Define the PSH_FOR function
int PSH_FOR(char **token_arr)
{
    size_t bufsize = 1024;
    char *loop_command = malloc(bufsize * sizeof(char));
    loop_command[0] = '\0';

    for (int i = 0; token_arr[i] != NULL; i++)
    {
        if (strlen(loop_command) + strlen(token_arr[i]) + 1 >= bufsize)
        {
            bufsize *= 2;
            loop_command = realloc(loop_command, bufsize * sizeof(char));
        }
        strcat(loop_command, token_arr[i]);
        strcat(loop_command, " ");
    }

    int run = 1;                              // Initialize run for the loop
    process_nested_loops(loop_command, &run); // Pass run to process_nested_loops

    free(loop_command);
    return run;
}

int PSH_TYPE(char **token_arr) // usage type <command>
{
    /* METHOD 1 */

    // char *builtin_str[] = {"exit", "cd", "echo", "pwd", "fc", "export", "for", "type"}; // for reference
    // int i = 0;
    // bool is_builtin = false;

    // while (i < size_builtin_str) {
    //     if(strcmp(token_arr[1], builtin_str[i]) == 0) {
    //         // printf("inf\n");
    //         is_builtin = true;
    //         break;
    //     }
    //     i++;
    // }

    // if (is_builtin) {
    //     printf("isbuiltin\n");
    //     return 1;
    // }
    // else {
    //     printf("external command\n");
    //     return 1;
    // }

    if (token_arr[1] == NULL)
    {
        printf("Usage: type <command>\n");
        return 1;
    }

    /* BETTER METHOD */

    // handling pwd, echo as in PSH they are built-ins
    if ((strcmp(token_arr[1], "pwd") == 0) || (strcmp(token_arr[1], "echo") == 0))
    {
        printf("%s is a PSH shell builtn\n", token_arr[1]);
        return 1;
    }

    char buff1[1024];
    char buff2[1024];
    struct stat stats;
    int perms = 0; // default is shell builtin

    snprintf(buff1, sizeof(buff1), "/usr/bin/%s", token_arr[1]);
    snprintf(buff2, sizeof(buff2), "/bin/%s", token_arr[1]);

    char *common_buff = malloc(1024 * 4);
    // printf("Checking: %s\n", buff1); //debug check

    if (stat(buff1, &stats) == 0)
    {
        // printf("Found in /usr/bin\n");
        perms = (stats.st_mode & S_IXUSR); // perms = 0 if command is a shell built-in
        // printf("%d\n",perms);
        strcpy(common_buff, buff1);
    }
    else if (stat(buff2, &stats) == 0)
    {
        // printf("Found in /bin\n");
        perms = (stats.st_mode & S_IXUSR);
        // printf("%d\n",perms);
        strcpy(common_buff, buff2);
    }

    // printf("%d this is perms\n",perms);
    if (perms == 0)
    {
        printf("%s is a PSH shell builtn\n", token_arr[1]);
        free(common_buff);
    }
    else
    {
        printf("%s is shell external in %s\n", token_arr[1], common_buff);
        free(common_buff);
    }
    return 1;
}

int PSH_READ_SHELL(char **token_arr)
{

    // char *buff = malloc(PATH_MAX);
    // char **arr_vars = malloc(PATH_MAX);
    int n = 0;

    if (token_arr[1] == NULL)
    { // read
        n = 1;
    }
    else if (strcmp(token_arr[1], "<<<") == 0)
    { // read <<< "hello, world"
        n = 2;
    }
    else if (strcmp(token_arr[1], "-p") == 0)
    { // read -p "Enter prompt" var1
        n = 3;
    }
    else if (token_arr[1] != NULL)
    { // read var1 var2
        n = 4;
    }

    switch (n)
    {

    case 1:
    {
        char *buff = malloc(PATH_MAX);
        fgets(buff, PATH_MAX, stdin);
        buff[strcspn(buff, "\n")] = '\0';
        setenv("REPLY", buff, 1);
        free(buff);
        return 1;
    }

    case 2: // bug : also works if it is not in quotes
    {
        setenv("REPLY", token_arr[2], 1);

        return 1;
    }

    case 3:
    {
        char *buff = malloc(PATH_MAX);
        printf("%s ", token_arr[2]); // read -p "Enter your name: " name
        fgets(buff, PATH_MAX, stdin);
        buff[strcspn(buff, "\n")] = '\0';
        setenv(token_arr[3], buff, 1);
        free(buff);
        return 1;
    }

    case 4:
    {

        char *buff = malloc(PATH_MAX);
        char **arr_vars = malloc(PATH_MAX);

        fgets(buff, PATH_MAX, stdin);
        buff[strcspn(buff, "\n")] = '\0';
        arr_vars = PSH_TOKENIZER(buff);
        int k = 0;
        while (arr_vars[k] != NULL) // printing arr_vars
        {
            // printf("%s\n",arr_vars[k]);
            setenv(token_arr[k + 1], arr_vars[k], 1);
            k++;
        }
        free(buff);
        free(arr_vars);
        return 1;
    }

    default:
    {
        fprintf(stderr, "psh error: read failed\n");
        return -1;
    }
    }
    return 1;
}

int PSH_ALIAS(char **token_arr)
{
  // Setting ALIAS file location


    char ALIAS[PATH_MAX];
    // getcwd(cwd, sizeof(cwd)); // home/$USER/psh
    // strcpy(PATH, cwd);
    // strcat(ALIAS, "/.files/ALIAS");

    get_alias_path(ALIAS, sizeof(ALIAS), cwd);
    // printf("ALIAS PATH: %s\n", ALIAS);
    // Initializing HashMap
    HashMap *map = create_map(HASHMAP_SIZE);
    load_aliases(map, ALIAS);
    if (token_arr[1] == NULL || strcmp(token_arr[1], "-p") == 0)
    {
            for (int i = 0; i < map->size; i++) 
            {
                Alias *current = map->buckets[i];
                while (current) 
                {
                    printf("%s=%s\n", current->name, current->command);
                    current = current->next;
                }
            }
    } 
    else if (strchr(token_arr[1], '=')) 
    {
        char *name = NULL;
        char *command = NULL;
        char *delimiter = strchr(token_arr[1], '=');
        *delimiter = '\0';
        name = token_arr[1];
        command = delimiter + 1;
        // printf("Command: %s\n", command);
        if (command && strchr(command, '\n')) 
        {
            *strchr(command, '\n') = '\0';
        }
        if (find(map, name))
        {
            delete_alias(map, name);
        }
        insert_alias(map, name, command);
    } 
    else 
    {
        fprintf(stderr, "Unknown option: %s\n", token_arr[1]);
    }
    // Save Aliases to file
    save_aliases(map, ALIAS);

    // Free memory
    free_map(map);
    return 1;
    
}

int PSH_UNALIAS(char **token_arr)
{
    char ALIAS[PATH_MAX];
    get_alias_path(ALIAS, sizeof(ALIAS), cwd);
    // Initializing HashMap
    HashMap *map = create_map(HASHMAP_SIZE);
    load_aliases(map, ALIAS);
    if (token_arr[1] == NULL)
    {
        fprintf(stderr, "Must have flag or alias name to be removed\n");
    }
    else if (strcmp(token_arr[1], "-a") == 0)
    {
        delete_all_aliases(map);
    }
    else if (find(map, token_arr[1]))
    {
        delete_alias(map, token_arr[1]);
    }
    else
    {
        fprintf(stderr, "alias %s not found\n", token_arr[1]);
    }
    // Save Aliases to file
    save_aliases(map, ALIAS);

    // Free memory
    free_map(map);

    return 1;
}
