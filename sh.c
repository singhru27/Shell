#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include "./jobs.h"

#define INPUT_REDIRECTION 0
#define INPUT_REDIRECTION_FILE 1
#define OUTPUT_REDIRECTION 2
#define OUTPUT_REDIRECTION_FILE 3

// Function Declarations

int parse_input(char *argv[], char *redirect[], int *argc,
                job_list_t *job_list);
int execute_built_in_commmands(char *argv[], int *argc, job_list_t *job_list);
int run_executable(char *argv[], char *redirect[], int *argc,
                   job_list_t *job_list);
void ignore_signals();
void restore_signals();
void reap_children(job_list_t *job_list);
int job_id = 1;

// Declaring the buffer to hold the contents of the file that is read in as a
// global variable, as well as arrays with the arguments and redirection tokens,
// and an integer representing the number of total
char buffer[1024] = {0};

int main() {
    int parse_result;
    int built_in_command_result = 0;
    int run_executable_result = 0;

    // Initializing the job list
    job_list_t *job_list = init_job_list();

    // This continues the program indefinitely
    while (1) {
        // argc is reset to 0 upon every iteration of the program
        int argc = 0;

        // These are two buffers which holds the parsed tokens from the input in
        // standard input, and the redirect commands as well as the file to
        // redirect to.
        char *argv[512] = {0};
        char *redirect[4] = {0};

        // This sets up signal handlers to ignore signals sent to the shell
        // ignore_signals ();
        ignore_signals();

        // This reaps child processes prior to the printing of the prompt
        reap_children(job_list);

        // This prints out the prompt if the macro is defined properly, and
        // error checks the syscall
#ifdef PROMPT
        if (printf("33sh> ") < 0) {
            fprintf(stderr, "Error printing prompt\n");
        }
        if (fflush(stdout) != 0) {
            fprintf(stderr, "Error printing prompt\n");
        }
#endif

        // This function call parses the input, and inserts all tokens into the
        // argv and redirect arrays. It error checks for incorrect usage of
        // redirection, and returns a 1 if there are no redirection syntax
        // errors and the input file was successfully parsed
        parse_result = parse_input(argv, redirect, &argc, job_list);

        // If the parsing detected incorrect input or no input, the shell goes
        // to a new line and rewaits for user input. It also resets the redirect
        // and argv
        if (parse_result != 1) {
            continue;
        }

        // If the redirection symbols were correctly inputted, the program then
        // checks for the built in commands. If these commands are found, it
        // executes these commands, provided there was correct input. The
        // function returns a 1 if a builtin command was executed, a 0 if no
        // built-in commands were found (indicating that the inputted argument
        // is a path to an executable), or a -1 if there was a syntax error
        built_in_command_result =
            execute_built_in_commmands(argv, &argc, job_list);

        // If the above function executed a built in command or returned a
        // syntax error, the shell goes to a new line and rewaits for user
        // input.

        if (built_in_command_result != 0) {
            continue;
        }

        // If this code is being reached, this means that there is an argument
        // passed into the stdin which is not one of the supported built-in
        // commands. Thus, we must attempt to execute the executable which argv
        // points to, passing in all subsequent arguments as well.

        run_executable_result = run_executable(argv, redirect, &argc, job_list);

        // If the agove function returned an error, the loop is re-entered
        if (run_executable_result == -1) {
            continue;
        }
    }

    return 0;
}

// This function is used to reap all children and update statuses accordingly,
// prior to the printing of the prompt

