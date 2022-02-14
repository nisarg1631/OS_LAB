#include <iostream>
#include <iomanip>
#include <stdio.h>
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
#include <chrono>
#include <random>
#include <time.h>
using namespace std;

const int MAT_SIZE = 1000;
const int MAX_QUEUE_SIZE = 10;
const int RANDOM_ID_RANGE_LO = 1;
const int RANDOM_ID_RANGE_HI = 100000;
const int MATRIX_ELEMENT_LO = -9;
const int MATRIX_ELEMENT_HI = 9;
const int WAIT_TIME_LO = 0;
const int WAIT_TIME_HI = 3000;

int total_jobs;
random_device rd;
mt19937 generator(rd());

vector<vector<int>> get_random_vectors(int cnt) {
    vector<int> v(RANDOM_ID_RANGE_HI - RANDOM_ID_RANGE_LO + 1);
    iota(v.begin(), v.end(), 1);
    shuffle(v.begin(), v.end(), generator);
    vector<vector<int>> r(cnt);
    int it = 0;
    while(!v.empty()) {
        r[it].push_back(v.back());
        it = (it + 1) % cnt;
        v.pop_back();
    }
    return r;
}

int get_random_matrix_entry() {
    static uniform_int_distribution<int> range(MATRIX_ELEMENT_LO, MATRIX_ELEMENT_HI);
    return range(generator);
}

int get_random_wait_time() {
    static uniform_int_distribution<int> range(WAIT_TIME_LO, WAIT_TIME_HI);
    return range(generator);
}

// FORWARD DECLARATIONS

// Shared mutex - INSPIRED BY : https://gist.github.com/yamnikov-oleg/abf61cf96b4867cbf72d
struct shared_mutex {
    pthread_mutex_t *mutex_ptr;
    void init();
} *sm_updatequeue, *sm_updatemat, *sm_jobscount, *sm_output;

struct comp_job {
    int producer_number, matrix_id, status, result_index;
    /* 
        STATUS FIELD DEFINITIONS

        MSB (31st) represents whether comp job is ready or not, it may so happen comp job is in queue but still not ready, i.e., there are workers still finishing the job
        
        Bits 0-7 represent which block D_ijk has been assigned for multiplication
        e.g., bit 0 corresponds to 000 (i=0, j=0, k=0)
        e.g., bit 5 corresponds to 101 (i=1, j=0, k=1)
        
        Bits 8-11 represent which block C_ij has been accessed once, after shifting by 8 bits to right we can see it as
        e.g., bit 8 (0 after shifting right 8 places) corresponds to 00 (i=0, j=0)
        e.g., bit 10 (2 after shifting right 8 places) corresponds to 10 (i=1, j=0)
        
        Bits 12-15 represnt which block C_ij has been accessed twice (means calculation complete for that block)
        e.g., bit 12 (0 after shifting right 12 places) corresponds to 00 (i=0, j=0)
        e.g., bit 15 (3 after shifting right 12 places) corresponds to 11 (i=1, j=1) 
    */
    int matrix[MAT_SIZE][MAT_SIZE];

    comp_job(int, int);
};

struct worker_task {
    int result_index, block_flag;
    // Matrix Block 1
    int block_A[MAT_SIZE/2][MAT_SIZE/2];
    int matrix1_id, matrix1_creator;
    // Matrix Block 2
    int block_B[MAT_SIZE/2][MAT_SIZE/2];
    int matrix2_id, matrix2_creator;
    // Block Product
    int block_product[MAT_SIZE/2][MAT_SIZE/2];
};

struct job_queue {
    comp_job jobs[MAX_QUEUE_SIZE];
    int count, front, back;

    void init();
    bool fetch(worker_task &, vector<int> &, int);
    void update(worker_task &, int);
    int push(const comp_job &, int);
    bool pop();
};

struct shared_memory {
    int jobs_created;
    job_queue queue;

    void init();
} *master;

// DEFINITIONS

void shared_mutex::init() {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(mutex_ptr, &attr);
}

comp_job::comp_job(int prod_num, int mat_id) : producer_number(prod_num), matrix_id(mat_id), status(0), result_index(-1) {}

void job_queue::init() {
    count = front = back = 0;
}

