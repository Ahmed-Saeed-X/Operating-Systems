#define _POSIX_C_SOURCE 200809L 

#include <stdio.h>  // Standard I/O functions 
#include <stdlib.h> // Memory management functions 
#include <string.h> // String manipulation functions 
#include <ctype.h>  // Character handling functions 
#include <unistd.h> // POSIX API functions (e.g., fork, execvp, chdir)
#include <sys/wait.h> // Process waiting functions (e.g., waitpid)
#include <signal.h>  // Signal handling functions (e.g., sigaction, SIGCHLD)
#include <fcntl.h>   // File control functions (e.g., open, close)
#include <errno.h>   // Error handling functions (e.g., errno, perror)

#define MAX_INPUT_LEN 1024 // Maximum length of user input (1024 characters)
#define MAX_ARGS 100       // Maximum number of arguments for a command (100)

// Global file descriptor for the shell.log file
static int log_fd;

// ----------------------------
// SIGCHLD Handler
// ----------------------------
void on_child_exit(int signo) {

    int saved_errno = errno; // Save errno to avoid interference
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (log_fd != -1) {
            char msg[128]; // Buffer for the log message
            int len = snprintf(msg, sizeof(msg), "The Child process with pid = %d was terminated\n", pid);
            if (len > 0) {
                write(log_fd, msg, len); // Write to log file
            }
        }
    }
    errno = saved_errno; // Restore errno
}

// ----------------------------
// Register SIGCHLD Signal Handler
// ----------------------------
void register_child_signal() {

    // creating an sigaction instance 
    struct sigaction sa;

    // assining "on_child_exit" as its handler
    sa.sa_handler = on_child_exit; 

    // ensuring that no additional signals are blocked during the execution of the SIGCHLD
    sigemptyset(&sa.sa_mask);     

    // SA_RESTART: Automatically restart certain system calls that are interrupted by this signal
    // SA_NOCLDSTOP: Do not call the handler when children stop only when it Terminates
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    // Calling the sigaction function to register the sa structure for the SIGCHLD signal and handling failures
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction"); // Printing error if sigaction fails
        exit(EXIT_FAILURE);
    }
}

// ----------------------------
// Built-in Commands
// ----------------------------
void handle_cd(char **args) {
    // Getting the directory path from args[1]
    char *path = args[1];
    
    // Handling special cases: no argument or "~"
    if (!path || strcmp(path, "~") == 0) 
        path = getenv("HOME"); //The home directory is stored in the HOME environment variable 
   
    // Changing the directory
    if (chdir(path) != 0) 
        perror("cd failed"); // Printing error if chdir fails
}

void handle_echo(char **args) {
    // If no arguments, print a newline and return
    if (!args[1]) { 
        printf("\n"); 
        return; 
    }

    // Get the input string
    char *input = args[1];

    // Initialize the output buffer
    char output[MAX_INPUT_LEN] = {0};

    // Tokenize the input string
    char *token = strtok(input, " ");
    while (token) {
        // Handle environment variables
        if (token[0] == '$') {
            char *value = getenv(token + 1); // Get environment variable value
            if (value) strcat(output, value); // Append value to output
        } 
        else {
            strcat(output, token); // Append token to output
        }

        // Get the next token
        token = strtok(NULL, " ");
        if (token){
            strcat(output, " "); // Add space between tokens
        }
    }

    // Print the final output
    printf("%s\n", output);
}
void handle_export(char **args) {

    // Count the number of tokens
    int token_count = 0;
    while (args[token_count] != NULL) {
        token_count++;
    }

    // Handle the no arguments case
    if (token_count < 2) {
        // If no argument is provided, print all environment variables.
        extern char **environ;
        for (char **env = environ; *env; env++) {
            printf("%s\n", *env);
        }
        return;
    }

    // Calculate the total length needed to Combine tokens into one string "assignment".
    size_t total_len = 0;
    for (int i = 1; i < token_count; i++) {
        total_len += strlen(args[i]) + 1; // +1 for space or terminating null
    }

    // Allocate memory for the combined assignment string.
    char *assignment = malloc(total_len);
    if (!assignment) {
        perror("malloc");
        return;
    }

    // Start with an empty string.
    assignment[0] = '\0';

    // Concatenate all tokens from args[1] onward, inserting a space between them.
    for (int i = 1; i < token_count; i++) {
        strcat(assignment, args[i]);
        if (i < token_count - 1)
            strcat(assignment, " ");
    }
    
    // Find the '=' character to split the variable name from its value.
    char *eq = strchr(assignment, '=');
    if (!eq) {
        fprintf(stderr, "export: invalid assignment\n");
        free(assignment);
        return;
    }
    
    // Extract the variable name
    int var_len = eq - assignment;
    char varname[128];
    if (var_len >= sizeof(varname)) {
        fprintf(stderr, "export: variable name too long\n");
        free(assignment);
        return;
    }
    strncpy(varname, assignment, var_len);
    varname[var_len] = '\0';
    
    // Get the value part (everything after the '=')
    char *value = eq + 1;
    int val_len = strlen(value);
    
    // Remove surrounding quotes if present
    if (val_len >= 2 && value[0] == '"' && value[val_len - 1] == '"') {
        value[val_len - 1] = '\0'; // Remove closing quote
        value++; // Move past the opening quote
    }
    
    // Set the environment variable
    if (setenv(varname, value, 1) != 0) {
        perror("export failed");
    }
    
    free(assignment);
}