void reap_children(job_list_t *job_list) {
    int status = 0;
    pid_t child_pid = 0;

    while ((child_pid =
                waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        // Checking for normal process termination
        if (WIFEXITED(status)) {
            printf("[%d] (%d) terminated with exit status %d\n",
                   get_job_jid(job_list, child_pid), child_pid,
                   WEXITSTATUS(status));

            // Removing the process from the jobs list
            remove_job_pid(job_list, child_pid);
        }

        // Checking for process termination via a signal
        if (WIFSIGNALED(status)) {
            printf("[%d] (%d) terminated by signal %d\n",
                   get_job_jid(job_list, child_pid), child_pid,
                   WTERMSIG(status));

            // Removing the process from the jobs list
            remove_job_pid(job_list, child_pid);
        }

        // Checking for process suspension via signal
        if (WIFSTOPPED(status)) {
            printf("[%d] (%d) suspended by signal %d\n",
                   get_job_jid(job_list, child_pid), child_pid,
                   WSTOPSIG(status));

            // Updating the enum to STOPPED
            update_job_pid(job_list, child_pid, STOPPED);
        }

        // Checking if a process was resumed via signal
        if (WIFCONTINUED(status)) {
            printf("[%d] (%d) resumed\n", get_job_jid(job_list, child_pid),
                   child_pid);

            // Updating the enum to RUNNING
            update_job_pid(job_list, child_pid, RUNNING);
        }
    }

    // Error checking the system call
    if (child_pid == -1) {
        // Checking if the error was due to no processes currently being run, in
        // which case the error is ignored
        if (errno == ECHILD) {
            return;
        }

        // If the error was more serious, the error is printed out
        perror("wait");
    }
}
// This function is used to restore the signal handlers to their default
// values when the child process is forked off

void restore_signals() {
    if (signal(SIGQUIT, SIG_DFL) == SIG_ERR) {
        perror("SIGQUIT");
    }
    if (signal(SIGINT, SIG_DFL) == SIG_ERR) {
        perror("SIGINT");
    }
    if (signal(SIGTSTP, SIG_DFL) == SIG_ERR) {
        perror("SIGTSTP");
    }
    if (signal(SIGTTOU, SIG_DFL) == SIG_ERR) {
        perror("SIGTTOU");
    }
}

// This function is used to set the shell to ignore the SIGQUIT, SIGINT , and
// SIGTSTP, and SIGTTOU commands

void ignore_signals() {
    if (signal(SIGQUIT, SIG_IGN) == SIG_ERR) {
        perror("SIGQUIT");
    }
    if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
        perror("SIGINT");
    }
    if (signal(SIGTSTP, SIG_IGN) == SIG_ERR) {
        perror("SIGTSTP");
    }
    if (signal(SIGTTOU, SIG_IGN) == SIG_ERR) {
        perror("SIGTTOU");
    }
}

// This function is used to run the executable, assuming that no built-in
// function was called. It returns 1 if the program was correctly executed, and
// -1 if the program was unable to be executed