bool job_queue::fetch(worker_task &task, vector<int> &random_ids, int worker_number) {
    bool success = false;
    pthread_mutex_lock(sm_updatequeue->mutex_ptr);
    if(master->jobs_created == total_jobs && count == 1) {
        pthread_mutex_unlock(sm_updatequeue->mutex_ptr);
        #ifndef SUPPRESS_OUTPUT
            pthread_mutex_lock(sm_output->mutex_ptr);
            printf("\n -> Worker with number %d terminated. PID: %d\n", worker_number, getpid());
            pthread_mutex_unlock(sm_output->mutex_ptr);
        #endif
        exit(0);
    }
    if(count > 1) {
        pthread_mutex_lock(sm_updatemat->mutex_ptr);
        int ready1 = jobs[front].status & (1 << 31);
        int ready2 = jobs[(front + 1) % MAX_QUEUE_SIZE].status & (1 << 31);
        if(ready1 != 0 && ready2 != 0) {
            success = true;
            int flag;
            for(flag = 0; flag < 8; flag++) {
                if((jobs[front].status & (1 << flag)) == 0)
                    break;
            }
            task.block_flag = flag;
            jobs[front].status |= (1 << flag);
            // Assign task.block_A and task.block_B here
            int i = (flag >> 2) & 1;
            int j = (flag >> 1) & 1;
            int k = (flag >> 0) & 1;
            int row_it = 0, col_it = 0;
            for(int row = i * (MAT_SIZE/2); row < (i + 1) * (MAT_SIZE/2); row++) {
                for(int col = k * (MAT_SIZE/2); col < (k + 1) * (MAT_SIZE/2); col++) {
                    task.block_A[row_it][col_it++] = jobs[front].matrix[row][col];
                }
                col_it = 0;
                row_it++;
            }
            row_it = 0, col_it = 0;
            for(int row = k * (MAT_SIZE/2); row < (k + 1) * (MAT_SIZE/2); row++) {
                for(int col = j * (MAT_SIZE/2); col < (j + 1) * (MAT_SIZE/2); col++) {
                    task.block_B[row_it][col_it++] = jobs[(front + 1) % MAX_QUEUE_SIZE].matrix[row][col];
                }
                col_it = 0;
                row_it++;
            }
            if(flag == 0) {
                // First access, push a new matrix to queue
                comp_job cj(-worker_number, random_ids.back());
                random_ids.pop_back();
                while((jobs[front].result_index = push(cj, 0)) == -1) ; // wait until push is successful, if everything works right it will push on the first try itself
            }
            task.result_index = jobs[front].result_index;
            task.matrix1_id = jobs[front].matrix_id;
            task.matrix1_creator = jobs[front].producer_number;
            task.matrix2_id = jobs[(front + 1) % MAX_QUEUE_SIZE].matrix_id;
            task.matrix2_creator = jobs[(front + 1) % MAX_QUEUE_SIZE].producer_number;
            #ifndef SUPPRESS_OUTPUT
                pthread_mutex_lock(sm_output->mutex_ptr);
                printf("\n ---- WORKER FETCHED BLOCKS ----\n");
                printf(" -> Worker number: %d\n", worker_number);
                printf(" -> PID: %d\n", getpid());
                printf(" -> Matrix 1 details:\n");
                printf("    [ Matrix ID: %d ]\n", task.matrix1_id);
                printf("    [ Block fetched: %d%d ]\n", i, k);
                printf("    [ Added by: %s %d ]\n", (task.matrix1_creator > 0 ? "Producer" : "Worker"), abs(task.matrix1_creator));
                printf(" -> Matrix 2 details:\n");
                printf("    [ Matrix ID: %d ]\n", task.matrix2_id);
                printf("    [ Block fetched: %d%d ]\n", k, j);
                printf("    [ Added by: %s %d ]\n", (task.matrix2_creator > 0 ? "Producer" : "Worker"), abs(task.matrix2_creator));
                printf(" -> Work done: Read blocks\n");
                printf(" -------------------------------\n");
                pthread_mutex_unlock(sm_output->mutex_ptr);
            #endif
            if(flag == 7) {
                // Last access, remove A and B from queue
                pop();
                pop();
            }
        }
        pthread_mutex_unlock(sm_updatemat->mutex_ptr);
    }
    pthread_mutex_unlock(sm_updatequeue->mutex_ptr);
    return success;
}