// ----------------------------
// Core Shell Functionality
// ----------------------------
char **parse_input(char *input, int *is_background) {

    char **args = malloc(MAX_ARGS * sizeof(char *));
    int i = 0;
    char *p = input;

    while (*p && i < MAX_ARGS - 1) {
        // Skip leading spaces
        while (*p && isspace(*p)) p++;
        if (!*p) break;

        // Handle quoted strings
        if (*p == '"') {
            p++; // Skip the opening quote
            char *start = p;
            while (*p && *p != '"') p++; // Find the closing quote
            if (*p == '"') {
                args[i++] = strndup(start, p - start); // Copy the quoted string
                p++; // Skip the closing quote
            } else {
                // Unmatched quote, treat the rest as a single token
                args[i++] = strdup(start);
            }
        } else {
            // Handle unquoted tokens
            char *start = p;
            while (*p && !isspace(*p)) p++;
            args[i++] = strndup(start, p - start);
        }

        // Check for background execution
        if (i > 0 && strcmp(args[i - 1], "&") == 0) {
            *is_background = 1;
            free(args[i - 1]); // Remove the "&" from args
            args[i - 1] = NULL;
            break;
        }
    }

    args[i] = NULL; // NULL-terminate the array
    return args;
}

void expand_environment_variables(char ***args_ptr) {

    char **args = *args_ptr;

    // Allocate memory for a new array to store the expanded token
    char **new_args = malloc(MAX_ARGS * sizeof(char *));
    if (!new_args) {
        perror("malloc");
        return;
    }

    int new_i = 0;

    // Iterate over the original args array
    for (int i = 0; args[i] != NULL && new_i < MAX_ARGS - 1; i++) {
        // Check if the token starts with '$' (indicating an environment variable)
        if (args[i][0] == '$') {

            // Get the value of the environment variable (skip the '$' character)
            char *env_val = getenv(args[i] + 1);

            if (env_val) {
                // Make a copy of the environment variable value
                char *env_copy = strdup(env_val);
                if (!env_copy) {
                    perror("strdup");
                    continue;
                }

                // Use strtok to split env_copy on whitespace
                char *token = strtok(env_copy, " ");
                while (token != NULL && new_i < MAX_ARGS - 1) {

                    // Add each token to the new_args array
                    new_args[new_i++] = strdup(token);

                    // Get the next token
                    token = strtok(NULL, " ");
                }
                free(env_copy);
            } 
            else {
                // If variable not found, insert an empty string or ignore it
                new_args[new_i++] = strdup("");
            }
        } 
        else {
            // Not an environment variable; copy as is.
            new_args[new_i++] = strdup(args[i]);
        }
    }
    new_args[new_i] = NULL;
    
    // Free the old tokens
    for (int i = 0; args[i] != NULL; i++) {
        free(args[i]);
    }
    *args_ptr = new_args;
}

// ----------------------------
// External Commands
// ----------------------------
void execute_command(char **args, int is_background) {
    // Expand environment variables in args,
    // and update args with the new token array.
    expand_environment_variables(&args);

    pid_t pid = fork(); // Create a child process

    if (pid < 0) {
        perror("fork failed");
        return;

    } 
    else if (pid == 0) { // Child process
        execvp(args[0], args); // Execute the command
        perror("execvp failed"); // Only reached if execvp fails
        exit(EXIT_FAILURE);
    } 
    else if (!is_background) { // Parent process (foreground)
        waitpid(pid, NULL, 0); // Wait for the child to finish
    } 
    else { // Parent process (background)
        printf("[%d]\n", pid); // Print PID of background process
    }

    // Free the new args array after execution
    for (int i = 0; args[i] != NULL; i++) {
        free(args[i]);
    }
    
}


// ----------------------------
// Main Program
// ----------------------------
int main(void) {

    // Open the log file for writing (create it if it doesn't exist, append if it does)
    log_fd = open("shell.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd == -1) {
        perror("open"); // Print an error if the file cannot be opened
        return EXIT_FAILURE; // Exit the program with a failure status
    }

    // Register the SIGCHLD signal handler to handle child process termination
    register_child_signal();

    // Buffer to store user input
    char input[MAX_INPUT_LEN];

    // Main shell loop
    while (1) {

        // Print the shell prompt
        printf("MyShell:) ");
        fflush(stdout); // Ensure the prompt is displayed immediately

        // Read user input
        if (!fgets(input, MAX_INPUT_LEN, stdin)) break; // Exit if input reading fails (e.g., EOF)
        input[strcspn(input, "\n")] = '\0'; // Remove the newline character from the input

        // Skip empty input
        if (!input[0]) continue;

        // Flag to indicate if the command should run in the background
        int is_background = 0;

        // Parse the input into an array of arguments (tokens)
        char **args = parse_input(input, &is_background);
        if (!args[0]) continue; // Skip if no command is provided

        // Handle built-in commands or execute external commands
        if (strcmp(args[0], "exit") == 0) {
            break; // Exit the shell if the command is "exit"
        } else if (strcmp(args[0], "cd") == 0) {
            handle_cd(args); // Handle the "cd" command
        } else if (strcmp(args[0], "echo") == 0) {
            handle_echo(args); // Handle the "echo" command
        } else if (strcmp(args[0], "export") == 0) {
            handle_export(args); // Handle the "export" command
        } else {
            execute_command(args, is_background); // Execute external commands
        }

        // Free the memory allocated for the arguments array
        free(args);
    }

    // Close the log file
    close(log_fd);

    // Exit the program successfully
    return 0;
}