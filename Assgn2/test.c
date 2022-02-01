#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>

signed main() {
    int n = 3;
    while(n--) {
        printf("Hello World from %d\n", getpid());
        sleep(2);
    }
    return 0;
}