void job_queue::update(worker_task &task, int worker_number) {
    pthread_mutex_lock(sm_updatemat->mutex_ptr);
    // Modify C_ij (jobs[task.result_index].matrix) here
    int i = (task.block_flag >> 2) & 1;
    int j = (task.block_flag >> 1) & 1;
    int k = (task.block_flag >> 0) & 1;
    task.block_flag >>= 1;
    bool copy_task = false;
    if((jobs[task.result_index].status & (1 << (8 + task.block_flag))) == 0) {
        copy_task = true;
        // First modification to C_ij, so copy the task.block_product to C_ij
        int row_it = 0, col_it = 0;
        for(int row = i * (MAT_SIZE/2); row < (i + 1) * (MAT_SIZE/2); row++) {
            for(int col = j * (MAT_SIZE/2); col < (j + 1) * (MAT_SIZE/2); col++) {
                jobs[task.result_index].matrix[row][col] = task.block_product[row_it][col_it++];
            }
            col_it = 0;
            row_it++;
        }
        jobs[task.result_index].status |= (1 << (8 + task.block_flag));
    } else {
        // Second modification to C_ij, so add the task.block_product to C_ij
        int row_it = 0, col_it = 0;
        for(int row = i * (MAT_SIZE/2); row < (i + 1) * (MAT_SIZE/2); row++) {
            for(int col = j * (MAT_SIZE/2); col < (j + 1) * (MAT_SIZE/2); col++) {
                jobs[task.result_index].matrix[row][col] += task.block_product[row_it][col_it++];
            }
            col_it = 0;
            row_it++;
        }
        jobs[task.result_index].status |= (1 << (12 + task.block_flag));
    }
    #ifndef SUPPRESS_OUTPUT
        pthread_mutex_lock(sm_output->mutex_ptr);
        printf("\n ---- WORKER WRITING BLOCK ----\n");
        printf(" -> Worker number: %d\n", worker_number);
        printf(" -> PID: %d\n", getpid());
        printf(" -> Matrix 1 details:\n");
        printf("    [ Matrix ID: %d ]\n", task.matrix1_id);
        printf("    [ Block fetched: %d%d ]\n", i, k);
        printf("    [ Added by: %s %d ]\n", (task.matrix1_creator > 0 ? "Producer" : "Worker"), abs(task.matrix1_creator));
        printf(" -> Matrix 2 details:\n");
        printf("    [ Matrix ID: %d ]\n", task.matrix2_id);
        printf("    [ Block fetched: %d%d ]\n", k, j);
        printf("    [ Added by: %s %d ]\n", (task.matrix2_creator > 0 ? "Producer" : "Worker"), abs(task.matrix2_creator));
        printf(" -> Work done: %s\n", (copy_task ? "Copy block" : "Add block"));
        printf(" -> Resultant matrix details:\n");
        printf("    [ Matrix ID: %d ]\n", jobs[task.result_index].matrix_id);
        printf("    [ Block modified: %d%d ]\n", i, j);
        printf("    [ Added by: %s %d ]\n", (jobs[task.result_index].producer_number > 0 ? "Producer" : "Worker"), abs(task.matrix1_creator)); // If everything works alright this should always print a Worker
        printf(" ------------------------------\n");
        pthread_mutex_unlock(sm_output->mutex_ptr);
    #endif
    int done = 1;
    for(int i=12; i<16; i++) {
        if((jobs[task.result_index].status & (1 << i)) == 0) {
            done = 0;
            break;
        }
    }
    // If all blocks completed set ready bit to 1
    if(done)
        jobs[task.result_index].status |= (1 << 31);
    pthread_mutex_unlock(sm_updatemat->mutex_ptr);
}

int job_queue::push(const comp_job &cj, int is_producer) {
    int success = -1;
    pthread_mutex_lock(sm_updatequeue->mutex_ptr);
    if(count < MAX_QUEUE_SIZE - is_producer) {
        count++;
        jobs[back] = cj;
        success = back;
        back = (back + 1) % MAX_QUEUE_SIZE;
        #ifndef SUPPRESS_OUTPUT
            pthread_mutex_lock(sm_output->mutex_ptr);
            printf("\n ---- JOB ADDED TO QUEUE ----\n");
            printf(" -> Added by: %s %d\n", (is_producer ? "Producer" : "Worker"), (is_producer ? cj.producer_number : -cj.producer_number));
            printf(" -> PID: %d\n", getpid());
            printf(" -> Matrix ID: %d\n", cj.matrix_id);
            printf(" ----------------------------\n");
            pthread_mutex_unlock(sm_output->mutex_ptr);
        #endif
    }
    pthread_mutex_unlock(sm_updatequeue->mutex_ptr);
    return success;
}

