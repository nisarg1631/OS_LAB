#include <iostream>
#include <fstream>
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
#include <sys/inotify.h>
#include <sys/select.h>
#include <map>
#include <string>
#include <time.h>

using namespace std;

// INOTIFY DEFINES

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (16384 * EVENT_SIZE)

// GLOBALS

int current_process_group;
stack<int> stoppedprocs;
set<int> foregroundprocs, backgroundprocs;
int multi_watch_on, inotify_fd, wd;

// IO SETTINGS

struct termios tio, oldtio;

// COMMAND PARSER

struct command
{
    char *args[128];
    char *outredirect, *inredirect;
};

vector<command> parseInput(const char user_input[])
{
    char *user_input_copy = strdup(user_input);
    vector<command> vec;
    const char pipedelim[] = "|";
    const char spacedelim[] = " ";
    char *token1, *token2;
    while ((token1 = strsep(&user_input_copy, pipedelim)) != NULL)
    {
        command comm;
        comm.outredirect = comm.inredirect = NULL;
        int cnt = 0, outredflag = 0, inredflag = 0;
        while ((token2 = strsep(&token1, spacedelim)) != NULL)
        {
            if (!strcmp(token2, ""))
                continue;
            if (outredflag)
            {
                comm.outredirect = strdup(token2);
                outredflag = 0;
                continue;
            }
            if (inredflag)
            {
                comm.inredirect = strdup(token2);
                inredflag = 0;
                continue;
            }
            if (!strcmp(token2, "<"))
            {
                inredflag = 1;
                continue;
            }
            if (!strcmp(token2, ">"))
            {
                outredflag = 1;
                continue;
            }
            comm.args[cnt++] = strdup(token2);
        }
        comm.args[cnt] = NULL;
        if (cnt)
            vec.push_back(comm);
    }
    return vec;
}

// SIGNAL HANDLERS

void sigint_callback_handler(int signum)
{
    signal(SIGINT, sigint_callback_handler);
    printf("\b  \b\n");
    // write to selfpipe to unblock from select call
    if (multi_watch_on)
    {
        inotify_rm_watch(inotify_fd, wd);
        if (close(inotify_fd) == -1)
            perror("Failed to close inotify: ");
    }
}

void sigtstp_callback_handler(int signum)
{
    signal(SIGTSTP, sigtstp_callback_handler);
    printf("\b  \b\n");
}

void sigchld_callback_handler(int signum)
{
    signal(SIGCHLD, sigchld_callback_handler);
    int flag = 0;
    while (1)
    {
        int status;
        int childpid = waitpid(-1, &status, WNOHANG | WUNTRACED);
        if (childpid <= 0)
            break;
        if (foregroundprocs.count(childpid))
        {
            foregroundprocs.erase(childpid);
            if (WIFSTOPPED(status))
            {
                stoppedprocs.push(childpid);
            }
        }
        if (backgroundprocs.count(childpid))
        {
            backgroundprocs.erase(childpid);
            printf("\nBackground process with PID %d done.\n", childpid);
            flag = 1;
        }
    }
    if (flag)
    {
        char pwd[1024];
        getcwd(pwd, 1024);
        printf("\n%s > ", pwd);
        fflush(stdout);
    }
}

// SIGNAL BLOCKER (To avoid race conditions)

void sigchldBlocker(int state)
{
    sigset_t sigs;
    sigemptyset(&sigs);
    sigaddset(&sigs, SIGCHLD);
    sigprocmask(state, &sigs, NULL);
}

// PROCESS EXECUTOR

