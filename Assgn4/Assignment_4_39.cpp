#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <pthread.h>
#include <chrono>
using namespace std;

const int MAX_NODES = 1500;
const int MAX_JOB_ID = 100000000;
const int MAX_JOB_COMPLETION_TIME = 250;

// FORWARD DECLARATIONS

// Shared mutex - INSPIRED BY : https://gist.github.com/yamnikov-oleg/abf61cf96b4867cbf72d
struct shared_mutex {
    pthread_mutex_t mutex_ptr;
    void init();
    void lock();
    void unlock();
};

struct node {
    /* 
        STATUS FIELD DEFINITIONS

        0 -> ready
        1 -> producer adding child
        2 -> consumer on going
        3 -> completed
    */
    int job_id, arr_ind, run_time, status, next_child_ind, incomplete_child_cnt, parent;
    shared_mutex status_update, dependency_update;
    int dependent_jobs[MAX_NODES];
    void init(int, int, int, int);
    void add_child();
    void remove_child();
};

struct shared_memory {
    int jobs_created;
    node tree[MAX_NODES];
    shared_mutex jobs_update, output_log;
    void init();
    int add_node(int);
} *master;

// DEFINITIONS

void shared_mutex::init() {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&(this->mutex_ptr), &attr);
}

void shared_mutex::lock() {
    pthread_mutex_lock(&(this->mutex_ptr));
}

void shared_mutex::unlock() {
    pthread_mutex_unlock(&(this->mutex_ptr));
}

void node::init(int job_id, int arr_ind, int run_time, int parent) {
    this->job_id = job_id;
    this->arr_ind = arr_ind;
    this->run_time = run_time;
    this->parent = parent;
    this->status = 0;
    this->incomplete_child_cnt = 0;
    this->next_child_ind = 0;
    status_update.init();
    dependency_update.init();
}

void node::add_child() {
    this->dependency_update.lock();
    (this->dependent_jobs)[(this->next_child_ind)++] = master->add_node(this->arr_ind);
    (this->incomplete_child_cnt)++;
    this->dependency_update.unlock();
}

void node::remove_child() {
    this->dependency_update.lock();
    (this->incomplete_child_cnt)--;
    this->dependency_update.unlock();
}

void shared_memory::init() {
    this->jobs_created = 0;
    this->jobs_update.init();
    this->output_log.init();
}

int shared_memory::add_node(int parent) {
    this->jobs_update.lock();
    int insert_pos = this->jobs_created;
    (this->tree)[insert_pos].init((rand() % MAX_JOB_ID) + 1, insert_pos, (rand() % MAX_JOB_COMPLETION_TIME) + 1, parent);
    (this->jobs_created)++;
    #ifndef SUPPRESS_OUTPUT
        master->output_log.lock();
        printf("\n -> New job added. Job ID: %d, Self index: %d, Runtime alloted: %d, Parent index: %d\n", (this->tree)[insert_pos].job_id, insert_pos, (this->tree)[insert_pos].run_time, parent);
        // printf("+ %d %d\n", insert_pos, parent);
        fflush(stdout);
        master->output_log.unlock();
    #endif
    this->jobs_update.unlock();
    return insert_pos;
}

void *producer(void *param) {
    srand(pthread_self());
    int thread_run_time = 10 + (rand() % 11);
    auto start = chrono::high_resolution_clock::now();
    do {
        master->jobs_update.lock();
        int ind = rand() % master->jobs_created;
        master->jobs_update.unlock();
        int found = 0;

        (master->tree)[ind].status_update.lock();
        if((master->tree)[ind].status == 0) {
            (master->tree)[ind].status = 1;
            found = 1;
        }
        (master->tree)[ind].status_update.unlock();

        if(found) {
            (master->tree)[ind].add_child();
            (master->tree)[ind].status_update.lock();
            (master->tree)[ind].status = 0;
            (master->tree)[ind].status_update.unlock();
            timespec sleepduration;
            sleepduration.tv_nsec = 1000000L * (200 + (rand() % 301));
            sleepduration.tv_sec = 0;
            nanosleep(&sleepduration, NULL);
        }
    } while(chrono::duration_cast<chrono::seconds>(chrono::high_resolution_clock::now() - start).count() < thread_run_time);
    pthread_exit(0);
}

