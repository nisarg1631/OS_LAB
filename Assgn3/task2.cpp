#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <pthread.h>
#include <stack>
#include <vector>
#include <algorithm>
#include <numeric>
using namespace std;

const int MAT_SIZE = 100;
const int MAX_QUEUE_SIZE = 10;
const int RANDOM_ID_RANGE_LO = 1;
const int RANDOM_ID_RANGE_HI = 100000;

int total_jobs;

vector<vector<int>> get_random_vectors(int cnt) {
    srand(time(0));
    vector<int> v(RANDOM_ID_RANGE_HI - RANDOM_ID_RANGE_LO + 1);
    iota(v.begin(), v.end(), 1);
    random_shuffle(v.begin(), v.end());
    vector<vector<int>> r(cnt);
    int it = 0;
    while(!v.empty()) {
        r[it].push_back(v.back());
        it = (it + 1) % cnt;
    }
    return r;
}

// INSPIRED BY : https://gist.github.com/yamnikov-oleg/abf61cf96b4867cbf72d

struct shared_mutex {
    pthread_mutex_t *mutex_ptr;
    
    void init() {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(mutex_ptr, &attr);
    }

} *sm_updatequeue, *sm_updatemat, *sm_jobscount;

struct comp_job {
    int producer_number, matrix_id, status, result_index;
    /* 
        STATUS FIELD DEFINITIONS

        MSB (31st) represents whether comp job is ready or not, it may so happen comp job is in queue but still not ready, i.e., there are workers still finishing the job
        
        Bits 0-7 represent which block Dijk has been assigned for multiplication
        e.g., bit 0 corresponds to 000 (i=0, j=0, k=0)
        e.g., bit 5 corresponds to 101 (i=1, j=0, k=1)
        
        Bits 8-11 represent which block Cij has been accessed once, after shifting by 8 bits to right we can see it as
        e.g., bit 8 (0 after shifting right 8 places) corresponds to 00 (i=0, j=0)
        e.g. bit 10 (2 after shifting right 8 places) corresponds to 10 (i=1, j=0)
        
        Bits 12-15 represnt which block Cij has been accessed twice (means calculation complete for that block)
        e.g., bit 12 (0 after shifting right 12 places) corresponds to 00 (i=0, j=0)
        e.g. bit 15 (3 after shifting right 12 places) corresponds to 11 (i=1, j=1) 
    */
    int matrix[MAT_SIZE][MAT_SIZE];

    comp_job(int prod_num, int mat_id) : producer_number(prod_num), matrix_id(mat_id), status(0), result_index(-1) {}
};

struct job_queue {
    comp_job jobs[MAX_QUEUE_SIZE];
    int count, front, back;

    void init() {
        count = front = back = 0;
    }

    bool fetch(int block_A[MAT_SIZE/2][MAT_SIZE/2], int block_B[MAT_SIZE/2][MAT_SIZE/2], int &block_flag, int &result_index, vector<int> &random_ids) {
        bool success = false;
        pthread_mutex_lock(sm_updatequeue->mutex_ptr);
        if(master->jobs_created == total_jobs && count == 1) 
            exit(0);
        if(count > 1) {
            pthread_mutex_lock(sm_updatemat->mutex_ptr);
            int ready1 = jobs[front].status & (1<<31);
            int ready2 = jobs[(front + 1) % MAX_QUEUE_SIZE].status & (1<<31);
            if(ready1 != 0 && ready2 != 0) {
                success = true;
                int flag;
                for(flag = 0; flag < 8; flag++) {
                    if((jobs[front].status & (1<<flag)) == 0)
                        break;
                }
                block_flag = flag;
                jobs[front].status |= (1<<flag);
                // TODO: Assign block_A and block_B here
                if(flag == 0) {
                    // First access, push a new matrix to queue
                    comp_job cj(-1, random_ids.back());
                    random_ids.pop_back();
                    while((jobs[front].result_index = push(cj, 0)) == -1) ; // wait until push is successful, if everything works right it will push on the first try itself
                }
                if(flag == 7) {
                    // Last access, remove A and B from queue
                }
                result_index = jobs[front].result_index;
            }
            pthread_mutex_unlock(sm_updatemat->mutex_ptr);
        }
        pthread_mutex_unlock(sm_updatequeue->mutex_ptr);
        return success;
    }

