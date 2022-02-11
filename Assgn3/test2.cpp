#include <iostream>
using namespace std;

signed main(int argc, char *argv[]) {
    int n = atoi(argv[1]);
    for(int i=0; i<4; i++)
        cout << n << ' ';
    cout << '\n';
    for(int i=0; i<n; i++) {
        for(int j=0; j<n; j++) {
            cout << (i==j ? 1 : 0) << ' ';
        }
        cout << '\n';
    }
    for(int i=0; i<n; i++) {
        for(int j=0; j<n; j++) {
            cout << (i==j ? 1 : 0) << ' ';
        }
        cout << '\n';
    }
    return 0;
}
