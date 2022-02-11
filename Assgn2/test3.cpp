#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/inotify.h>
#include <sys/select.h>
#include <signal.h>
#include <map>
#include <string>
#include <time.h>

using namespace std;

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * EVENT_SIZE)

int selfpipe[2];

void signal_handler (int signum) {
    signal(SIGINT, signal_handler);
    printf("Heyyy\n");
    fflush(stdout);
    if(write(selfpipe[1], "", 1) == -1) {
        perror("failed to write: ");
    }
}

signed main() {
    signal(SIGINT, signal_handler);
    pipe(selfpipe);

    int inotify_fd, wd;
    if((inotify_fd = inotify_init()) < 0) {
        perror("inotify init: ");
        exit(0);
    }
    if((wd = inotify_add_watch(inotify_fd, ".", IN_MODIFY)) < 0) {
        perror("add watch: ");
        exit(0);
    }

    map<string, pair<int, string>> m;
    for(int i=0; i<2; i++) {
        string filename = ".tmp." + to_string(i) + ".txt";
        int tempfd = open(filename.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_TRUNC, 0666);
        m[filename] = make_pair(0, "Sample Command" + to_string(i));
    }
    for(auto &i:m) {
        i.second.first = open(i.first.c_str(), O_RDONLY | O_CREAT, 0666);
        int flag = 1;
        char ctemp;
        while(read(i.second.first, &ctemp, 1)) {
            if(flag) {
                flag = 0;
                time_t rawtime;
                time(&rawtime);
                struct tm *timeinfo = localtime(&rawtime);
                printf("\n\"%s\", %d:%d:%d\n", i.second.second.c_str(), timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
                printf("<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-\n");
            }
            printf("%c", ctemp);
        }
        if(!flag) {
            printf("\n->->->->->->->->->->->->->->->->->->->\n");
        }
    }
    while(1) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(inotify_fd, &rfds);
        FD_SET(selfpipe[0], &rfds);
        select(max(inotify_fd, selfpipe[0])+1, &rfds, NULL, NULL, NULL);
        if(FD_ISSET(selfpipe[0], &rfds)) {
            break;
        }
        if(FD_ISSET(inotify_fd, &rfds)) {
            char buf[BUF_LEN];
            int readlen = read(inotify_fd, buf, BUF_LEN), it = 0;
            while (it < readlen) {
                struct inotify_event *event = (struct inotify_event *) &buf[it];
                if(m.count(event->name)) {
                    auto fdetails = m[event->name];
                    int flag = 1;
                    char ctemp;
                    while(read(fdetails.first, &ctemp, 1)) {
                        if(flag) {
                            flag = 0;
                            time_t rawtime;
                            time(&rawtime);
                            struct tm *timeinfo = localtime(&rawtime);
                            printf("\n\"%s\", %d:%d:%d\n", fdetails.second.c_str(), timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
                            printf("<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-\n");
                        }
                        printf("%c", ctemp);
                    }
                    if(!flag) {
                        printf("\n->->->->->->->->->->->->->->->->->->->\n");
                    }
                }
                it += EVENT_SIZE + event->len;
            }
        }
    }

    inotify_rm_watch(inotify_fd, wd);
    close(inotify_fd);

    return 0;
}

/* Old Implementation

void sigint_callback_handler(int signum) {
    signal(SIGINT, sigint_callback_handler);
    printf("\b  \b\n");
    // write to selfpipe to unblock from select call
    if(multi_watch_on && write(selfpipe[1], "n", 1) == -1)
        perror("Failed to write: ");
}

// MULTI WATCH

void multiWatchPrintUtil(int fd, string cmd) {
    int flag = 1;
    char ctemp;
    while(read(fd, &ctemp, 1)) {
        if(flag) {
            flag = 0;
            time_t rawtime;
            time(&rawtime);
            struct tm *timeinfo = localtime(&rawtime);
            printf("\n\" %s\", %d:%d:%d\n", cmd.c_str(), timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
            printf("<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-\n");
        }
        printf("%c", ctemp);
    }
    if(!flag) {
        printf("\n->->->->->->->->->->->->->->->->->->->\n");
    }
}

void executeMultiWatch(const char user_input[]) {
    char user_input_modified[1024];
    int it1 = 0, it2 = 0;
    while(user_input[it2++] != '[') ;
    while(user_input[it2] != '\0') {
        if(user_input[it2] == ',') user_input_modified[it1++] = '|';
        else if(user_input[it2] == '\"' || user_input[it2] == ']') ; // do nothing
        else user_input_modified[it1++] = user_input[it2];
        it2++;
    }
    user_input_modified[it1] = '\0';
    vector<command> procs = parseInput(user_input_modified);

    if((inotify_fd = inotify_init()) < 0) {
        perror("inotify init: ");
        exit(0);
    }
    if((wd = inotify_add_watch(inotify_fd, ".", IN_MODIFY | IN_CREATE | IN_ACCESS)) < 0) {
        perror("add watch: ");
        exit(0);
    }

    map<string, pair<int, string>> procfiles;
    current_process_group = 0;

    for(auto proc:procs) {
        int childpid = fork();

        if(childpid == 0) {
            string filename = ".tmp." + to_string(getpid()) + ".txt";
            int writefd = open(filename.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_TRUNC, 0666);

            dup2(writefd, STDOUT_FILENO);
            close(writefd);

            setpgid(0, current_process_group);

            execvp(proc.args[0], proc.args);
            perror("Exec error: ");
            exit(EXIT_FAILURE);
        }
        if(current_process_group == 0) 
            current_process_group = childpid;
        
        string filename = ".tmp." + to_string(childpid) + ".txt";
        int readfd = open(filename.c_str(), O_RDONLY | O_CREAT, 0666);

        string cmd; 
        int it = 0;
        while(proc.args[it++] != NULL) {
            cmd += proc.args[it-1];
            cmd += " ";
        }

        procfiles[filename] = make_pair(readfd, cmd);
    }

    multi_watch_on = 1;
    // pipe(selfpipe);

    for(auto &i:procfiles) 
        multiWatchPrintUtil(i.second.first, i.second.second);
    
    fd_set rfds;

    while(1) {
        // FD_ZERO(&rfds);
        // FD_SET(inotify_fd, &rfds);
        // FD_SET(selfpipe[0], &rfds);
        // select(max(inotify_fd, selfpipe[0])+1, &rfds, NULL, NULL, NULL);

        // if(FD_ISSET(selfpipe[0], &rfds)) {
        //     char ch;
        //     read(selfpipe[0], &ch, 1);
        //     kill(-current_process_group, SIGKILL);
        //     close(selfpipe[1]);
        //     close(selfpipe[0]);
        //     multi_watch_on = 0;
        //     for(auto &i:procfiles) remove(i.first.c_str());
        //     break;
        // }

        // if(FD_ISSET(inotify_fd, &rfds)) {
        //     char buf[BUF_LEN];
        //     int readlen = read(inotify_fd, buf, BUF_LEN), it = 0;
        //     while (it < readlen) {
        //         struct inotify_event *event = (struct inotify_event *) &buf[it];
        //         if(procfiles.count(event->name)) {
        //             auto fdetails = procfiles[event->name];
        //             multiWatchPrintUtil(fdetails.first, fdetails.second);
        //         }
        //         it += EVENT_SIZE + event->len;
        //     }
        // }

        char buf[BUF_LEN];
        int readlen = read(inotify_fd, buf, BUF_LEN), it = 0;
        if(readlen == -1) {
            kill(-current_process_group, SIGKILL);
            multi_watch_on = 0;
            for(auto &i:procfiles) remove(i.first.c_str());
            break;
        }
        while (it < readlen) {
            struct inotify_event *event = (struct inotify_event *) &buf[it];
            if(procfiles.count(event->name)) {
                auto fdetails = procfiles[event->name];
                multiWatchPrintUtil(fdetails.first, fdetails.second);
            }
            it += EVENT_SIZE + event->len;
        }
    }

    // inotify_rm_watch(inotify_fd, wd);
    // close(inotify_fd);
}
*/