bool job_queue::pop() {
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

void shared_memory::init() {
    jobs_created = 0;
    queue.init();
}

void producer(const int producer_number, vector<int> random_ids) {
    #ifndef SUPPRESS_OUTPUT
        pthread_mutex_lock(sm_output->mutex_ptr);
        printf("\n -> Producer with number %d created. PID: %d\n", producer_number, getpid());
        pthread_mutex_unlock(sm_output->mutex_ptr);
    #endif
    while(1) {
        comp_job cj(producer_number, random_ids.back());
        cj.status |= (1 << 31);
        random_ids.pop_back();
        // Generate random matrix here
        for(int i = 0; i < MAT_SIZE; i++) {
            for(int j = 0; j < MAT_SIZE; j++) {
                cj.matrix[i][j] = get_random_matrix_entry();
                // uncomment to generate identity matrices and check if multiplication algorithm is working fine
                // cj.matrix[i][j] = (i == j ? 1 : 0);
            }
        }
        pthread_mutex_lock(sm_jobscount->mutex_ptr);
        if(master->jobs_created == total_jobs) {
            pthread_mutex_unlock(sm_jobscount->mutex_ptr);
            break;
        }
        do {
            timespec sleepduration;
            sleepduration.tv_nsec = 1000000L * (long)get_random_wait_time();
            sleepduration.tv_sec = sleepduration.tv_nsec / 1000000000L;
            sleepduration.tv_nsec %= 1000000000L;
            nanosleep(&sleepduration, NULL);
        } while((master->queue).push(cj, 1) == -1); // wait until push is successful
        (master->jobs_created)++;
        pthread_mutex_unlock(sm_jobscount->mutex_ptr);
    }
    #ifndef SUPPRESS_OUTPUT
        pthread_mutex_lock(sm_output->mutex_ptr);
        printf("\n -> Producer with number %d terminated. PID: %d\n", producer_number, getpid());
        pthread_mutex_unlock(sm_output->mutex_ptr);
    #endif
    exit(0);
}

void worker(const int worker_number, vector<int> random_ids) {
    #ifndef SUPPRESS_OUTPUT
        pthread_mutex_lock(sm_output->mutex_ptr);
        printf("\n -> Worker with number %d created. PID: %d\n", worker_number, getpid());
        pthread_mutex_unlock(sm_output->mutex_ptr);
    #endif
    while(1) {
        worker_task task;
        do {
            timespec sleepduration;
            sleepduration.tv_nsec = 1000000L * (long)get_random_wait_time();
            sleepduration.tv_sec = sleepduration.tv_nsec / 1000000000L;
            sleepduration.tv_nsec %= 1000000000L;
            nanosleep(&sleepduration, NULL);
        } while(!(master->queue).fetch(task, random_ids, worker_number)); // wait until two jobs are fetched
        // Multiply task.block_A and task.block_B, store result in task.block_product
        for(int i = 0; i < (MAT_SIZE/2); i++) {
            for(int j = 0; j < (MAT_SIZE/2); j++) {
                task.block_product[i][j] = 0;
                for(int k = 0; k < (MAT_SIZE/2); k++) {
                    task.block_product[i][j] += (task.block_A[i][k] * task.block_B[k][j]);
                }
            }
        }
        (master->queue).update(task, worker_number);
    }
    exit(0);
}

signed main() {
    srand(time(0));

    int np, nw;
    cout << "Enter number of producers: ";
    cin >> np;
    cout << "Enter number of workers: ";
    cin >> nw;
    cout << "Enter total matrices to multiply: ";
    cin >> total_jobs;

    int shm_id;
    if((shm_id = shmget(IPC_PRIVATE, (4 * sizeof(pthread_mutex_t)) + sizeof(shared_memory), IPC_CREAT | 0666)) == -1) {
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
    sm_output = (shared_mutex *)malloc(sizeof(shared_mutex));

    sm_updatequeue->mutex_ptr = (pthread_mutex_t *)shm_data;
    sm_updatemat->mutex_ptr = (sm_updatequeue->mutex_ptr) + 1;
    sm_jobscount->mutex_ptr = (sm_updatemat->mutex_ptr) + 1;
    sm_output->mutex_ptr = (sm_jobscount->mutex_ptr) + 1;

    master = (shared_memory *) ((sm_output->mutex_ptr) + 1);

    sm_updatequeue->init();
    sm_updatemat->init();
    sm_jobscount->init();
    sm_output->init();

    master->init();

    vector<vector<int>> random_vecs = get_random_vectors(np + nw);

    auto start = chrono::high_resolution_clock::now();

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

    auto stop = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(stop - start); 
    cout << "\nTime taken: " << duration.count() << " milliseconds" << endl;

    int ans = 0;
    for(int i = 0; i < MAT_SIZE; i++)
        ans += (master->queue).jobs[(master->queue).front].matrix[i][i];

    cout << "\nSum of the elements in the principal diagonal of the final matrix: " << ans << endl;

    shmdt(shm_data);
    shmctl(shm_id, IPC_RMID, NULL);

    return 0;
}
