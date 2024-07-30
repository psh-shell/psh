// helpers.c
#include "psh.h"

volatile sig_atomic_t SIGNAL = 0;

void free_double_pointer(char **array)
{
    if (array == NULL)
    {
        return;
    }
    for (int i = 0; array[i] != NULL; i++)
    {
        free(array[i]);
    }
    free(array);
}

void remove_last_component(char *path)
{
    char *last_slash = strrchr(path, '/');
    if (last_slash != NULL)
    {
        *last_slash = '\0';
    }
}

void get_last_path_component(const char *full_path, char *last_component)
{
    char *last_slash = strrchr(full_path, '/');
    if (last_slash != NULL)
    {
        strcpy(last_component, last_slash + 1); // +1 to skip the '/'
    }
    else
    {
        strcpy(last_component, full_path); // Fallback in case there's no '/'
    }
}

// Function to read lines
void read_lines(const char *filename, int low_lim, int up_lim)
{
    FILE *file = fopen(filename, "r");
    char *line = NULL;
    size_t len = 0;
    size_t line_no = 1;

    if (file == NULL)
    {
        perror("Error:");
        return;
    }

    while ((getline(&line, &len, file) != -1) && line != NULL)
    {
        if (line_no >= low_lim && line_no <= up_lim)
        {
            printf("%zu %s", line_no, line);
        }
        line_no++;
        if (line_no > up_lim)
        {
            break;
        }
    }

    fclose(file);

    if (line)
    {
        free(line);
    }
}

// Function to read lines without displaying the numbers
void read_lines_wo_no(const char *filename, int low_lim, int up_lim)
{
    FILE *file = fopen(filename, "r");
    char *line = NULL;
    size_t len = 0;
    size_t line_no = 1;

    if (file == NULL)
    {
        perror("Error:");
        return;
    }

    while (getline(&line, &len, file) != -1)
    {
        if (line_no >= low_lim && line_no <= up_lim)
        {
            printf("%s", line);
        }
        line_no++;
        if (line_no > up_lim)
        {
            break;
        }
    }

    fclose(file);

    if (line)
    {
        free(line);
    }
}

// Function to count lines
int count_lines(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    int total_lines = 0;
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        total_lines++;
    }
    fclose(fp);
    return total_lines;
}

void read_lines_reverse(const char *filename, int low_lim, int up_lim)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Error:");
        return;
    }

    char **lines = NULL;
    size_t size = 0;
    size_t capacity = 10;
    lines = malloc(capacity * sizeof(char *));
    if (lines == NULL)
    {
        perror("Error:");
        fclose(file);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    size_t line_no = 1;

    while (getline(&line, &len, file) != -1)
    {
        if (line_no >= low_lim && line_no <= up_lim)
        {
            if (size == capacity)
            {
                capacity *= 2;
                lines = realloc(lines, capacity * sizeof(char *));
                if (lines == NULL)
                {
                    perror("Error:");
                    free(line);
                    fclose(file);
                    return;
                }
            }
            // Storing the line with the line number
            lines[size++] = malloc((strlen(line) + 1) * sizeof(char));
            sprintf(lines[size - 1], "%zu %s", line_no, line);
        }
        line_no++;
        if (line_no > up_lim)
        {
            break;
        }
    }

    fclose(file);
    if (line)
    {
        free(line);
    }

    for (size_t i = size; i > 0; i--)
    {
        printf("%s", lines[i - 1]);
        free(lines[i - 1]);
    }

    free(lines);
}

// Function to remove a specific line
void remove_line(const char *filename, size_t line_to_remove)
{
    FILE *file = fopen(filename, "r");
    FILE *tempfile = fopen("temp.txt", "w");
    char *line = NULL;
    size_t len = 0;
    size_t line_no = 1;

    if (file == NULL || tempfile == NULL)
    {
        perror("Error:");
        return;
    }

    while (getline(&line, &len, file) != -1)
    {
        if (line_no != line_to_remove)
        {
            fputs(line, tempfile);
        }
        line_no++;
    }

    fclose(file);
    fclose(tempfile);

    remove(filename);
    rename("temp.txt", filename);

    if (line)
    {
        free(line);
    }
}

