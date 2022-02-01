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

int keep_running;

void signal_handler (int signum) {
    signal(SIGINT, signal_handler);
    keep_running = 0;
}

signed main() {
    keep_running = 1;
    signal(SIGINT, signal_handler);

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
    while(keep_running) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(inotify_fd, &rfds);
        select(inotify_fd + 1, &rfds, NULL, NULL, NULL);
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

    inotify_rm_watch(inotify_fd, wd);
    close(inotify_fd);

    return 0;
}
