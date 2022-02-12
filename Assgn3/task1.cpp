#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <fcntl.h>
using namespace std;

typedef struct _process_data {
    double **A;
    double **B;
    double **C;
    int veclen, i, j;
} ProcessData;

void *mult(void *arg) {
    ProcessData *pd = (ProcessData *)arg;
    for(int it=0; it<pd->veclen; it++)
        pd->C[pd->i][pd->j] += (pd->A[pd->i][it] * pd->B[it][pd->j]);
}

signed main() {
    cout << setprecision(6);
    int r1, c1, r2, c2;
    cout << "Enter r1, c1, r2 and c2: ";
    cin >> r1 >> c1 >> r2 >> c2;

    if(c1 != r2) {
        printf("Cannot multiply two matrices of size %dx%d and %dx%d", r1, c1, r2, c2);
        exit(0);
    }

    int shm_id;
    if((shm_id = shmget(IPC_PRIVATE, ((r1 * c1) + (r2 * c2) + (r1 * c2)) * sizeof(double), IPC_CREAT | 0666)) == -1) {
        perror("Failed to create shared memory: ");
        exit(0);
    }
    
    int A_offset = 0;
    int B_offset = A_offset + (r1 * c1);
    int C_offset = B_offset + (r2 * c2);

    void *shm_data;
    if ((shm_data = shmat(shm_id, NULL, 0)) == (void *) -1) {
        perror("Failed to attach shared memory: ");
        exit(0);
    }
    double *matrices = (double *)shm_data;

    ProcessData *pd = (ProcessData *)malloc(sizeof(ProcessData));
    pd->A = (double **)malloc(r1 * sizeof(double *));
    for(int i=0; i<r1; i++) {
        pd->A[i] = matrices + A_offset + (c1 * i);
    }

    pd->B = (double **)malloc(r2 * sizeof(double *));
    for(int i=0; i<r2; i++) {
        pd->B[i] = matrices + B_offset + (c2 * i);
    }

    pd->C = (double **)malloc(r1 * sizeof(double *));
    for(int i=0; i<r1; i++) {
        pd->C[i] = matrices + C_offset + (c2 * i);
    }

    printf("Enter a %dx%d matrix:\n", r1, c1);
    for(int i=0; i<r1; i++) {
        for(int j=0; j<c1; j++) {
            cin >> pd->A[i][j];
        }
    }

    printf("Enter a %dx%d matrix:\n", r2, c2);
    for(int i=0; i<r2; i++) {
        for(int j=0; j<c2; j++) {
            cin >> pd->B[i][j];
        }
    }

    pd->veclen = c1;
    
    for(int i=0; i<r1; i++) {
        for(int j=0; j<c2; j++) {
            int cpid = fork();
            if(cpid == -1) {
                perror("Fork error: ");
                exit(0);
            } else if(cpid == 0) {
                pd->i = i;
                pd->j = j;
                mult(pd);
                exit(0);
            }
        }
    }

    while(wait(NULL) > 0) ;

    cout << "\nMultiplication complete \n\n";
    
    for(int i=0; i<r1; i++) {
        for(int j=0; j<c2; j++) {
            cout << pd->C[i][j] << ' ';
        }
        cout << '\n';
    }

    shmdt(shm_data);
    shmctl(shm_id, IPC_RMID, NULL);
    
    return 0;
}
