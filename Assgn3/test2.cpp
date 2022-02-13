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
    vector<int> v(RANDOM_ID_RANGE_HI - RANDOM_ID_RANGE_LO + 1);
    iota(v.begin(), v.end(), 1);
    random_shuffle(v.begin(), v.end());
    vector<vector<int>> r(cnt);
    int it = 0;
    while(!v.empty()) {
        r[it].push_back(v.back());
        it = (it + 1) % cnt;
        v.pop_back();
    }
    return r;
}

void foo1(int &n) {
    n = 10;
}

void foo2(int &n) {
    foo1(n);
}

signed main(int argc, char *argv[]) {
    // auto vecs = get_random_vectors(atoi(argv[1]));
    // for(auto i:vecs) {
    //     for(auto j:i) cout << j << ' ';
    //     cout << '\n';
    // }
    int n = -1;
    cout << n << endl;
    foo2(n);
    cout << n << endl;
    return 0;
}