int run_executable(char *argv[], char *redirect[], int *argc,
                   job_list_t *job_list) {
    // To perform the execv command, we need to parse out an arg array, and
    // create a pointer to the full path of the executable.
    char file_path[1024];
    char tokenized_final_path[1024];

    // The buffer pointer is used to point to the final path component of the
    // path to the program, and the delimiter is used to separate out
    // intermediate path components. The temporary pointer is used to cycle
    // through the tokenized_final_path array
    char *buffer_pointer = NULL;
    char *temporary_pointer = NULL;
    char *delimiter = "/";

    // This copies the file path into two different character arrays. One is
    // used to hold the filepath, while the other is used to tokenize the first
    // argument to retrieve the final path component of the path to the program
    strcpy(file_path, argv[0]);
    strcpy(tokenized_final_path, argv[0]);

    // This tokenizes the first argument of the argv array, and places the
    // final path component of argv in argv[0]
    temporary_pointer = strtok(tokenized_final_path, delimiter);

    while (temporary_pointer != NULL) {
        buffer_pointer = temporary_pointer;
        temporary_pointer = strtok(NULL, delimiter);
    }

    // This sets the first element of the argv array to be the final path name
    argv[0] = buffer_pointer;

    // This calls the fork command, creating a new child process with the file
    // path and the argument vector. If it fails, an error message is printed
    // and the child process exits If it succeeds, the parent resumes execution
    // upon completion.
    pid_t child_pid;

    if ((child_pid = fork()) == 0) {
        // Setting the process group ID of the child process to its process ID
        if (setpgid(0, 0) == -1) {
            perror("setpgid");
            exit(1);
        }

        // If the & specifier was included when executing the program, the
        // program is run in the background.
        if (strcmp(argv[*argc - 1], "&") == 0) {
            // Removing the & symbol from the argv array
            argv[*argc - 1] = NULL;
            (*argc)--;

            // Setting the child process to be the foreground process by
            // changing the process group ID of standard input to that of the
            // child, if the & specifier was not included

        } else {
            if (tcsetpgrp(0, getpgrp()) == -1) {
                perror("tcsetpgrp");
                exit(1);
            }
        }

        // Restoring signal functionality in the child process
        restore_signals();

        // This handles the input  redirection. If the input redirection index
        // in the redirect array is not NULL, the input is redirected to the
        // specified file. If this fails, an error is printed and the function
        // exits
        if (redirect[INPUT_REDIRECTION] != NULL) {
            // Closing the current open file descriptor of stdin
            close(0);

            // Opening the specified INPUT_REDIRECTION_FILE, and printing an
            // error message if it does not exist while also exiting from the
            // program,
            if (open(redirect[INPUT_REDIRECTION_FILE], O_RDONLY) == -1) {
                perror(buffer_pointer);
                exit(1);
            }
        }

        // This handles input redirection for >. If the output in the redirect
        // array is this token, the output is redirected to the specified file.
        // If this fails, an error is printed and the function exits

        if (redirect[OUTPUT_REDIRECTION] != NULL) {
            if (strcmp(redirect[OUTPUT_REDIRECTION], ">") == 0) {
                // Closing the current open file descriptor of stdout
                close(1);

                // Opening the specified OUTPUT_REDIRECTION_FILE, and printing
                // an error message if there is an error in this function. An
                // error also causes program termination
                if (open(redirect[OUTPUT_REDIRECTION_FILE],
                         O_RDWR | O_TRUNC | O_CREAT, 0777) == -1) {
                    perror(buffer_pointer);
                    exit(1);
                }
            }

            if (strcmp(redirect[OUTPUT_REDIRECTION], ">>") == 0) {
                // Closing the current open file descriptor of stdout
                close(1);

                // Opening the specified OUTPUT_REDIRECTION_FILE, and printing
                // an error message if there is an error in this function. An
                // error also causes program termination
                if (open(redirect[OUTPUT_REDIRECTION_FILE],
                         O_RDWR | O_APPEND | O_CREAT, 0777) == -1) {
                    perror(buffer_pointer);
                    exit(1);
                }
            }
        }
        execv(file_path, argv);

        // This handles the case in which the execv command fails to execute
        perror("execv");
        exit(1);
    }

    // This prints out the job id and the process id of any job that was started
    // in the background, checking the argv array to determine if the program
    // was launched in the background
    if (strcmp(argv[(*argc) - 1], "&") == 0) {
        add_job(job_list, job_id, child_pid, RUNNING, file_path);
        printf("[%d] (%d)\n", job_id, child_pid);
        job_id++;

        // If the process was launched in the foreground, the shell waits for it
        // to finish execution before continuing the REPL
    } else {
        int status;

        if (waitpid(child_pid, &status, WUNTRACED) == -1) {
            perror("wait");
        }

        // Setting the shell  to be the foreground process by changing
        // the process group ID of standard input to that of the shell
        if (tcsetpgrp(0, getpgrp()) == -1) {
            perror("tcsetpgrp");
            return -1;
        }

        // If the process was terminated by a signal, the job ID and the process
        // ID of the process are printed to the terminal
        if (WIFSIGNALED(status)) {
            printf("[%d] (%d) terminated by signal %d\n", job_id, child_pid,
                   WTERMSIG(status));
            job_id++;
        }

        // If the process was stopped by a signal, the job ID and the process ID
        // of the process are printed to the terminal. The job is then added
        // to the job list

        if (WIFSTOPPED(status)) {
            printf("[%d] (%d) suspended by signal %d\n", job_id, child_pid,
                   WSTOPSIG(status));
            add_job(job_list, job_id, child_pid, STOPPED, file_path);
            job_id++;
        }
    }

    return 1;
}

// This function is used to check the argument array for built in commands. If
// it finds a built-in command, it performs error checking to ensure that the
// correct number of arguments were passed in. If there are not the correct
// number of arguments passed in, the function returns with code -1 and prints
// an error message. If the given argument is not a built-in command, the
// function assumes that it is a path to an executable and returns with code 0.

