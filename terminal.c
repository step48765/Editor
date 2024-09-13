#include "editor.h"



// Terminal Mode Management
void disableRawMode(void) {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_attr) == -1) {
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
        perror("tcsetattr");
        exit(1);
    }
}

void enableRawMode(void) {
    if (tcgetattr(STDIN_FILENO, &E.orig_attr) == -1) {
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
        perror("tcgetattr");
        exit(1);
    }

    struct termios raw = E.orig_attr;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
        perror("tcsetattr raw");
        exit(1);
    }
}

int editorReadKey(void) {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }

    if (c == '\x1b') {
        char seq[3];

        // Read the next two characters to check if it's an escape sequence
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        // Check if the sequence is an arrow key
        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': return ARROW_UP;
                case 'B': return ARROW_DOWN;
                case 'C': return ARROW_RIGHT;
                case 'D': return ARROW_LEFT;
            }
        }

        return '\x1b';  // Return escape if no valid sequence was found
    } else {
        return c;  // Return the character if it's not an escape sequence
    }
}

int getWinSize(int *cols, int *rows) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != -1) {
        *rows = ws.ws_row;
        *cols = ws.ws_col;
        return 0;
    } else {
        return -1;
    }
}



