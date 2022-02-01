#include <iostream>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <signal.h>
#include <termios.h>
#include <vector>

using namespace std;

// GLOBALS

const char *inbuilt_commands[] = { "cd", "multiWatch", "exit" };
int current_process_group;

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
    if(current_process_group && kill(-current_process_group, SIGINT) == -1) {
        perror("Couldn't kill: ");
    }
}

void sigtstp_callback_handler(int signum) {
    signal(SIGTSTP, sigtstp_callback_handler);
    printf("\b  \b\n");
    if(current_process_group && kill(-current_process_group, SIGTSTP) == -1) {
        perror("Couldn't stop: ");
    }
}

// PROCESS EXECUTOR

void executeProcesses(const vector<command> &procs, int background) {
    if(procs.empty()) return;

}

signed main() {
    char pwd[1024], user_input[1024];

    // set signal handlers
    signal(SIGINT, sigint_callback_handler);
    signal(SIGTSTP, sigtstp_callback_handler);

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
            executeProcesses(procs, background);
        }
    }
    return 0;
}
