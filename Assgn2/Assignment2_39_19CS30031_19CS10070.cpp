#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <wait.h>
#include <signal.h>
#include <termios.h>
#include <vector>
#include <set>
#include <stack>

using namespace std;

// GLOBALS

int current_process_group;
stack<pair<int, int>> processedprocs;
set<int> activeprocs;

// COMMAND PARSER

struct command {
    char *args[128];
    char *outredirect, *inredirect;
};

vector<command> parseInput(char user_input[]) {
    vector<command> vec;
    const char pipedelim[] = "|";
    const char spacedelim[] = " ";
    char *token1, *token2;
    while((token1 = strsep(&user_input, pipedelim)) != NULL) {
        command comm;
        comm.outredirect = comm.inredirect = NULL;
        int cnt = 0, outredflag = 0, inredflag = 0;
        while((token2 = strsep(&token1, spacedelim)) != NULL) {
            if(!strcmp(token2, ""))
                continue;
            if(outredflag) {
                comm.outredirect = strdup(token2);
                outredflag = 0;
                continue;
            }
            if(inredflag) {
                comm.inredirect = strdup(token2);
                inredflag = 0;
                continue;
            }
            if(!strcmp(token2, "<")) {
                inredflag = 1;
                continue;
            }
            if(!strcmp(token2, ">")) {
                outredflag = 1;
                continue;
            }
            comm.args[cnt++] = strdup(token2);
        }
        comm.args[cnt] = NULL;
        if(cnt) vec.push_back(comm);
    }
    return vec;
}

// SIGNAL HANDLERS

void sigint_callback_handler(int signum) {
    signal(SIGINT, sigint_callback_handler);
    printf("\b  \b\n");
    // if(current_process_group && kill(-current_process_group, SIGINT) == -1) {
    //     perror("Couldn't kill: ");
    // }
}

void sigtstp_callback_handler(int signum) {
    signal(SIGTSTP, sigtstp_callback_handler);
    printf("\b  \b\n");
    // if(current_process_group && kill(-current_process_group, SIGTSTP) == -1) {
    //     perror("Couldn't stop: ");
    // }
}

void sigchld_callback_handler(int signum) {
    signal(SIGCHLD, sigchld_callback_handler);
    while(1) {
        int status;
        int childpid = waitpid(-1, &status, WNOHANG | WUNTRACED);
        if(childpid <= 0) break;
        if(activeprocs.count(childpid)) {
            activeprocs.erase(childpid);
            if(WIFSTOPPED(status)) {
                processedprocs.push({childpid, 1});
            } else {
                processedprocs.push({childpid, 0});
            }
        }
    }
}

// SIGNAL BLOCKER (To avoid race conditions)
void sigchldBlocker(int state) {
    sigset_t sigs;
    sigemptyset(&sigs);
    sigaddset(&sigs, SIGCHLD);
    sigprocmask(state, &sigs, NULL);
}

// PROCESS EXECUTOR

void executeProcesses(const vector<command> &procs, int background) {
    current_process_group = 0;
    activeprocs.clear();
    int pipefd[2];
    for(int i=0; i<procs.size(); i++) {
        int infd = STDIN_FILENO, outfd = STDOUT_FILENO;
        if(procs[i].inredirect != NULL) {
            infd = open(procs[i].inredirect, O_RDONLY);
        }
        if(procs[i].outredirect != NULL) {
            outfd = open(procs[i].outredirect, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666);
        }
        if(i > 0) {
            infd = pipefd[0];
        }
        if(i + 1 < procs.size()) {
            if(pipe(pipefd) == -1) {
                perror("Pipe error: ");
            }
            outfd = pipefd[1];
        }
        sigchldBlocker(SIG_BLOCK);
        int childpid = fork();
        if(childpid == 0) {
            sigchldBlocker(SIG_UNBLOCK);
            if(infd != STDIN_FILENO) {
                dup2(infd, STDIN_FILENO);
                close(infd);
            }
            if(outfd != STDOUT_FILENO) {
                dup2(outfd, STDOUT_FILENO);
                close(outfd);
            }
            setpgid(0, current_process_group);
            execvp(procs[i].args[0], procs[i].args);
            perror("Exec error: ");
            exit(EXIT_FAILURE);
        }
        if(current_process_group == 0) {
            current_process_group = childpid;
            if(!background) tcsetpgrp(STDIN_FILENO, current_process_group);
        }
        if(!background) activeprocs.insert(childpid);
        sigchldBlocker(SIG_UNBLOCK);
        if(i + 1 < procs.size())
            close(outfd);
    }
    if(!background) {
        while(!activeprocs.empty()) ;
        while(!processedprocs.empty()) {
            if(processedprocs.top().second) kill(processedprocs.top().first, SIGCONT);
            processedprocs.pop();
        }
        tcsetpgrp(STDIN_FILENO, getpgrp());
    }
    current_process_group = 0;
    activeprocs.clear();
}

