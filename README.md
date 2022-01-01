## Project Name & Description

Interactive shell program written in C.

This is a replica of the Bash shell written in C. This shell supports a limited subset of commands supported by the BASH shell, and also supports the execution of programs as both foreground and background processes. 

Command line parsing ensures that all user input is correctly handled by the shell. Users can enter any supported commands (supported commands are detailed below), redirection tokens, and any amount of whitespace and the shell will correctly execute the input. The shell supports input and output file redirection, allowing the shell to feed input to a user program from a file and direct its output into another file.

In addition, this shell implements a basic job control system. The shell is able to handle multiple processes by running them in the background (and jobs can be switched to run in the foreground or the background). Signal forwarding is implemented so that if SIGINT, SIGTSTP or SIGQUIT signals are sent into the shell (by typing CTRL-C, CTRL-Z, or CTRL-/ respectively), the signal is sent to the currently running foreground job. If no foreground job is running, then nothing happens (identical to the behavior of the BASH shell). All jobs that are started by the shell and have been terminated are reaped. Whenever a job is terminated by a signal, the shell prints a message indicating the cause of termination.

## Project Status

This project is completed

## Project Screen Shot(s)

#### Example:   

![ScreenShot](https://raw.github.com/singhru27/Shell/main/screenshots/Home.png)

## Installation and Setup Instructions

To build the program, run the command

```
make all
```

which compiles the shell. To run the shell, run the command

```
./33sh
```
The shell supports a few basic default commands. These commands are as follows:

```
cd <Bash command file paths for command>: Changes the working directory
ln <src> <dest> : Makes a hard link to a file
rm <file>: Removes the file from the directory
jobs: Lists all the current jobs, listing each job's job ID, state, and command used to execute it
bg %<job> resumes <job> (if it is suspended) and runs it in the background
fg %<job> resumes <job> (if it is suspended) and runs it in the foreground
exit: Exits the shell
```

Flags are currently NOT supported by this shell. 

This shell can also execute commands in the background or the foreground. If a command ends with the character "&", the command will be run in the foreground. The "&" character must be the last thing on the command line. When a job is started in the background, a message indicating the job and process ID is printed to standard output in the following format:

```
[<job id>] (<process id>)
```

Whenever a job is stopped by a signal, the following message is printed:

```
[<job id>] (<pid>) suspended by signal <signal number>

```

Whenever a job is resumed via a SIGCONT signal, the following message is printed:

```
[<job id>] (<pid>) resumed

```
Whenever a background job exits normally, the following message is printed
```
[<job id>] (<pid>) terminated with exit status <status>

```
