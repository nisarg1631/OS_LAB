#include <iostream>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <signal.h>
#include <termios.h>

using namespace std;

const char *inbuilt_commands[] = { "cd", "multiWatch", "exit" };

struct command {
    char *comm;
    char *args[128];
    char *outredirect, *inredirect;
};

void signal_callback_handler_parent(int signum) {
    signal(SIGINT, signal_callback_handler_parent);
    // cout << "\b  \b" << '\n';
    printf("\b \b\n");
}

void executeSingleProcess(char *command, char *args[]) {
    int childpid = fork();
    if(childpid > 0) {
        waitpid(childpid, NULL, 0);
    }
    if(childpid == 0) {
        execvp(command, args);
    }
}

void executeSingleProcessBackground(char *command, char *args[]) {
    int childpid = fork();
    if(childpid > 0) {
        // waitpid(childpid, NULL, 0);
        return;
    }
    if(childpid == 0) {
        setpgrp();
        execvp(command, args);
    }
}

signed main() {
    char pwd[1024], command[1024];
    signal(SIGINT, signal_callback_handler_parent);

    struct termios old_tio, new_tio;

    /* get the terminal settings for stdin */
    tcgetattr(STDIN_FILENO, &old_tio);

    /* we want to keep the old setting to restore them a the end */
    new_tio = old_tio;

    /* disable canonical mode (buffered i/o) and local echo */
    new_tio.c_lflag &= (~ICANON & ~ECHO);
    new_tio.c_cc[VMIN] = 1;
    new_tio.c_cc[VTIME] = 0;
    /* set the new settings immediately */
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

    while(1) {
        getcwd(pwd, 1024);
        // cout << pwd << " > ";
        printf("%s > ", pwd);
        fflush(stdout);
        int cnt = 0;
        char inp;
        do {
            inp = getchar();
            if(inp == '\t') {
                // autocomplete
            } else if (inp == '\n' || inp == 18) {
                command[cnt] = '\0';
                // cout << endl;
                printf("\n");
            } else if (inp == 127) {
                if(cnt) {
                    printf("\b \b");
                    cnt--;
                }
            } else {
                command[cnt++] = inp;
                printf("%c", inp);
            }
        } while (inp != '\n' && inp != 18);

        if(inp == 18) {
            // cout << "Enter search term: ";
            printf("Enter search term: ");
            // string s; cin >> s;
        }

        if(inp == '\n') {
            char *args[2];
            args[0] = command;
            args[1] = NULL;
            if(command[0] != '\0')
                executeSingleProcessBackground(command, args);
        }
    }

    /* restore the former settings */
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);

    return 0;
}