void executeProcesses(const vector<command> &procs, int background)
{
    current_process_group = 0;
    foregroundprocs.clear();
    while (!stoppedprocs.empty())
        stoppedprocs.pop();
    int pipefd[2];
    for (int i = 0; i < procs.size(); i++)
    {
        int infd = STDIN_FILENO, outfd = STDOUT_FILENO;
        if (procs[i].inredirect != NULL)
        {
            infd = open(procs[i].inredirect, O_RDONLY);
        }
        if (procs[i].outredirect != NULL)
        {
            outfd = open(procs[i].outredirect, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666);
        }
        if (i > 0)
        {
            infd = pipefd[0];
        }
        if (i + 1 < procs.size())
        {
            if (pipe(pipefd) == -1)
            {
                perror("Pipe error: ");
            }
            outfd = pipefd[1];
        }
        sigchldBlocker(SIG_BLOCK);
        int childpid = fork();
        if (childpid == 0)
        {
            sigchldBlocker(SIG_UNBLOCK);
            if (infd != STDIN_FILENO)
            {
                dup2(infd, STDIN_FILENO);
                close(infd);
            }
            if (outfd != STDOUT_FILENO)
            {
                dup2(outfd, STDOUT_FILENO);
                close(outfd);
            }
            setpgid(0, current_process_group);
            execvp(procs[i].args[0], procs[i].args);
            perror("Exec error: ");
            exit(EXIT_FAILURE);
        }
        if (current_process_group == 0)
        {
            current_process_group = childpid;
            if (!background)
            {
                tcsetpgrp(STDIN_FILENO, current_process_group);
                tcsetattr(STDIN_FILENO, TCSANOW, &oldtio);
            }
        }
        if (!background)
            foregroundprocs.insert(childpid);
        else
            backgroundprocs.insert(childpid);
        sigchldBlocker(SIG_UNBLOCK);
        if (i + 1 < procs.size())
            close(outfd);
    }
    if (!background)
    {
        while (!foregroundprocs.empty())
            ;
        while (!stoppedprocs.empty())
        {
            backgroundprocs.insert(stoppedprocs.top());
            kill(stoppedprocs.top(), SIGCONT);
            stoppedprocs.pop();
        }
        tcsetpgrp(STDIN_FILENO, getpgrp());
        tcsetattr(STDIN_FILENO, TCSANOW, &tio);
    }
    current_process_group = 0;
    foregroundprocs.clear();
    while (!stoppedprocs.empty())
        stoppedprocs.pop();
}

// MULTI WATCH

