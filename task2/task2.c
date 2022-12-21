#include "linux/limits.h"
#include <unistd.h>
#include <stdio.h>
#include "../LineParser.c"
#include <stdlib.h>
#include "errno.h"
#include <sys/wait.h>

#define TERMINATED  -1
#define RUNNING 1
#define SUSPENDED 0
#define PRINT_FORMAT "%s    %s  %s\n"

//Process Manager code
typedef struct process{
    cmdLine* cmd;                         /* the parsed command line*/
    pid_t pid; 		                  /* the process id that is running the command*/
    int status;                           /* status of the process: RUNNING/SUSPENDED/TERMINATED */
    struct process *next;	                  /* next process in chain */
} process;

process* my_process_list;

char* getStatusString(int status){
    if(status == -1){
        return "TERMINATED";
    }else if(status == 1){
        return "RUNNING";
    }else{
        return "SUSPENDED";
    }
}

int getProcessStatus(pid_t pid){
    int status;
    pid_t return_pid = waitpid(pid, &status, WNOHANG);
    if(return_pid == -1 || return_pid == pid){
        return TERMINATED;
    }else if(return_pid == 0){
        return RUNNING;
    }
    printf("%d\n",return_pid);
    return SUSPENDED;
}


void addProcess(process** process_list, cmdLine* cmd, pid_t pid){
    process *newProcess = calloc(1, sizeof(process));
    newProcess->cmd = cmd;
    newProcess->pid = pid;
    newProcess->status = getProcessStatus(pid);
    if(process_list != NULL){
        newProcess->next = *process_list;
    }
    *process_list = newProcess;
}

void printProcessListData(process** process_list){
    printf(PRINT_FORMAT, "PID", "COMMAND", "STATUS");
    if(*process_list != NULL){
        process* current = *process_list;
        char snum[32];
        while(current != NULL){
            sprintf(snum, "%d", current->pid);
            printf(PRINT_FORMAT, snum, current->cmd->arguments[0], getStatusString(current->status));
            current = current->next;
        }
    }
}


void freeProcessList(process* process_list){
    if(process_list != NULL){
        free(process_list->cmd);
        if(process_list->next != NULL){
            freeProcessList(process_list->next);
        }
    }
}

void updateProcessList(process **process_list){
    if(process_list != NULL){
        process* cur = (*process_list);
        while(cur != NULL){
            int status = getProcessStatus(cur->pid);
            if (cur->status != TERMINATED && status == TERMINATED ){
                cur -> status = TERMINATED;
            }
            cur = cur->next;
        }
    }
}

void updateProcessStatus(process** process_list, int pid, int status){
    process* cur = *process_list;
    while(cur != NULL && cur->pid != pid){
        cur = cur->next;
    }
    if(cur != NULL){
        cur->status = status;
    }
}

void cleanProcessList(process** process_list){
    process* prior = NULL;
    process* curr = *process_list;
    while (curr != NULL){
        if(curr->status == -1 && prior != NULL){
            prior->next = curr->next;
        }else if (curr->status == -1){
            *process_list = (*process_list)->next;
        }
        curr = curr->next;
    }
}

void suspendProcess(pid_t pid){
    kill(pid, SIGTSTP);
}

void terminateProcess(pid_t pid){
    kill(pid, SIGINT);
}

void wakeProcess(pid_t pid){
    kill(pid, SIGCONT);
}


void printProcessList(process** process_list){
    updateProcessList(process_list);
    printProcessListData(process_list);
    cleanProcessList(process_list);

}

//End of process manager code


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

int isSpacialCommand(cmdLine* cmd){
    if(strcmp("cd", cmd->arguments[0]) == 0){
        return 0;
    }else if(strcmp("procs", cmd->arguments[0]) == 0){
        return 1;
    }else if(strcmp("kill", cmd->arguments[0]) == 0){
        return 2;
    }else if(strcmp("suspend", cmd->arguments[0]) == 0){
        return 3;
    }else if(strcmp("wake", cmd->arguments[0]) == 0){
        return 4;
    }
    return -1;
}

void handleSpacialCommand(int cmdId, cmdLine* cmd){
    if(cmdId == 0){
        chdir(cmd->arguments[1]);
    }else if(cmdId == 1){
        printProcessList(&my_process_list);
    }else if(cmdId == 2){
        terminateProcess(atoi(cmd->arguments[1]));
        updateProcessStatus(&my_process_list, atoi(cmd->arguments[1]), TERMINATED);
    }else if(cmdId == 3){
        suspendProcess(atoi(cmd->arguments[1]));
        updateProcessStatus(&my_process_list, atoi(cmd->arguments[1]), SUSPENDED);
    }else if(cmdId == 4 ){
        wakeProcess(atoi(cmd->arguments[1]));
        updateProcessStatus(&my_process_list, atoi(cmd->arguments[1]), RUNNING);
    }
}

void execute(cmdLine* cmd){
    //Handle cd command
    int cmdId = isSpacialCommand(cmd);
    int pid = fork();
    char* executingCmd = cmd->arguments[0];
    //Handle debug mode
    if(isDebug(cmd)  && pid != 0){
        fprintf(stderr,"Process ID: %i, Executed Command: %s\n",pid, executingCmd);
    }
    //Execute commands on child
    if(pid == 0){
        if(cmdId == -1){
            if(execvp(executingCmd, cmd->arguments) < 0){
                perror("Error executing command\n");

            }
        }
        _exit(errno);
    }else if(cmdId != -1){
        handleSpacialCommand(cmdId,cmd);
        return;
    }else if (cmdId == -1 && pid != 0){
        addProcess(&my_process_list, cmd, pid);
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
                freeProcessList(my_process_list);
                break;
            }
            execute(currentCmd);
        }else{
            perror("Error reading command from stdin");
            break;
        }

    }
    printf("closed shell normally\n");
    return 0;
}