int execute_built_in_commmands(char *argv[], int *argc, job_list_t *job_list) {
    if (*argc == 0) {
        fprintf(stderr, "%s", "redirects with no command\n");
        return -1;
    }

    // This exits the shell if the exit command was passed in as the first
    // argument to the argv array
    if (strcmp(argv[0], "exit") == 0) {
        cleanup_job_list(job_list);
        exit(0);
    }

    // This checks if the command is "cd". If it is, it first error checks to
    // ensure that there is an argument representing the directory inputed by
    // the user. If there is not, it returns an error and exits with 0. If there
    // is, it attempts to change the directory to the specified directory
    if (strcmp(argv[0], "cd") == 0) {
        if (argv[1] == NULL) {
            fprintf(stderr, "%s", "cd: syntax error\n");
            return -1;
        }

        // The chdir system call is executed. If a valid path was input by
        // the user, the directory will be changed and execute_built_in_commands
        // returns 1. If a valid directory was not input by the user, the
        // appropriate error message is printed, and a -1 is returned to the
        // user
        if (chdir(argv[1]) == -1) {
            perror("cd");
            return -1;
        }

        // Returning a 1 if the directory was successfully changed
        return 1;
    }

    // This checks if the command is "rm". If it is, it first error checks to
    // ensure that there is an argument representing the directory inputed by
    // the user. If there is not, it returns an error and exits with 0. If
    // there is, it attempts to remove the specified file
    if (strcmp(argv[0], "rm") == 0) {
        if (argv[1] == NULL) {
            fprintf(stderr, "%s", "rm: syntax error\n");
            return -1;
        }

        // The unlink system call is executed. If a valid path was input by
        // the user, the specified file will be removed and
        // execute_built_in_commands returns 1. If a valid directory was not
        // input by the user, the appropriate error message is printed and a -1
        // is returned to the user

        if (unlink(argv[1]) == -1) {
            perror("rm");
            return -1;
        }

        // Returning a 1 if the file was successfully removed
        return 1;
    }

    // This checks if the command is "ln". If it is, it first error checks to
    // ensure that there are two arguments representing the existing path
    // and the new path. If there are not two arguments, it returns an error
    // and exits with 0. If there is, it attempts to create the appropriate
    // linkage
    if (strcmp(argv[0], "ln") == 0) {
        if (argv[1] == NULL || argv[2] == NULL) {
            fprintf(stderr, "%s", "ln: syntax error\n");
            return -1;
        }

        // The link system call is executed. If a valid path was input by the
        // user, the old path in argv[1] will be linked to the new path in the
        if (link(argv[1], argv[2]) == -1) {
            perror("ln");
            return -1;
        }

        // Returning a 1 if the linke operation was successfully executed
        return 1;
    }

    // This checks if the command is "jobs". If it is, it first error checks
    // to ensure that there are no other commands besides "jobs" input
    // into the terminal. If the error-checking passes, the jobs are printed
    // to standard output

    if (strcmp(argv[0], "jobs") == 0) {
        if (argv[1] != NULL) {
            fprintf(stderr, "%s", "jobs: syntax error\n");
            return -1;
        }

        // Calling the jobs function if there was no error
        jobs(job_list);
        return 1;
    }

    // This checks of the command is "bg". If it is, it first checks to ensure
    // that the correct arguments have been inputed into the function. If the
    // correct arguments were input, the program attempts to find the
    // appropriate process in the job_list, and send a continue signal to that
    // process

    if (strcmp(argv[0], "bg") == 0) {
        // Checking to ensure that only 1 argument was passed in
        if (argv[1] == NULL || argv[2] != NULL) {
            fprintf(stderr, "%s", "bg: syntax error\n");
            return -1;
        }

        // Extracting the job ID from the argv[0]
        char job_id_number[1024] = {0};
        int child_job_id = 0;
        pid_t child_pid = 0;
        strcpy(job_id_number, argv[1]);

        // If the first character in the first argument is not a % sign,
        // an error is thrown

        if (job_id_number[0] != '%') {
            fprintf(stderr, "%s", "bg: job input does not begin with %\n");
            return -1;
        }

        child_job_id = atoi(job_id_number + 1);

        // Checking to ensure that a valid job_id was input
        if ((child_pid = get_job_pid(job_list, child_job_id)) == -1) {
            fprintf(stderr, "%s", "job not found\n");
            return -1;
        }

        // This sends the SIGCONT signal to the entire process group in
        // question, then updates the status in the jobs list

        kill(-child_pid, SIGCONT);
        update_job_pid(job_list, child_pid, RUNNING);
        return 1;
    }

    // This checks if the command is "fg". If it is, it first checks to ensure
    // that the correct arguments have been inputed into the funciton. If the
    // correct arguments were input, the program attempts to find the
    // appropriate process in the job_list, and send a continue signal to the
    // process.

    if (strcmp(argv[0], "fg") == 0) {
        // Checking to ensure that only 1 argument was passed in
        if (argv[1] == NULL || argv[2] != NULL) {
            fprintf(stderr, "%s", "fg: syntax error\n");
            return -1;
        }

        // Extracting the job ID from the argv[0]
        char job_id_number[1024] = {0};
        int child_job_id = 0;
        pid_t child_pid = 0;
        strcpy(job_id_number, argv[1]);

        // If the first character in the first argument is not a % sign,
        // an error is thrown

        if (job_id_number[0] != '%') {
            fprintf(stderr, "%s", "fg: job input does not begin with %\n");
            return -1;
        }

        child_job_id = atoi(job_id_number + 1);

        // Checking to ensure that a valid job_id was input
        if ((child_pid = get_job_pid(job_list, child_job_id)) == -1) {
            fprintf(stderr, "%s", "job not found\n");
            return -1;
        }

        // This sends the SIGCONT signal to the entire process group in
        // question,
        kill(-child_pid, SIGCONT);

        // This sets the foreground process to be the resumed process group
        if (tcsetpgrp(0, getpgid(child_pid)) == -1) {
            perror("tcsetpgrp");
            return -1;
        }

        // This uses waitpid to wait for the child process to complete execution
        // before continuing
        int status;
        if (waitpid(child_pid, &status, WUNTRACED) == -1) {
            perror("wait");
        }

        // Setting the shell  to be the foreground process by changing
        // the process group ID of standard input to that of the shell
        if (tcsetpgrp(0, getpgrp()) == -1) {
            perror("tcsetpgrp");
            return -1;
        }

        // If the process was terminated by a signal, the job ID and the process
        // ID of the process are printed to the terminal
        if (WIFSIGNALED(status)) {
            printf("[%d] (%d) terminated by signal %d\n", child_job_id,
                   child_pid, WTERMSIG(status));
            remove_job_pid(job_list, child_pid);
        }

        // If the process was stopped by a signal, the job ID and the process ID
        // of the process are printed to the terminal. The job is then added
        // to the job list

        if (WIFSTOPPED(status)) {
            printf("[%d] (%d) suspended by signal %d\n", child_job_id,
                   child_pid, WSTOPSIG(status));
            update_job_jid(job_list, child_job_id, STOPPED);
        }

        // If the process completed normally, the process is removed from the
        // job_list
        if (WIFEXITED(status)) {
            remove_job_pid(job_list, child_pid);
        }

        // End of new code

        return 1;
    }

    return 0;
}

