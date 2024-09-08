#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <ctype.h>
#include <sys/ioctl.h>

#define VERSION "0.0.1"
#define CTRL_KEY(k) ((k) & 0x1f)
#define ABUF_INIT {NULL, 0}  // Macro to initialize dynamic append buffer

// Structures
struct abuf {
    char *b;
    int len;
};

struct editorConfig {
    int screenrows;
    int screencols;
    struct termios orig_attr;
};

struct editorConfig E;

// Buffer Management Functions
void abAppend(struct abuf *ab, const char *s, int len) {
    char *new = realloc(ab->b, ab->len + len);
    if (new == NULL) return;
    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}

void abFree(struct abuf *ab) {
    free(ab->b);
}

// Screen Drawing Functions
void drawRows(struct abuf *ab) {
    int y, welcomelen, padding;
    char welcome[80];

    for (y = 0; y < E.screenrows; y++) {
        if (y == E.screenrows / 3) {
            welcomelen = snprintf(welcome, sizeof(welcome), "Welcome Michael -- version %s", VERSION);
            if (welcomelen > E.screencols) welcomelen = E.screencols;

            padding = (E.screencols - welcomelen) / 2;
            if (padding) {
                abAppend(ab, "~", 1);
                padding--;
            }
            while (padding--) {
                abAppend(ab, " ", 1);
            }
            abAppend(ab, welcome, welcomelen);
        } else {
            abAppend(ab, "~", 1);
        }

        abAppend(ab, "\x1b[K", 3); // Clear the line
        if (y < E.screenrows - 1) abAppend(ab, "\r\n", 2);
    }
}

void refreshScreen() {
    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6);  // Hide cursor
    abAppend(&ab, "\x1b[H", 3);     // Move cursor to top left
    drawRows(&ab);                  // Draw rows with tildes and welcome message
    abAppend(&ab, "\x1b[H", 3);     // Move cursor back to top left
    abAppend(&ab, "\x1b[?25h", 6);  // Show cursor

    write(1, ab.b, ab.len);         // Write the buffer content to stdout
    abFree(&ab);                    // Free the buffer
}

// Terminal Mode Management
void disableRawMode() {
    if (tcsetattr(0, TCSAFLUSH, &E.orig_attr) == -1) {
        write(1, "\x1b[2J", 4);
        write(1, "\x1b[H", 3);
        perror("tcsetattr");
        exit(1);
    }
}

void enableRawMode() {
    if (tcgetattr(0, &E.orig_attr) == -1) {
        write(1, "\x1b[2J", 4);
        write(1, "\x1b[H", 3);
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

    if (tcsetattr(0, TCSAFLUSH, &raw) == -1) {
        write(1, "\x1b[2J", 4);
        write(1, "\x1b[H", 3);
        perror("tcsetattr raw");
        exit(1);
    }
}

// Input Handling Functions
char editorReadKey() {
    char c;
    if (read(1, &c, 1) == -1) {
        perror("read");
        exit(1);
    }
    printf("Key read: %c (0x%x)\n", c, c);  // Debugging keypress
    return c;
}

void editorProcessKeypress() {
    char c = editorReadKey();
    switch (c) {
        case CTRL_KEY('q'):
            write(1, "\x1b[2J", 4);  // Clear screen before quitting
            write(1, "\x1b[H", 3);   // Move cursor home
            exit(0);
            break;
    }
}

// Window Size Functions
int getWinSize(int *cols, int *rows) {
    struct winsize ws;

    if (ioctl(1, TIOCGWINSZ, &ws) != -1) {
        *rows = ws.ws_row;
        *cols = ws.ws_col;
        editorReadKey();  // Read a key after getting window size
        return 0;
    } else {
        return -1;
    }
}

void initEditor() {
    if (getWinSize(&E.screencols, &E.screenrows) == -1) {
        write(1, "\x1b[2J", 4);
        write(1, "\x1b[H", 3);
        perror("Init");
        exit(1);
    }
}

// Main Loop
int main() {
    enableRawMode();
    initEditor();

    while (1) {
        refreshScreen();
        editorProcessKeypress();
    }

    disableRawMode();
    return 0;
}