void multiWatchPrintUtil(int fd, string cmd)
{
    int flag = 1;
    char ctemp;
    while (read(fd, &ctemp, 1))
    {
        if (flag)
        {
            flag = 0;
            time_t rawtime;
            time(&rawtime);
            struct tm *timeinfo = localtime(&rawtime);
            printf("\n\" %s\", %d:%d:%d\n", cmd.c_str(), timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
            printf("<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-\n");
        }
        printf("%c", ctemp);
    }
    if (!flag)
    {
        printf("\n->->->->->->->->->->->->->->->->->->->\n");
    }
}

void executeMultiWatch(const char user_input[])
{
    char user_input_modified[1024];
    int it1 = 0, it2 = 0;
    while (user_input[it2++] != '[')
        ;
    while (user_input[it2] != '\0')
    {
        if (user_input[it2] == ',')
            user_input_modified[it1++] = '|';
        else if (user_input[it2] == '\"' || user_input[it2] == ']')
            ; // do nothing
        else
            user_input_modified[it1++] = user_input[it2];
        it2++;
    }
    user_input_modified[it1] = '\0';
    vector<command> procs = parseInput(user_input_modified);

    if ((inotify_fd = inotify_init()) < 0)
    {
        perror("inotify init: ");
        exit(0);
    }
    if ((wd = inotify_add_watch(inotify_fd, ".", IN_MODIFY | IN_CREATE | IN_ACCESS)) < 0)
    {
        perror("add watch: ");
        exit(0);
    }

    map<string, pair<int, string>> procfiles;
    current_process_group = 0;

    for (auto proc : procs)
    {
        int childpid = fork();

        if (childpid == 0)
        {
            string filename = ".tmp." + to_string(getpid()) + ".txt";
            int writefd = open(filename.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_TRUNC, 0666);

            dup2(writefd, STDOUT_FILENO);
            close(writefd);

            setpgid(0, current_process_group);

            execvp(proc.args[0], proc.args);
            perror("Exec error: ");
            exit(EXIT_FAILURE);
        }
        if (current_process_group == 0)
            current_process_group = childpid;

        string filename = ".tmp." + to_string(childpid) + ".txt";
        int readfd = open(filename.c_str(), O_RDONLY | O_CREAT, 0666);

        string cmd;
        int it = 0;
        while (proc.args[it++] != NULL)
        {
            cmd += proc.args[it - 1];
            cmd += " ";
        }

        procfiles[filename] = make_pair(readfd, cmd);
    }

    multi_watch_on = 1;

    for (auto &i : procfiles)
        multiWatchPrintUtil(i.second.first, i.second.second);

    while (1)
    {
        char buf[BUF_LEN];
        int readlen = read(inotify_fd, buf, BUF_LEN), it = 0;
        if (readlen == -1)
        {
            kill(-current_process_group, SIGKILL);
            multi_watch_on = 0;
            for (auto &i : procfiles)
                remove(i.first.c_str());
            break;
        }
        while (it < readlen)
        {
            struct inotify_event *event = (struct inotify_event *)&buf[it];
            if (procfiles.count(event->name))
            {
                auto fdetails = procfiles[event->name];
                multiWatchPrintUtil(fdetails.first, fdetails.second);
            }
            it += EVENT_SIZE + event->len;
        }
    }
}
// code adapted from https://cp-algorithms.com/string/suffix-automaton.html
// for a string of length n and an alphabet of size k, time for build is O(nlogk) and memory is O(n)
struct state
{
    int len, link;
    map<char, int> next;
};
class SuffixAutomaton
{
    int text_size;
    vector<state> st;
    int sz, last;
    const string TEXT;
    void sa_init()
    {
        st[0].len = 0;
        st[0].link = -1;
        sz = 1;
        last = 0;
    }
    void sa_extend(char c)
    {
        int cur = sz++;
        st[cur].len = st[last].len + 1;
        int p = last;
        while (p != -1 && !st[p].next.count(c))
        {
            st[p].next[c] = cur;
            p = st[p].link;
        }
        if (p == -1)
        {
            st[cur].link = 0;
        }
        else
        {
            int q = st[p].next[c];
            if (st[p].len + 1 == st[q].len)
            {
                st[cur].link = q;
            }
            else
            {
                int clone = sz++;
                st[clone].len = st[p].len + 1;
                st[clone].next = st[q].next;
                st[clone].link = st[q].link;
                while (p != -1 && st[p].next[c] == q)
                {
                    st[p].next[c] = clone;
                    p = st[p].link;
                }
                st[q].link = st[cur].link = clone;
            }
        }
        last = cur;
    }

public:
    SuffixAutomaton(const string &text) : TEXT(text), text_size(text.size())
    {
        st.resize(2 * text_size + 10);
        sa_init();
        for (auto c : text)
            sa_extend(c);
    }

    int longest_prefix(string pattern)
    {
        int cur = 0;
        int len = 0;
        for (auto c : pattern)
        {
            if (st[cur].next.count(c))
            {
                cur = st[cur].next[c];
                len++;
            }
            else
            {
                return len;
            }
        }
        return len;
    }
    int longest_substring(string pattern)
    {
        int cur = 0;
        int len = 0;
        int temp = 0;
        string ans;

        for (int i = 0; i < pattern.size(); i++)
        {
            char c = pattern[i];
            if (st[cur].next.count(c))
            {
                temp++;
                cur = st[cur].next[c];
            }
            else
            {
                while (~cur and !st[cur].next.count(c))
                {
                    cur = st[cur].link;
                }
                if (~cur)
                {
                    temp = st[cur].len + 1;
                    cur = st[cur].next[c];
                }
                else
                {
                    cur = 0;
                    temp = 0;
                }
            }
            // i - temp + 1 is start of match
            if (len < temp)
            {
                ans = pattern.substr(i - temp + 1, temp);
                len = temp;
            }
        }
        // cout<<ans<<endl;
        return static_cast<int>(ans.size());
    }
    string returnFullString()
    {
        return TEXT;
    }
};
// HISTORY (used the singleton design pattern)

class history
{
private:
    history()
    {
        // read from file
        ifstream readstream(history_file_name, ios::in);
        if (!readstream.fail())
        {
            string s;
            while (getline(readstream, s))
                addHistory(s);
        }
        readstream.close();
    }

    static const int max_history;
    static const string history_file_name;
    // Command strings are stored in a deque, with the most recent ones being in the front
    deque<string> buff;
    deque<SuffixAutomaton> suffix_automata_history;

public:
    static history &shellHistoy()
    {
        static history sShellHistory;
        return sShellHistory;
    }

    void addHistory(string command)
    {
        buff.push_front(command);
        suffix_automata_history.push_front(SuffixAutomaton(command));
        if (buff.size() > max_history)
        {
            buff.pop_back();
            suffix_automata_history.pop_back();
        }
    }

    void saveHistory()
    {
        // write to file
        ofstream writestream(history_file_name, ios::out | ios::trunc);
        for (auto command : buff)
            writestream << command << '\n';
        writestream.close();
    }

    const deque<string> &getHistory()
    {
        return buff;
    }
    string return_match(const string &s)
    {
        string ans;
        int len = 0;
        for (auto i : suffix_automata_history)
        {
            if (i.longest_substring(s) > len)
            {
                ans = i.returnFullString();
                len = i.longest_substring(s);
            }
        }
        return ans;
    }
};

const int history::max_history = 10000;
const string history::history_file_name = ".termhistory";

signed main()
{
    char pwd[1024], user_input[1024];

    current_process_group = 0;
    multi_watch_on = 0;
    foregroundprocs.clear();
    backgroundprocs.clear();
    while (!stoppedprocs.empty())
        stoppedprocs.pop();

    // set signal handlers
    signal(SIGINT, sigint_callback_handler);
    signal(SIGTSTP, sigtstp_callback_handler);
    signal(SIGCHLD, sigchld_callback_handler);
    signal(SIGTTOU, SIG_IGN);

    // set io settings
    tcgetattr(STDIN_FILENO, &tio);
    oldtio = tio;
    tio.c_lflag &= (~ICANON & ~ECHO);
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &tio);

    // start shell
    while (1)
    {
        getcwd(pwd, 1024);
        printf("%s > ", pwd);
        fflush(stdout);
        int cnt = 0;
        char inp;
        do
        {
            inp = getchar();
            if (inp == '\t')
            {
                // autocomplete
                // printf("AUTOCOMPLETE %s",user_input);
            }
            else if (inp == '\n' || inp == 18)
            {
                user_input[cnt] = '\0';
                // cout << endl;
                printf("\n");
            }
            else if (inp == 127)
            {
                if (cnt)
                {
                    printf("\b \b");
                    cnt--;
                }
            }
            else
            {
                user_input[cnt++] = inp;
                printf("%c", inp);
            }
        } while (inp != '\n' && inp != 18);

        if (inp == 18)
        {
            printf("Enter search term: ");
            char hist_input[1024];
            do
            {
                inp = getchar();
                if (inp == '\n')
                {
                    hist_input[cnt] = '\0';
                    // cout << endl;
                    printf("\n");
                }
                else if (inp == 127)
                {
                    if (cnt)
                    {
                        printf("\b \b");
                        cnt--;
                    }
                }
                else
                {
                    hist_input[cnt++] = inp;
                    printf("%c", inp);
                }
            } while (inp != '\n' && inp != 18);
            if (inp != 18)
            {
                string search_term(hist_input);
                string ans = history::shellHistoy().return_match(search_term);
                if (ans.size() > 2)
                    printf("%s\n", ans.c_str());
                else
                    printf("No match found\n");
            }
        }

        if (inp == '\n')
        {
            while (cnt && user_input[cnt - 1] == ' ')
            {
                user_input[cnt - 1] = '\0';
                cnt--;
            }
            if (cnt)
                history::shellHistoy().addHistory(user_input);
            int background = 0;
            if (cnt && user_input[cnt - 1] == '&')
            {
                user_input[cnt - 1] = '\0';
                cnt--;
                background = 1;
            }
            vector<command> procs = parseInput(user_input);
            if (!procs.empty())
            {
                char *firstcommand = procs.front().args[0];
                if (!strcmp(firstcommand, "cd"))
                {
                    // change directory
                    if (chdir(procs.front().args[1]) == -1)
                    {
                        perror("Failed to change directory: ");
                    }
                }
                else if (!strcmp(firstcommand, "multiWatch"))
                {
                    // multi watch
                    executeMultiWatch(user_input);
                }
                else if (!strcmp(firstcommand, "exit"))
                {
                    // exit shell
                    tcsetattr(STDIN_FILENO, TCSANOW, &oldtio);
                    history::shellHistoy().saveHistory();
                    printf("\n\n---- Terminating shell. Bye :) ----\n\n");
                    exit(0);
                }
                else
                {
                    // normal command
                    executeProcesses(procs, background);
                }
            }
        }
    }
    return 0;
}
