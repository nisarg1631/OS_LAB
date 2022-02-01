#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <signal.h>

int cpid;

void signal_callback_handler_parent(int signum) {
    signal(SIGINT, signal_callback_handler_parent);
    // cout << "\b  \b" << '\n';
    printf("\b\b  \b\b\n");
    if(cpid && kill(-cpid, SIGINT) == -1) {
        perror("Couldn't kill: ");
    }
}

void signal_callback_handler_parent2(int signum) {
    signal(SIGTSTP, signal_callback_handler_parent2);
    // cout << "\b  \b" << '\n';
    printf("\b\b  \b\b\n");
    if(cpid && kill(-cpid, SIGTSTP) == -1) {
        perror("Couldn't stop: ");
    }
}

signed main() {
    signal(SIGINT, signal_callback_handler_parent);
    signal(SIGTSTP, signal_callback_handler_parent2);
    cpid = 0;
    for(int i=0; i<5; i++) {
        int childpid = fork();
        if(childpid == 0) {
            char command[] = "./test";
            char *args[6];
            args[0] = command;
            args[1] = NULL;
            setpgid(0, cpid);
            execvp(command, args);
        }
        if(cpid == 0)
            cpid = childpid;
    }
    // int childpid = fork();
    // if(childpid == 0) {
    //     char command[] = "./test";
    //     char *args[2];
    //     args[0] = command;
    //     args[1] = NULL;
    //     setpgrp();
    //     execvp(command, args);
    // }
    int status;
    while(waitpid(-cpid, &status, WUNTRACED) > 0) {
        if(WIFSTOPPED(status)) {
            printf("Child was stopped!\n");
            kill(-cpid, SIGCONT);
            break;
        } else {
            printf("Child is done!\n");
        }
    }
    cpid = 0;
    printf("Shell is back!\n");
    while(1) ;
    return 0;
}