    int push(comp_job cj, int is_producer) {
        int success = -1;
        pthread_mutex_lock(sm_updatequeue->mutex_ptr);
        if(count < MAX_QUEUE_SIZE - is_producer) {
            count++;
            jobs[back] = cj;
            success = back;
            back = (back + 1) % MAX_QUEUE_SIZE;
        }
        pthread_mutex_unlock(sm_updatequeue->mutex_ptr);
        return success;
    }

    bool pop() {
        bool success = false;
        pthread_mutex_lock(sm_updatequeue->mutex_ptr);
        if(count > 0) {
            count--;
            front = (front + 1) % MAX_QUEUE_SIZE;
            success = true;
        }
        pthread_mutex_unlock(sm_updatequeue->mutex_ptr);
        return success;
    }
};

struct shared_memory {
    int jobs_created;
    job_queue queue;

    void init() {
        jobs_created = 0;
        queue.init();
    }

} *master;

void producer(const int producer_number, vector<int> random_ids) {
    while(1) {
        comp_job cj(producer_number, random_ids.back());
        random_ids.pop_back();
        // TODO: Generate random matrix here
        pthread_mutex_lock(sm_jobscount->mutex_ptr);
        if(master->jobs_created == total_jobs)
            break;
        sleep(1); // TODO: Make this sleep random between 0 and 3000 ms
        while((master->queue).push(cj, 1) == -1) ; // wait until push is successful
        (master->jobs_created)++;
        pthread_mutex_unlock(sm_jobscount->mutex_ptr);
    }
    exit(0);
}

void worker(const int worker_number, vector<int> random_ids) {
    while(1) {

    }
    exit(0);
}

signed main() {

    int np, nw;
    cin >> np >> nw;
    cin >> total_jobs;

    int shm_id;
    if((shm_id = shmget(IPC_PRIVATE, (3 * sizeof(pthread_mutex_t)) + sizeof(shared_memory), IPC_CREAT | 0666)) == -1) {
        perror("Failed to create shared memory: ");
        exit(0);
    }

    void *shm_data;
    if ((shm_data = shmat(shm_id, NULL, 0)) == (void *) -1) {
        perror("Failed to attach shared memory: ");
        exit(0);
    }

    sm_updatequeue = (shared_mutex *)malloc(sizeof(shared_mutex));
    sm_updatemat = (shared_mutex *)malloc(sizeof(shared_mutex));
    sm_jobscount = (shared_mutex *)malloc(sizeof(shared_mutex));

    sm_updatequeue->mutex_ptr = (pthread_mutex_t *)shm_data;
    sm_updatemat->mutex_ptr = (sm_updatequeue->mutex_ptr) + 1;
    sm_jobscount->mutex_ptr = (sm_updatemat->mutex_ptr) + 1;

    master = (shared_memory *) ((sm_jobscount->mutex_ptr) + 1);

    sm_updatequeue->init();
    sm_updatemat->init();
    sm_jobscount->init();

    master->init();

    vector<vector<int>> random_vecs = get_random_vectors(np + nw);
    int it = 0;
    for(int i = 0; i < np; i++, it++) {
        int cpid = fork();
        if(cpid == 0) {
            producer(i+1, random_vecs[it]);
        } else if(cpid == -1) {
            perror("Producer fork error: ");
        }
    }
    for(int i = 0; i < nw; i++, it++) {
        int cpid = fork();
        if(cpid == 0) {
            worker(i+1, random_vecs[it]);
        } else if(cpid == -1) {
            perror("Worker fork error: ");
        }
    }

    while(wait(NULL) > 0) ;

    shmdt(shm_data);
    shmctl(shm_id, IPC_RMID, NULL);

    return 0;
}
