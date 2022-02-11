#include <iostream>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <signal.h>
#include <termios.h>
#include <vector>

using namespace std;

struct command {
    char *args[128];
    char *outredirect, *inredirect;
};

vector<command> parseInput(char userInput[]) {
    vector<command> vec;
    const char pipedelim[] = "|";
    const char spacedelim[] = " ";
    char *token1, *token2;
    while((token1 = strsep(&userInput, pipedelim)) != NULL) {
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

signed main() {
    char userInput[1024];
    string s;
    while(1) {
        getline(cin, s);
        vector<command> v = parseInput(strcpy(userInput, s.c_str()));
        for(auto i:v) {
            cout << "Command: " << i.args[0] << endl;
            cout << "Args: ";
            int it = 0;
            while(i.args[it] != NULL) cout << i.args[it++] << " ";
            cout << endl;
            cout << "Output: " << (i.outredirect != NULL ? i.outredirect : "STDOUT") << endl;
            cout << "Input: " << (i.inredirect != NULL ? i.inredirect : "STDIN") << endl;
            cout << endl;
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