void delete_file(const char *filename){
    if (remove(filename) != 0)
    {
        perror("Error deleting file");
    }
}

void clear_session_history(void)
{
    char MEMORY_HISTORY_FILE[PATH_MAX];
    char SESSION_HISTORY_FILE[PATH_MAX];

    strcpy(path_memory, cwd);
    strcat(path_memory, "/.files/MEMORY_HISTORY_FILE");
    strcpy(MEMORY_HISTORY_FILE, path_memory);

    char path_session[PATH_MAX];
    strcpy(path_session, cwd);
    strcat(path_session, "/.files/SESSION_HISTORY_FILE");
    strcpy(SESSION_HISTORY_FILE, path_session);

    FILE *global_fp, *session_fp, *temp_fp;
    char *global_line = NULL, *session_line = NULL;
    size_t global_len = 0, session_len = 0;
    ssize_t global_read, session_read;
    int found;

    // Open the files
    global_fp = fopen(MEMORY_HISTORY_FILE, "r");
    session_fp = fopen(SESSION_HISTORY_FILE, "r");
    temp_fp = fopen("temp_history_file.txt", "w");

    if (global_fp == NULL || session_fp == NULL || temp_fp == NULL)
    {
        perror("Error opening files");
        exit(EXIT_FAILURE);
    }

    // Read global history and remove session history lines
    while ((global_read = getline(&global_line, &global_len, global_fp)) != -1)
    {
        rewind(session_fp); // Reset session file pointer for each global line
        found = 0;
        while ((session_read = getline(&session_line, &session_len, session_fp)) !=
               -1)
        {
            if (strcmp(global_line, session_line) == 0)
            {
                found = 1;
                break;
            }
        }
        if (!found)
        {
            fprintf(temp_fp, "%s", global_line);
        }
    }

    // Free allocated memory and close files
    free(global_line);
    free(session_line);
    fclose(global_fp);
    fclose(session_fp);
    fclose(temp_fp);

    // Replace global history file with the updated one
    remove(MEMORY_HISTORY_FILE);
    rename("temp_history_file.txt", MEMORY_HISTORY_FILE);

    // Clear session history file
    temp_fp = fopen(SESSION_HISTORY_FILE, "w");
    fclose(temp_fp);
    printf("Session history cleared.\n");
}

char *expand_history(const char *arg, FILE *history_file)
{
    char line[MAX_LINE_LENGTH];
    char *expanded = NULL;

    if (arg[0] == '!')
    {
        int event_number = atoi(arg + 1);
        int current_line = 0;

        while (fgets(line, sizeof(line), history_file) != NULL)
        {
            current_line++;
            if (current_line == event_number)
            {
                // Removing newline character if present
                // line[strcspn(line, "\n")] = 0;
                expanded = strdup(line);
                break;
            }
        }
    }

    return expanded ? expanded : strdup(arg);
}

void read_lines_reverse_wo_no(const char *filename, int low_lim, int up_lim)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Error:");
        return;
    }

    char **lines = NULL;
    size_t size = 0;
    size_t capacity = 10;
    lines = malloc(capacity * sizeof(char *));
    if (lines == NULL)
    {
        perror("Error:");
        fclose(file);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    size_t line_no = 1;

    while (getline(&line, &len, file) != -1)
    {
        if (line_no >= low_lim && line_no <= up_lim)
        {
            if (size == capacity)
            {
                capacity *= 2;
                lines = realloc(lines, capacity * sizeof(char *));
                if (lines == NULL)
                {
                    perror("Error:");
                    free(line);
                    fclose(file);
                    return;
                }
            }
            // Storing the line without the line number
            lines[size++] = strdup(line);
        }
        line_no++;
        if (line_no > up_lim)
        {
            break;
        }
    }

    fclose(file);
    if (line)
    {
        free(line);
    }

    for (size_t i = size; i > 0; i--)
    {
        printf("%s", lines[i - 1]);
        free(lines[i - 1]);
    }
    free(lines);
}

