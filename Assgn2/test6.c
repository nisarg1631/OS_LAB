#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>

signed main() {
    int n = 5;
    while(n--) {
        // char buff[20];
        // scanf("%s", buff);
        // printf("%s\n", buff);
        printf("Ok Again\n");
        fflush(stdout);
        sleep(1);
    }
    return 0;
}
