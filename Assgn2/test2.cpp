#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <signal.h>
#include <sys/types.h>
#include <stack>
#include <map>
#include <set>

using namespace std;

int cpid;
stack<pair<int, int>> s;
set<int> activeprocs;

void signal_callback_handler_parent(int signum) {
    signal(SIGINT, signal_callback_handler_parent);
    // cout << "\b  \b" << '\n';
    printf("\b\b  \b\b\nHey\n");
    // if(cpid && kill(-cpid, SIGINT) == -1) {
    //     perror("Couldn't kill: ");
    // }
}

void signal_callback_handler_parent2(int signum) {
    signal(SIGTSTP, signal_callback_handler_parent2);
    // cout << "\b  \b" << '\n';
    printf("\b\b  \b\b\nHey2\n");
    // if(cpid && kill(-cpid, SIGTSTP) == -1) {
    //     perror("Couldn't stop: ");
    // }
}

void signal_callback_handler_parent3(int signum) {
    signal(SIGCHLD, signal_callback_handler_parent3);
    while(1) {
        int status;
        int childpid = waitpid(-1, &status, WNOHANG | WUNTRACED);
        if(childpid <= 0) break;
        if(activeprocs.count(childpid)) {
            activeprocs.erase(childpid);
            if(WIFSTOPPED(status)) {
                s.push({childpid, 1});
            } else {
                s.push({childpid, 0});
            }
        }
    }
}

void toggleSIGCHLDBlock(int how) {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(how, &mask, NULL);
}

signed main() {
    signal(SIGINT, signal_callback_handler_parent);
    signal(SIGTSTP, signal_callback_handler_parent2);
    signal(SIGCHLD, signal_callback_handler_parent3);
    signal(SIGTTOU, SIG_IGN);
    cpid = 0;
    for(int i=0; i<1; i++) {
        toggleSIGCHLDBlock(SIG_BLOCK);
        int childpid = fork();
        if(childpid == 0) {
            toggleSIGCHLDBlock(SIG_UNBLOCK);
            char *args[6];
            args[0] = "./test";
            // args[1] = "ls";
            args[1] = NULL;
            setpgid(0, cpid);
            execvp(args[0], args);
        }
        if(cpid == 0) {
            cpid = childpid;
            tcsetpgrp(STDIN_FILENO, cpid);
        }
        sleep(5);
        activeprocs.insert(childpid);
        toggleSIGCHLDBlock(SIG_UNBLOCK);
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
    // int status, childpid;
    while(!activeprocs.empty()) ;
    tcsetpgrp(STDIN_FILENO, getpgrp());
    while(!s.empty()) {
        if(s.top().second) kill(s.top().first, SIGCONT);
        s.pop();
    }
    cpid = 0;
    printf("Shell is back!\n");
    while(1) ;
    return 0;
}