int compare_strings(const void *a, const void *b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}

void sort_strings(char **strings, int num_strings)
{
    qsort(strings, num_strings, sizeof(char *), compare_strings);
}

int size_token_arr(char **token_arr)
{
    int i = 0;
    while (token_arr[i] != NULL)
    {
        i++;
    }
    return i;
}

bool contains_wildcard(char **token_arr)
{

    int size = size_token_arr(token_arr);
    for (int i = 0; i < size; i++)
    {
        if (strchr(token_arr[i], '?') ||
            strchr(token_arr[i], '*'))
        {
            return true;
        }
    }
    return false;
}

int handle_wildcard(char *pattern)
{
    char **found;
    glob_t glob_struct;
    int run;

    run = glob(pattern, GLOB_ERR, NULL, &glob_struct);

    // check for errors
    if (run != 0)
    {
        if (run == GLOB_NOMATCH)
            fprintf(stderr, "No matches\n");

        else
            fprintf(stderr, "Some kinda glob error\n");
        // exit(1);
        return 1;
    }

    /* success, output found filenames */
    printf("\n");
    printf("Found %zu filename matches\n", glob_struct.gl_pathc);
    found = glob_struct.gl_pathv;

    while (*found)
    {
        printf("%s\n", *found);
        found++;
    }
    printf("\n");
    return 1;
}

char *find_closing_done(char *start)
{
    int depth = 0;
    char *pos = start;
    while (*pos)
    {
        if (strncmp(pos, "for ", 4) == 0)
        {
            depth++;
        }
        else if (strncmp(pos, "done", 4) == 0)
        {
            if (depth == 0)
            {
                return pos;
            }
            depth--;
        }
        pos++;
    }
    return NULL;
}

void process_nested_loops(char *command, int *run)
{
    char *current_command = command;
    while ((current_command = strstr(current_command, "for ")) != NULL)
    {
        current_command = process_for_loop(current_command, run);
        if (!current_command || *run == 0)
        {
            return;
        }
    }
}

char *process_for_loop(char *loop_command, int *run)
{
    char *saveptr;
    char *start = strstr(loop_command, "for ");
    if (!start)
    {
        fprintf(stderr, "Error: Missing 'for' keyword\n");
        return NULL;
    }

    start += 4;
    char *var_name = strtok(start, " ");
    if (!var_name)
    {
        fprintf(stderr, "Error: Missing variable name\n");
        return NULL;
    }

    char *in = strtok(NULL, " ");
    if (!in || strcmp(in, "in") != 0)
    {
        fprintf(stderr, "Error: Missing 'in' keyword\n");
        return NULL;
    }

    char *values = strtok(NULL, ";");
    if (!values)
    {
        fprintf(stderr, "Error: Missing values after 'in'\n");
        return NULL;
    }

    char *do_keyword = strtok(NULL, " ");
    if (!do_keyword || strcmp(do_keyword, "do") != 0)
    {
        fprintf(stderr, "Error: Missing 'do' keyword\n");
        return NULL;
    }

    char *commands_start = do_keyword + 3;
    char *commands_end = find_closing_done(commands_start);
    if (!commands_end)
    {
        fprintf(stderr, "Error: Missing 'done' keyword\n");
        return NULL;
    }

    size_t commands_length = commands_end - commands_start;
    char *commands = malloc(commands_length + 1);
    if (!commands)
    {
        fprintf(stderr, "Error: Allocation error for commands\n");
        return NULL;
    }

    strncpy(commands, commands_start, commands_length);
    commands[commands_length] = '\0';

    char *value = strtok_r(values, " ", &saveptr);
    while (value)
    {
        char command_block[PATH_MAX];
        snprintf(command_block, PATH_MAX, "%s=%s; %s", var_name, value, commands);
        value = strtok_r(NULL, " ", &saveptr);
        process_commands(command_block, run);
        if (*run == 0)
        {
            free(commands);
            return commands_end + 4; // Return the position after "done"
        }
    }
    free(commands);
    return commands_end + 4; // Return the position after "done"
}

