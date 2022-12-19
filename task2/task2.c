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
#define PRINT_FORMAT "%s    %s  %s"


typedef struct process{
    cmdLine* cmd;                         /* the parsed command line*/
    pid_t pid; 		                  /* the process id that is running the command*/
    int status;                           /* status of the process: RUNNING/SUSPENDED/TERMINATED */
    struct process *next;	                  /* next process in chain */
} process;

char* getStatusString(int status){
    if(status == -1){
        return "TERMINATED";
    }else if(status == 1){
        return "RUNNING";
    }else{
        return "SUSPENDED";
    }
}


void addProcess(process** process_list, cmdLine* cmd, pid_t pid){
    int status;
    process *newProcess = calloc(1, sizeof(process));
    newProcess->cmd = cmd;
    newProcess->pid = pid;
    pid_t return_pid = waitpid(pid, &status, WNOHANG);
    if(return_pid == -1){
        printf("Error waiting for process %i\n", pid);
    }else if(return_pid == 0){
        newProcess->status = RUNNING;
    }else if(return_pid == pid){
        newProcess->status = TERMINATED;
    }else{
        newProcess->status = SUSPENDED;
    }
    newProcess->next = *process_list;
    *process_list = newProcess;
}

void printProcessList(process** process_list){
    process* current = *process_list;
    char snum[32];
    printf(PRINT_FORMAT, "PID", "COMMAND", "STATUS");
    while(current != NULL){
        sprintf(snum, "%d", current->pid);
        printf(PRINT_FORMAT, snum, current->cmd->arguments[0], getStatusString(current->status));
        current = current->next;
    }
}

int main(){
    return 0;
}