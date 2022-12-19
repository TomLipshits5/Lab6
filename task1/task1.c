#include "linux/limits.h"
#include <unistd.h>
#include <stdio.h>
#include "../LineParser.c"
#include <stdlib.h>
#include "errno.h"
#include <sys/wait.h>


void printCwd(){
    char cwd[PATH_MAX];
    if(getcwd(cwd, sizeof(cwd)) == NULL){
        perror("getcwd() Error");
    }else{
        printf("Shell is running in %s:\n", cwd);
    }
}

int isExitCommand(cmdLine* cmd){
    return strcmp(cmd->arguments[0], "quit") == 0;
}

int isDebug(cmdLine* cmd){
    int i = 0;
    while(cmd-> arguments[i] != NULL){
        if(strcmp(cmd-> arguments[i], "-d") == 0){
            return 1;
        }
        i++;
    }
    return 0;
}

int isCdCommand(cmdLine* cmd){
    return strcmp("cd", cmd->arguments[0]) == 0;
}

void execute(cmdLine* cmd){
    //Handle cd command
    if(isCdCommand(cmd)){
        chdir(cmd->arguments[1]);
        return;
    }
    int pid = fork();
    char* executingCmd = cmd->arguments[0];
    //Handle debug mode
    if(isDebug(cmd)  && pid != 0){
        fprintf(stderr,"Process ID: %i, Executed Command: %s\n",pid, executingCmd);
    }
    //Execute commands on child
    if(pid == 0){
        if(execvp(executingCmd, cmd->arguments) < 0){
            perror("Error executing command\n");
            _exit(errno);
        }
    }
    //Wait for blocking commands
    if(cmd->blocking){
        int status;
        printf("waiting for process: %i\n", pid);
        waitpid(pid, &status, 0);
    }

}

int main(int argc, char **argv){
    printCwd();
    char userInput[2048];
    while(1){
        if(fgets(userInput, sizeof(userInput), stdin) != NULL){
            struct cmdLine* currentCmd = parseCmdLines(userInput);
            if (isExitCommand(currentCmd)){
                freeCmdLines(currentCmd);
                break;
            }
            execute(currentCmd);
            freeCmdLines(currentCmd);
        }else{
            perror("Error reading command from stdin");
            break;
        }

    }
    printf("closed shell normally\n");
    return 0;
}