void get_last_line(char **inputline) {
    last_command_up = 1;
    FILE *fp1 = fopen(path_memory, "r");

    if (fp1 == NULL) {
        perror("Error opening file");
        return;
    }

    char line[MAX_LINE_LENGTH] = "";
    char lastLine[MAX_LINE_LENGTH] = "";

    // Read each line and store the last one in lastLine
    while (fgets(line, sizeof(line), fp1)) {
        strcpy(lastLine, line);
    }

    fclose(fp1);

    // Remove newline character if present
    size_t len = strlen(lastLine);
    if (len > 0 && lastLine[len-1] == '\n') {
        lastLine[len-1] = '\0';
    }

    // Allocate memory for inputline and copy lastLine
    *inputline = strdup(lastLine);
    if (*inputline == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
}

void parse_ps1(const char *ps1, const char *cwd) {
    char expanded[4096] = "";
    char *exp_ptr = expanded;
    
    while (*ps1) {
        if (*ps1 == '\\') {
            ps1++;
            switch (*ps1) {
                case 'u':
                    exp_ptr += sprintf(exp_ptr, "%s", getenv("USER"));
                    break;
                case 'h':
                    {
                        char hostname[256];
                        gethostname(hostname, sizeof(hostname));
                        exp_ptr += sprintf(exp_ptr, "%s", hostname);
                    }
                    break;
                case 'w':
                    exp_ptr += sprintf(exp_ptr, "%s", cwd);
                    break;
                case 'W':
                    {
                        char *last_slash = strrchr(cwd, '/');
                        exp_ptr += sprintf(exp_ptr, "%s", last_slash ? last_slash + 1 : cwd);
                    }
                    break;
                case '$':
                    exp_ptr += sprintf(exp_ptr, " %s ", getuid() == 0 ? "#" : "$");
                    break;
                case '[':
                case ']':
                    // Ignore these characters as they're used for bash prompt escaping
                    break;
                case 'e':
                    *exp_ptr++ = '\033';  // ESC character
                    break;
                default:
                    *exp_ptr++ = '\\';
                    *exp_ptr++ = *ps1;
            }
        } else if (*ps1 == '\033') {
            // If we encounter an ESC character directly, copy it
            *exp_ptr++ = *ps1;
        } else {
            *exp_ptr++ = *ps1;
        }
        ps1++;
    }
    *exp_ptr = '\0';
    
    printf("%s$ ", expanded);
    fflush(stdout);
}

void print_prompt(const char *PATH) {
    char *ps1 = getenv("PS1");
    if (ps1 == NULL) {
        // New default PS1
        ps1 = "\\[\\e[1;36m\\]\\u\\[\\e[0m\\]@\\[\\e[1;34m\\]PSH\\[\\e[0m\\] → \\[\\e[1;35m\\]\\W\\[\\e[0m\\]";
    }
    parse_ps1(ps1, PATH);
    setenv("PS1", ps1, 1);
}

void load_history(void) {

    free_history(); //free existing history
    
    // char *history[MAX_HISTORY];
    // int history_count = 0;
    // int current_history = -1;

    FILE *fp = fopen(path_memory, "r");
    if (fp == NULL) {
        perror("Error opening history file");
        return;
    }

    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), fp) && history_count < PATH_MAX) {
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        history[history_count] = strdup(line);

        if(history[history_count] == NULL) {
            perror("mem alloc failed");
            free_history();
            fclose(fp);
            return;
        }
        history_count++;
    }

    fclose(fp);
}

