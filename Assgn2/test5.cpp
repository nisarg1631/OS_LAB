#include <bits/stdc++.h>
using namespace std;

signed main() {
    deque<int> dq;
    dq.push_front(9);
    dq.push_front(10);
    dq.push_front(11);
    dq.push_back(12);
    for(int i=0; i<dq.size(); i++) cout << dq[i] << endl;
    // for(auto i:dq) {
    //     cout << i << endl;
    // }
    return 0;
}