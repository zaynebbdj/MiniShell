#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_ARGS 16
#define BUFFER_SIZE 1024

// ctrl-C handler ( stop execution of a program)
void sigint_handler(int sig) {
    printf("\nUse 'exit' to quit the shell.\n$");
    fflush(stdout);
}

// return the number of argument
int parse_line(char *s, char *argv[]) {
    int count = 0;
    char *token = strtok(s, " \t\n"); // first token
    while (token != NULL && count < MAX_ARGS) { // all next tokens
        argv[count++] = token;
        token = strtok(NULL, " \t\n");
    }
    argv[count] = NULL;
    return count;
}

void execute_command(char *argv[]) {
    int fd;
    int i = 0;
    int pipe_pos = -1;
    int redirect_pos = -1;
    // find pipe position or redirection position in the arguments
    while (argv[i] != NULL) { 
        if (strcmp(argv[i], "|") == 0) {
            pipe_pos = i;
            break;
        }
        if (strcmp(argv[i], ">") == 0) {
            redirect_pos = i;
            break;
        }
        i++;
    }

    
    if (pipe_pos != -1) { // case with pipe, creation of two process communicating with each other
        argv[pipe_pos] = NULL;
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
        pid_t pid1 = fork();
        if (pid1 == 0) {            // first child process
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
            execvp(argv[0], argv);
            perror("execvp");
            exit(EXIT_FAILURE);
        }
        pid_t pid2 = fork();
        if (pid2 == 0) {             // second child process
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            execvp(argv[pipe_pos + 1], &argv[pipe_pos + 1]);
            perror("execvp");
            exit(EXIT_FAILURE);
        }
        close(pipefd[0]);
        close(pipefd[1]);
        wait(NULL);
        wait(NULL);
    } else if (redirect_pos != -1) {           // case redirection
        argv[redirect_pos] = NULL;
        fd = open(argv[redirect_pos + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);  // new output
        if (fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        pid_t pid = fork();
        if (pid == 0) {
            dup2(fd, STDOUT_FILENO); // stdout refers to the choosen output fd
            close(fd);
            execvp(argv[0], argv);   // execution
            perror("execvp");
            exit(EXIT_FAILURE);
        }
        close(fd);
        wait(NULL);
    } else {          //  command execution, in case of no pipe and no redirection
        pid_t pid = fork();
        if (pid == 0) {
            execvp(argv[0], argv);
            perror("execvp");
            exit(EXIT_FAILURE);
        }
        wait(NULL);
    }
}

int main() {
    char input[BUFFER_SIZE];
    char *argv[MAX_ARGS + 1];
    
    signal(SIGINT, sigint_handler); // signal hndler
    // minishell main loop
    while (1) {
        printf("$ ");
        fflush(stdout);
        
        if (fgets(input, BUFFER_SIZE, stdin) == NULL) {
            printf("\nExiting shell.\n");
            break;
        }
        // exit minishell
        if (strcmp(input, "exit\n") == 0) {
            break;
        }
        // find the number of argument
        int arg_count = parse_line(input, argv);
        // execute input
        if (arg_count > 0) {
            execute_command(argv);
        }
    }
    return 0;
}