// This function is used to parse the input in standard input that was passed in
// by the user. It returns -1 if there was an erroneous input, 0 if nothing was
// input by the user, and 1 if the user input was valid and parsed correctly

int parse_input(char *argv[], char *redirect[], int *argc,
                job_list_t *job_list) {
    // This is a local variable used to hold the number of bytes which were read
    // into the buffer
    ssize_t end_of_buffer = 0;

    // These are local variables used to determine the total number of output
    // and input redirects
    int output_redirects = 0;
    int input_redirects = 0;

    // This resets the global buffer variable to null
    memset(buffer, 0, 1024);

    // This is a buffer which the read command reads into, and a pointer used by
    // strtok as well. The delimiter is set to be a space or a tab

    char *buffer_pointer = NULL;
    char *delimiter = " \t";

    // Reading in the input into the buffer
    end_of_buffer = read(0, buffer, 1024);

    // Error checking the read command
    if (end_of_buffer == -1) {
        perror("read");
        return -1;
    }

    // This exits the shell if the read command returns a 0, which means that
    // the CTRL_D comman was input by the user
    if (end_of_buffer == 0) {
        cleanup_job_list(job_list);
        exit(0);
    }

    // Setting the last character after all data has been read in from standard
    // input to null
    buffer[end_of_buffer - 1] = '\0';

    buffer_pointer = strtok(buffer, delimiter);

    // If buffer pointer is NULL, this means that a string of all spaces was
    // passed in, or that enter was pressed without actually inputting anything
    // into the terminal. In this case, parse_input exits with code 0

    if (buffer_pointer == NULL) {
        return 0;
    }

    // This tokenizes the buffer, inserting all commands into the argv array and
    // all redirect symbols as well as redirect pathnames into the redirect
    // array. This array is used within the main function. This also performs
    // extensive error checking on the user input into the command line. The
    // token to be used in the function is " " since all commands must be
    // separated by white space

    while (buffer_pointer != NULL) {
        // If the buffer pointer is a left redirect, the function inserts it
        // into the redirect array at index 0, then inserts the following token
        // as the next element in the redirect array. If the next argument is
        // another redirect symbol, or does not exist, the function returns an
        // error message and returns with code 0 to refresh the shell

        if (strcmp(buffer_pointer, "<") == 0) {
            redirect[INPUT_REDIRECTION] = buffer_pointer;
            input_redirects++;
            buffer_pointer = strtok(NULL, delimiter);

            // If there is more than one redirect of the same type, the shell
            // prints an error code and exits with code 0, so that the shell can
            // refresh
            if (input_redirects > 1) {
                fprintf(stderr, "%s", "syntax error: multiple input files\n");
                return -1;
            }

            // Printing an error if there is no input following the redirection
            // symbol
            if (buffer_pointer == NULL) {
                fprintf(stderr, "%s", "syntax error: no input file\n");
                return -1;
            }

            // Printing an error if the input following the redirection symbol
            // is another redirection symbol

            if (strcmp(buffer_pointer, "<") == 0 ||
                strcmp(buffer_pointer, ">") == 0 ||
                strcmp(buffer_pointer, ">>") == 0) {
                fprintf(stderr, "%s",
                        "syntax error: input file is a redirection symbol\n");
                return -1;
            }

            // If there is valid input following the input redirection, it is
            // input into the redirect array at index 1, then iterates to the
            // next token
            redirect[INPUT_REDIRECTION_FILE] = buffer_pointer;
            buffer_pointer = strtok(NULL, delimiter);
            continue;
        }

        // If the buffer pointer is a output redirect, the function inserts it
        // into the redirect array at index 2, then inserts the following token
        // as the next element in the redirect array. If the next argument is
        // another redirect symbol, or does not exist, the function returns an
        // error message and returns with code 0 to refresh the shell

        if (strcmp(buffer_pointer, ">") == 0 ||
            strcmp(buffer_pointer, ">>") == 0) {
            redirect[OUTPUT_REDIRECTION] = buffer_pointer;
            buffer_pointer = strtok(NULL, delimiter);
            output_redirects++;

            // If there is more than one redirect of the same type, the shell
            // prints an error code and exits with code 0, so that the shell can
            // refresh
            if (output_redirects > 1) {
                fprintf(stderr, "%s", "syntax error: multiple output files\n");
                return -1;
            }

            // Printing an error if there is no input following the redirection
            // symbol
            if (buffer_pointer == NULL) {
                fprintf(stderr, "%s", "syntax error: no output file\n");
                return -1;
            }

            // Printing an error if the input following the redirection symbol
            // is another redirection symbol

            if (strcmp(buffer_pointer, "<") == 0 ||
                strcmp(buffer_pointer, ">") == 0 ||
                strcmp(buffer_pointer, ">>") == 0) {
                fprintf(stderr, "%s",
                        "syntax error: output file is a redirection symbol\n");
                return -1;
            }

            // If there is valid input following the input redirection, it is
            // input into the redirect array at spot 3, then iterates to the
            // next token
            redirect[OUTPUT_REDIRECTION_FILE] = buffer_pointer;
            buffer_pointer = strtok(NULL, delimiter);
            continue;
        }

        // If the passed in input was not a redirection symbol, it is presumed
        // to be a command or parameters and added to the argv array, which
        // holds the command in argv[0] and parameters in the subsequent array
        // indices. It increments *argc to keep a running count of the number of
        // arguments passed into the function

        argv[*argc] = buffer_pointer;
        (*argc)++;
        buffer_pointer = strtok(NULL, delimiter);
    }

    // Setting the last character in the argv array to null, once the while loop
    // has terminated
    argv[*argc] = NULL;

    // Returning 1 if the parsing was successfully completed
    return 1;
}