void dfs(int curr_node) {
    (master->tree)[curr_node].status_update.lock();

    if((master->tree)[curr_node].status == 2 || (master->tree)[curr_node].status == 3) {
        (master->tree)[curr_node].status_update.unlock();
        return;
    }

    int found = 0;
    if((master->tree)[curr_node].status == 0) {
        (master->tree)[curr_node].dependency_update.lock();
        if((master->tree)[curr_node].incomplete_child_cnt == 0) {
            (master->tree)[curr_node].status = 2;
            found = 1;
        }
        (master->tree)[curr_node].dependency_update.unlock();
    }

    (master->tree)[curr_node].status_update.unlock();

    if(found) {
        #ifndef SUPPRESS_OUTPUT
            master->output_log.lock();
            printf("\n -> Job started. Job ID: %d, Self index: %d, Runtime alloted: %d, Parent index: %d\n", (master->tree)[curr_node].job_id, (master->tree)[curr_node].arr_ind, (master->tree)[curr_node].run_time, (master->tree)[curr_node].parent);
            // printf("~ %d %d\n", (master->tree)[curr_node].arr_ind, (master->tree)[curr_node].parent);
            fflush(stdout);
            master->output_log.unlock();
        #endif

        timespec sleepduration;
        sleepduration.tv_nsec = 1000000L * (master->tree)[curr_node].run_time;
        sleepduration.tv_sec = 0;
        nanosleep(&sleepduration, NULL);

        #ifndef SUPPRESS_OUTPUT
            master->output_log.lock();
            printf("\n -> Job completed. Job ID: %d, Self index: %d, Runtime alloted: %d, Parent index: %d\n", (master->tree)[curr_node].job_id, (master->tree)[curr_node].arr_ind, (master->tree)[curr_node].run_time, (master->tree)[curr_node].parent);
            // printf("- %d %d\n", (master->tree)[curr_node].arr_ind, (master->tree)[curr_node].parent);
            fflush(stdout);
            master->output_log.unlock();
        #endif

        (master->tree)[curr_node].status_update.lock();
        (master->tree)[curr_node].status = 3;
        (master->tree)[curr_node].status_update.unlock();
        (master->tree)[(master->tree)[curr_node].parent].remove_child();
    } else {
        for(int i = 0; i < (master->tree)[curr_node].next_child_ind; i++)
            dfs((master->tree)[curr_node].dependent_jobs[i]);
    }
}

void *consumer(void *param) {
    int root_status;
    do {
        dfs(0);
        (master->tree)[0].status_update.lock();
        root_status = (master->tree)[0].status;
        (master->tree)[0].status_update.unlock();
    }while(root_status != 2 && root_status != 3);
    pthread_exit(0);
}

// void dfs(int curr_node) {
//     cout << "Children of " << curr_node << " : ";
//     for(int i=0; i<(master->tree)[curr_node].next_child_ind; i++) {
//         cout << (master->tree)[curr_node].dependent_jobs[i] << " ";
//     }
//     cout << endl;
//     for(int i=0; i<(master->tree)[curr_node].next_child_ind; i++) {
//         dfs((master->tree)[curr_node].dependent_jobs[i]);
//     }
// }

signed main() {
    // freopen("output.txt","w", stdout);
    srand(time(NULL));

    int prod_thread_cnt, cons_thread_cnt;
    cout << "Enter number of producer threads: "; cin >> prod_thread_cnt;
    cout << "Enter number of consumer threads: "; cin >> cons_thread_cnt;
    
    int shm_id;
    if((shm_id = shmget(IPC_PRIVATE, sizeof(shared_memory), IPC_CREAT | 0666)) == -1) {
        perror("Failed to create shared memory: ");
        exit(0);
    }

    void *shm_data;
    if ((shm_data = shmat(shm_id, NULL, 0)) == (void *) -1) {
        perror("Failed to attach shared memory: ");
        exit(0);
    }

    master = (shared_memory *)shm_data;
    master->init();

    master->add_node(-1);
    int initial_jobs = 299 + (rand() % 201);
    // int initial_jobs = 5;
    
    while(initial_jobs) {
        int ind = rand() % master->jobs_created;
        (master->tree)[ind].add_child();
        initial_jobs--;
    }

    int cons_process_pid = fork();
    if(cons_process_pid == 0) {

        pthread_t cons_threads[cons_thread_cnt];
        for(int i = 0; i < cons_thread_cnt; i++) {
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_create(cons_threads + i, &attr, consumer, NULL);
        }

        for(int i = 0; i < cons_thread_cnt; i++)
            pthread_join(cons_threads[i], NULL);

        exit(0);

    } else if(cons_process_pid == -1) {
        perror("Consumer process fork error: ");
        exit(0);
    }

    pthread_t prod_threads[prod_thread_cnt];
    for(int i = 0; i < prod_thread_cnt; i++) {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(prod_threads + i, &attr, producer, NULL);
    }

    for(int i = 0; i < prod_thread_cnt; i++)
        pthread_join(prod_threads[i], NULL);
    printf("\nProducer process done.\n");

    while(wait(NULL) > 0) ;
    printf("\nConsumer process done.\n");

    printf("\nTotal jobs created: %d\n", master->jobs_created);

    shmdt(shm_data);
    shmctl(shm_id, IPC_RMID, NULL);

    return 0;
}