// MULTI WATCH

signed main() {
    char pwd[1024], user_input[1024];
    current_process_group = 0;
    activeprocs.clear();
    while(!processedprocs.empty()) processedprocs.pop();

    // set signal handlers
    signal(SIGINT, sigint_callback_handler);
    signal(SIGTSTP, sigtstp_callback_handler);
    signal(SIGCHLD, sigchld_callback_handler);
    signal(SIGTTOU, SIG_IGN);

    // set io settings
    struct termios tio;
    tcgetattr(STDIN_FILENO, &tio);
    tio.c_lflag &= (~ICANON & ~ECHO);
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &tio);

    // start shell
    while(1) {
        getcwd(pwd, 1024);
        printf("%s > ", pwd);
        fflush(stdout);
        int cnt = 0;
        char inp;
        do {
            inp = getchar();
            if(inp == '\t') {
                // autocomplete
            } else if (inp == '\n' || inp == 18) {
                user_input[cnt] = '\0';
                // cout << endl;
                printf("\n");
            } else if (inp == 127) {
                if(cnt) {
                    printf("\b \b");
                    cnt--;
                }
            } else {
                user_input[cnt++] = inp;
                printf("%c", inp);
            }
        } while (inp != '\n' && inp != 18);

        if(inp == 18) {
            printf("Enter search term: ");
        }

        if(inp == '\n') {
            while(cnt && user_input[cnt-1] == ' ') {
                user_input[cnt-1] = '\0';
                cnt--;
            }
            int background = 0;
            if(cnt && user_input[cnt-1] == '&') {
                user_input[cnt-1] = '\0';
                cnt--;
                background = 1;
            }
            vector<command> procs = parseInput(user_input);
            if(!procs.empty()) {
                char *firstcommand = procs.front().args[0];
                if(!strcmp(firstcommand, "cd")) {
                    // change directory
                    if(chdir(procs.front().args[1]) == -1) {
                        perror("Failed to change directory: ");
                    }
                } else if(!strcmp(firstcommand, "multiWatch")) {
                    // multi watch
                } else if(!strcmp(firstcommand, "exit")) {
                    // exit shell
                    printf("\n\n---- Terminating shell. Bye :) ----\n\n");
                    exit(0);
                } else {
                    // normal command
                    executeProcesses(procs, background);
                }
            }
            // cout << endl;
            // for(auto i:v) {
            //     cout << "Command: " << i.args[0] << endl;
            //     cout << "Args: ";
            //     int it = 0;
            //     while(i.args[it] != NULL) cout << i.args[it++] << " ";
            //     cout << endl;
            //     cout << "Output: " << (i.outredirect != NULL ? i.outredirect : "STDOUT") << endl;
            //     cout << "Input: " << (i.inredirect != NULL ? i.inredirect : "STDIN") << endl;
            //     cout << endl;
            // }
            // cout << "Background: " << (background ? "YES" : "NO") << endl;
            // cout << endl;
        }
    }
    return 0;
}