void free_history(void) {
    for (int i = 0; i < history_count; i++) {
        free(history[i]);
    }
    history_count = 0;
    // free_double_pointer(history);
}

int kbhit(void) {
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO); //disables canonical mode & echo 
    tcsetattr(STDIN_FILENO, TCSANOW, &newt); //sets new terminal settings
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}
// Function to enable raw mode
void enableRawMode(void) {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag &= ~(ECHO | ICANON);                //change from canonical to raw and turning off echo
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Function to disable raw mode
void disableRawMode(void) {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag |= (ECHO | ICANON);                 //change from raw to canonical and turning on echo
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

char *trim_whitespace(char *str) {
    char *end;

    // Trim leading space
    while (isspace((unsigned char)*str)) str++;

    if (*str == 0)  // All spaces
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator character
    end[1] = '\0';

    return str;
}

void generate_session_id(void) {
    snprintf(session_id, sizeof(session_id), "%ld", (long)time(NULL));
    // printf("%s",session_id);
    setenv("SESSIONID", session_id, 1);
}

void initialize_paths(const char *cwd) {
    snprintf(path_memory, sizeof(path_memory), "%s/.files/MEMORY_HISTORY_FILE", cwd);
}

void get_session_path(char *path_session, size_t size, const char *cwd) {
    snprintf(path_session, size, "%s/.files/SESSION_HISTORY_FILE_%s", cwd, session_id);
}

void initialize_shell(const char *cwd) {
    generate_session_id();
    initialize_paths(cwd);
}

unsigned int hash(const char *str, int size)
{
  unsigned int hash = 0;
  int i = 0;
  while (str[i])
  {
    hash = (hash << 5) + str[i++];
  }
  return hash % size;
}

HashMap *create_map(int size)
{
    HashMap *map = (HashMap *)malloc(sizeof(HashMap));
    if (!map)
    {
        perror("Failed to allocate memory for HashMap");
        exit(EXIT_FAILURE);
    }
    map->size = size;
    map->buckets = (Alias **)malloc(size * sizeof(Alias *));
    if (!map->buckets)
    {
        perror("Failed to allocate memory for HashMap buckets");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < size; i++)
    {
        map->buckets[i] = NULL;
    }
    return map;
}

void delete_all_aliases(HashMap *map) 
{
    for (int i = 0; i < map->size; i++)
    {
        Alias *current = map->buckets[i];
        while (current)
        {
            Alias *delete = current;
            current = current->next;
            free(delete->name);
            free(delete->command);
            free(delete);
        }
        map->buckets[i] = NULL;
    }
}

void insert_alias(HashMap *map, const char *name, const char *command)
{
  // printf("Inside insert function\n"); //debugging
  unsigned int bucket_index = hash(name, map->size);
  Alias *new_alias = (Alias *)malloc(sizeof(Alias));
  if (!new_alias)
  {
    perror("Failed to allocate memory for new alias");
    return;
  }
  new_alias->name = strdup(name);
  new_alias->command = strdup(command);
  new_alias->next = map->buckets[bucket_index];
  map->buckets[bucket_index] = new_alias;
}

const char *get_alias_command(HashMap *map, const char *name)
{
    unsigned int bucket_index = hash(name, map->size);
    Alias *current = map->buckets[bucket_index];
    while (current)
    {
        if (strcmp(current->name, name) == 0)
        {
            return current->command;
        }
        current = current->next;
    }
    return NULL;
}

void load_aliases(HashMap *map, const char *filepath)
{
    FILE *fp = fopen(filepath, "r");
    if (!fp)
    {
        fp = fopen(filepath, "w");
        if (!fp)
        {
            perror("Failed to create ALIAS file\n");
            free(map->buckets);
            free(map);
            //return -1;
        }
        fclose(fp);
        fp = fopen(filepath, "r");
    }
    if (fp != NULL)
    {
        char line[256];
        while (fgets(line, sizeof(line), fp))
        {
            if (strchr(line, '='))
            {
                char *name = NULL;
                char *command = NULL;
                char *delimiter = strchr(line, '=');
                *delimiter = '\0';
                name = line;
                command = delimiter + 1;

                if (command && strchr(command, '\n'))
                {
                    *strchr(command, '\n') = '\0';
                }
                insert_alias(map, name, command);
            }
        }
        fclose(fp);
    }
}

void save_aliases(HashMap *map, const char *filepath)
{
    FILE *fp = fopen(filepath, "w");
    if (!fp)
    {
        perror("PSH: Failed to open file");
        return;
    }
    for (int i = 0; i < map->size; i++)
    {
        Alias *current = map->buckets[i];
        while (current)
        {
            fprintf(fp, "%s=%s\n", current->name, current->command);
            current = current->next;
        }
    }
    fclose(fp);
}

void free_map(HashMap *map)
{
    for (int i = 0; i < map->size; i++)
    {
        Alias *current = map->buckets[i];
        while (current)
        {
            Alias *delete = current;
            current = current->next;
            free(delete->name);
            free(delete->command);
            free(delete);
        }
    }
    free(map->buckets);
    free(map);
}

Alias *find(HashMap *map, const char *name)
{
    unsigned int index = hash(name, map->size);
    Alias *entry = map->buckets[index];

    while (entry != NULL)
    {
        if (strcmp(entry->name, name) == 0)
        {
            return entry;
        }
        entry = entry->next;
    }
    return NULL; // Name not found
}

void delete_alias(HashMap *map, const char *name)
{
    unsigned int index = hash(name, map->size);
    Alias *entry = map->buckets[index];
    Alias *prev = NULL;

    while (entry != NULL)
    {
        if (strcmp(entry->name, name) == 0) {
            if (prev == NULL) 
            {
                map->buckets[index] = entry->next;
            }
            else
            {
                prev->next = entry->next;
            }
            free(entry->name);
            free(entry->command);
            free(entry);
            return;
        }
        prev = entry;
        entry = entry->next;
    }
}

char **split_strings(const char *string) {
    char *str = strdup(string);
    if (!str) {
        fprintf(stderr, "psh: strdup allocation error\n");
        exit(EXIT_FAILURE);
    }

    size_t bufsize = 64;
    int position = 0;
    char **split_arr = malloc(bufsize * sizeof(char *));
    if (!split_arr) {
        fprintf(stderr, "psh: allocation error\n");
        free(str);
        exit(EXIT_FAILURE);
    }

    char *token = strtok(str, " ");
    while (token != NULL) {
        split_arr[position] = strdup(token);
        if (!split_arr[position]) {
            fprintf(stderr, "psh: strdup allocation error\n");
            free(str);
            for (int i = 0; i < position; i++) {
                free(split_arr[i]);
            }
            free(split_arr);
            exit(EXIT_FAILURE);
        }
        position++;
        if (position >= bufsize) {
            bufsize += 64;
            split_arr = realloc(split_arr, bufsize * sizeof(char *));
            if (!split_arr) {
                fprintf(stderr, "psh: allocation error\n");
                free(str);
                for (int i = 0; i < position; i++) {
                    free(split_arr[i]);
                }
                free(split_arr);
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, " ");
    }
    split_arr[position] = NULL;
    free(str);
    return split_arr;
}

char **replace_alias(HashMap *map, char **token_arr) {
    const char *command = get_alias_command(map, token_arr[0]);
    if (command) {
        size_t bufsize = 64;
        int position = 0;
        char **new_token_arr = malloc(bufsize * sizeof(char *));
        if (!new_token_arr) {
            fprintf(stderr, "psh: allocation error\n");
            exit(EXIT_FAILURE);
        }

        char **temp_arr = split_strings(command);
        while (temp_arr[position] != NULL) {
            new_token_arr[position] = strdup(temp_arr[position]);
            if (!new_token_arr[position])
            {
                fprintf(stderr, "psh: strdup allocation error\n");
                for (int i = 0; i < position; i++) 
                {
                    free(new_token_arr[i]);
                }
                free(new_token_arr);
                exit(EXIT_FAILURE);
            }
            position++;
            if (position >= bufsize) 
            {
                bufsize += 64;
                new_token_arr = realloc(new_token_arr, bufsize * sizeof(char *));
                if (!new_token_arr) 
                {
                    fprintf(stderr, "psh: allocation error\n");
                    for (int i = 0; i < position; i++) 
                    {
                        free(new_token_arr[i]);
                    }
                    free(new_token_arr);
                    exit(EXIT_FAILURE);
                }
            }
        }

        int i = 1;
        while (token_arr[i] != NULL) {
            new_token_arr[position] = strdup(token_arr[i]);
            if (!new_token_arr[position]) {
                fprintf(stderr, "psh: strdup allocation error\n");
                for (int j = 0; j < position; j++) {
                    free(new_token_arr[j]);
                }
                free(new_token_arr);
                //exit(EXIT_FAILURE);
            }
            position++;
            i++;
            if (position >= bufsize) {
                bufsize += 64;
                new_token_arr = realloc(new_token_arr, bufsize * sizeof(char *));
                if (!new_token_arr) {
                    fprintf(stderr, "psh: allocation error\n");
                    for (int j = 0; j < position; j++) {
                        free(new_token_arr[j]);
                    }
                    free(new_token_arr);
                    //exit(EXIT_FAILURE);
                }
            }
        }
        new_token_arr[position] = NULL;

        // Free temp_arr
        for (i = 0; temp_arr[i] != NULL; i++) {
            free(temp_arr[i]);
        }
        free(temp_arr);

        return new_token_arr;
    } else {
        return token_arr;
    }
}

char *remove_quotes(char *str) {
    size_t len = strlen(str);
    if ((str[0] == '"' && str[len - 1] == '"') || (str[0] == '\'' && str[len - 1] == '\'')) {
        str[len - 1] = '\0';
        return str + 1;
    }
    return str;
}

// Function to expand environment variables within a string
char *expand_variables(char *str) {
    static char buffer[1024];
    char *ptr = str;
    char *buf_ptr = buffer;

    while (*ptr) {
        if (*ptr == '$') {
            ptr++;
            char var_name[256];
            char *var_ptr = var_name;

            while (*ptr && (isalnum(*ptr) || *ptr == '_')) {
                *var_ptr++ = *ptr++;
            }
            *var_ptr = '\0';

            char *var_value = getenv(var_name);
            if (var_value) {
                while (*var_value) {
                    *buf_ptr++ = *var_value++;
                }
            }
        } else {
            *buf_ptr++ = *ptr++;
        }
    }
    *buf_ptr = '\0';
    return buffer;
}

void handle_env_variable(char *token_arr[]) {
    if (strchr(token_arr[0], '=')) {
        char *input = strdup(token_arr[0]);
        if (input == NULL) {
            perror("strdup");
            return;
        }

        char *var_name = strtok(input, "=");
        char *var_value = strtok(NULL, "=");

        if (var_name && var_value) {
            var_value = remove_quotes(var_value);
            var_value = expand_variables(var_value);

            if (setenv(var_name, var_value, 1) != 0) {
                perror("setenv");
            }
        } else {
            fprintf(stderr, "Error: Invalid environment variable assignment\n");
        }

        free(input);
    }
}

void get_alias_path(char *path_session, size_t size, const char *cwd) {
    snprintf(path_session, size, "%s/.files/ALIAS", cwd);
}

void sigint_handler(int sig)
{
    const char *message = "SIGINT Detected\n";
    write(STDOUT_FILENO, message, strlen(message));
    SIGNAL = 1;
    setenv("?", "130", 1);
}
