#include <stdio.h>
#include <termios.h>
#include <unistd.h>

int main() {
    struct termios old_tio, new_tio;
    unsigned char c;

    /* get the terminal settings for stdin */
    tcgetattr(STDIN_FILENO, &old_tio);

    /* we want to keep the old setting to restore them a the end */
    new_tio = old_tio;

    /* disable canonical mode (buffered i/o) and local echo */
    new_tio.c_lflag &= (~ICANON & ~ECHO);
    new_tio.c_cc[VMIN] = 1;
    new_tio.c_cc[VTIME] = 0;
    /* set the new settings immediately */
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
    int i = 0;
    char buff[10000];
    do {
        c = getchar();
        printf("%d\n", c);
        // if (c == 127) {
        //     printf("\b \b");
        //     i--;
        // } else {
        //     buff[i++] = c;
        //     printf("%c", c);
        // }
    } while (c != '\t' && c != '\n');
    // if (c == '1')
    // buff[i] = '\0';
    // printf("%s", buff);

    /* restore the former settings */
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);

    return 0;
}
