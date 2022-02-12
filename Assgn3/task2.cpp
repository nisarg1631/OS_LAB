#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <pthread.h>
using namespace std;

// INSPIRED BY : https://gist.github.com/yamnikov-oleg/abf61cf96b4867cbf72d

struct shared_mutex {
    pthread_mutex_t *mutex_ptr;
    char *name;
} *sm;

signed main() {
    int shm_id;
    if((shm_id = shmget(IPC_PRIVATE, sizeof(pthread_mutex_t), IPC_CREAT | 0666)) == -1) {
        perror("Failed to create shared memory: ");
        exit(0);
    }

    void *shm_data;
    if ((shm_data = shmat(shm_id, NULL, 0)) == (void *) -1) {
        perror("Failed to attach shared memory: ");
        exit(0);
    }

    sm = (shared_mutex *)malloc(sizeof(shared_mutex));

    sm->mutex_ptr = (pthread_mutex_t *)shm_data;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(sm->mutex_ptr, &attr);
    
    int n=5;
    for(int i=0; i<n; i++) {
        int cpid = fork();
        if(cpid == 0) {
            pthread_mutex_lock(sm->mutex_ptr);
            printf("Hi from process %d\n", getpid());
            sleep(2);
            printf("Bye from process %d\n", getpid());
            pthread_mutex_unlock(sm->mutex_ptr);
            exit(0);
        }
    }
    while(wait(NULL) > 0) ;

    shmdt(shm_data);
    shmctl(shm_id, IPC_RMID, NULL);

    return 0;
}
