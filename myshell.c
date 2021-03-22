/****************************************************************
 * Name        :                                                *
 * Class       :  CSC 415                                       *
 * Date        :                                                *
 * Description :  Writting a simple shell program               *
 *                that will execute simple commands. The main   *
 *                goal of the assignment is working with        *
 *                fork, pipes and exec system calls.            *
 ****************************************************************/

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
/* CANNOT BE CHANGED */
#define BUFFERSIZE 256
/* ------------------*/
#define MAXCWDSIZE 256
#define ARGVSIZE 32
#define PROMPT "myShell "
#define FOREVER for(;;)
#define DELIMS " \t\n"

void executeSecondPipe(int rightArgStartIndex, int retVal, int pipe_fds[2], pid_t id2, char *arg_vector[ARGVSIZE]);

void executeFileCommands(char *arg_vector[ARGVSIZE], int arg_count);

void executeDirectoryCommands(char **arg_vector, int arg_count, char cwd[MAXCWDSIZE], bool *executeDirectoryCmd);

int
main(int argc, char** argv)
{
    pid_t id;
    pid_t id2;
    char cwd[MAXCWDSIZE];
    char buffer[BUFFERSIZE];
    char* arg_vector[ARGVSIZE];
    char* temp_arg;
    int pipe_fds[2];
    int arg_count;
    int rightArgStartIndex = 0;
    int retVal;
    bool executeDirectoryCmd = false;
    bool runInBackground = false;

    FOREVER {
        printf("%s %s >> ", PROMPT, getcwd(cwd, sizeof(cwd)));

        if (!fgets(&buffer[0], BUFFERSIZE, stdin)) {
            perror("error reading from user");
            exit(-1);
        }

        //Tokenize arguments into one arg_vector array
        arg_count = 0;
        temp_arg = strtok(buffer, DELIMS);
        while (temp_arg != NULL) {
            arg_vector[arg_count++] = temp_arg;
            temp_arg = strtok(NULL, DELIMS);
        }
        arg_vector[arg_count] = NULL;

        //Replace all pipes with NULLs and save the index in which the pipe occurs
        rightArgStartIndex = 0;
        for (int i = 0; i < arg_count; i++) {
            if (strcmp(arg_vector[i], "|") == 0) {
                arg_vector[i] = NULL;
                rightArgStartIndex = i + 1;
                break;
            }
        }

        //Check if a command to change directory or print working directory was input
        //If that is the case, [continue;] to go back up to PROMPT again
        if (arg_count > 0) {
            executeDirectoryCommands(arg_vector, arg_count, cwd, &executeDirectoryCmd);
            if(executeDirectoryCmd == true){
                executeDirectoryCmd = false;
                continue;
            }
        }

        //Checks if '&' flag is last argument input by user to run cmd in background
        if (strcmp(arg_vector[arg_count - 1], "&") == 0) {
            runInBackground = true;
            arg_vector[arg_count-1] = NULL;
        }

        id = fork();

        if (id == 0) {
            //If there is an existing pipe index, execute pipe function
            if(rightArgStartIndex > 0){
                executeSecondPipe(rightArgStartIndex,retVal,pipe_fds,id2,arg_vector);
            }
            //If at least 3 commands were passed, check for file modifiers '>, >>, <'
            if (arg_count > 2) {
                executeFileCommands(arg_vector, arg_count);
            }
            //Execute commands
            execvp(arg_vector[0], &arg_vector[0]);
            perror("Try another command. Execvp in Child failed\n");
        }
        else if (id > 0) {
            //if & present, parent does not wait. If & not present, parent waits.
            if (!runInBackground) {
                wait(NULL);
            }
        }
        else {
            perror("Fork has failed");
            exit(-1);
        }
    }

    return 0;
}

/**
 * Executes shell command of changing directory or print working directory based on command line arguments.
 * @param arg_vector the vector holding all arguments input in shell.
 * @param arg_count the number of arguments input.
 * @param cwd the char array holding the string location of the current working directory to display.
 * @param executeDirectoryCmd boolean value to let main() know if we go back to beginning.
 */
void executeDirectoryCommands(char **arg_vector, int arg_count, char cwd[MAXCWDSIZE], bool *executeDirectoryCmd) {
    if (arg_count > 1) {
        if (strcmp(arg_vector[arg_count - 2], "cd") == 0) {
            *executeDirectoryCmd = true;
            chdir(arg_vector[arg_count - 1]);
        }
    }

    if (strcmp(arg_vector[arg_count - 1], "pwd") == 0) {
        *executeDirectoryCmd = true;
        printf("%s\n", getcwd(cwd, MAXCWDSIZE));
    }
}

/**
 * Executes commands to write to, append, or read from files.
 * @param arg_vector the vector holding all arguments input in shell.
 * @param arg_count the number of arguments input.
 */
void executeFileCommands(char *arg_vector[ARGVSIZE], int arg_count) {
    int fp;

    if (strcmp(arg_vector[arg_count - 2], ">") == 0) {

        fp = open(arg_vector[arg_count - 1], O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);

        arg_vector[arg_count - 1] = NULL;
        arg_vector[arg_count - 2] = NULL;

        dup2(fp, 1);
        close(fp);
    } else if (strcmp(arg_vector[arg_count - 2], ">>") == 0) {
        fp = open(arg_vector[arg_count - 1], O_CREAT | O_WRONLY | O_APPEND, S_IRWXU);

        arg_vector[arg_count - 1] = NULL;
        arg_vector[arg_count - 2] = NULL;

        dup2(fp, 1);
        close(fp);
    } else if (strcmp(arg_vector[arg_count - 2], "<") == 0) {
        fp = open(arg_vector[arg_count - 1], O_RDONLY);

        arg_vector[arg_count - 1] = NULL;
        arg_vector[arg_count - 2] = NULL;

        dup2(fp, 0);
    }
}

/**
 * Executes a second fork() for piping commands then returns back to main parent.
 * @param rightArgStartIndex the index where the first pipe was found.
 * @param retVal contains the value of our pipe_fds creation. -1 means it failed to create.
 * @param pipe_fds contains file descriptors to either read or write to pipe.
 * @param id2 our second process id tag for our second fork().
 * @param arg_vector the vector holding all arguments input in shell.
 */
void executeSecondPipe(int rightArgStartIndex, int retVal, int pipe_fds[2], pid_t id2, char *arg_vector[ARGVSIZE]) {
    //If Piping, fork() again and execute on child
    //Creates pipe process and parent fork()s a child process
    retVal = pipe(pipe_fds);
    if (retVal == -1) {
        perror("Error making pipes");
        exit(-1);
    }

    id2 = fork();

    if (id2 == -1) {
        perror("error making second fork()");
        exit(-1);
    } else if (id2 == 0) {
        close(0); //close stdin b/c we are not taking user input
        dup(pipe_fds[0]); //copies given file descriptor into the highest available spot (0)
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        execvp(arg_vector[rightArgStartIndex], &arg_vector[rightArgStartIndex]);
    } else {
        close(1); //output go to right end of the pipe, closes stdout
        dup(pipe_fds[1]); //copies given file descriptor into the highest available spot (1)
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        execvp(arg_vector[0], &arg_vector[0]);
    }

}
