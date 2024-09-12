#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <time.h>
#include <sys/errno.h>

#define VERSION "0.0.1"
#define KILO_TAB_STOP 8
#define CTRL_KEY(k) ((k) & 0x1f)
#define ABUF_INIT {NULL, 0}  // Macro to initialize dynamic append buffer

void editorInsertChar(int c);

enum editorKey {
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  DEL_KEY,
  HOME_KEY,
  END_KEY,
  PAGE_UP,
  PAGE_DOWN
};

typedef struct erow {
    int size;
    int rsize;    // Rendered size
    char *chars;  // Original row content
    char *render; // Rendered content (tabs as spaces, etc.)
} erow;

// Structures
struct abuf {
    char *b;
    int len;
};

struct editorConfig {
    int cx, cy;   // Cursor position
    int rx;       // Render index for cursor
    int rowoff;   // Row offset for vertical scrolling
    int coloff;   // Column offset for horizontal scrolling
    int screenrows;
    int screencols;
    int numrows;
    char *filename;
    char statusmsg[80];
    time_t statusmsg_time;
    erow *row;
    struct termios orig_attr;
};

struct editorConfig E;

// Function Prototypes
void abAppend(struct abuf *ab, const char *s, int len);
void abFree(struct abuf *ab);
void editorScroll();
void drawRows(struct abuf *ab);
void editorDrawStatusBar(struct abuf *ab);
void editorDrawMessageBar(struct abuf *ab);
void refreshScreen();
void editorSetStatusMessage(const char *msg);
void disableRawMode();
void enableRawMode();
int editorReadKey();
void moveCursor(int key);
void editorProcessKeypress();
int getWinSize(int *cols, int *rows);
void editorAppendRow(char *s, size_t len);
void editorRowInsertChar(erow *row, int at, int c);
void editorInsertChar(int c);
void editorOpen(char *filename);
void initEditor();
void editorUpdateRow(erow *row);
int editorRowCxToRx(erow *row, int cx);
void editorSave();

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
void die(const char *s) {
  perror(s);
  exit(1);
}

void editorScroll() {
    E.rx = 0;
    if (E.cy < E.numrows) {
        E.rx = editorRowCxToRx(&E.row[E.cy], E.cx);
    }
    if (E.cy < E.rowoff) {
        E.rowoff = E.cy;
    }
    if (E.cy >= E.rowoff + E.screenrows) {
        E.rowoff = E.cy - E.screenrows + 1;
    }
    if (E.rx < E.coloff) {
        E.coloff = E.rx;
    }
    if (E.rx >= E.coloff + E.screencols) {
        E.coloff = E.rx - E.screencols + 1;
    }
}

// Screen Drawing Functions 
void drawRows(struct abuf *ab) {
    int y;
    for (y = 0; y < E.screenrows; y++) {
        int filerow = y + E.rowoff;
        if (filerow >= E.numrows) {
            if (E.numrows == 0 && y == E.screenrows / 3) {
                char welcome[80];
                int welcomelen = snprintf(welcome, sizeof(welcome),
                  "Welcome Michael -- version %s", VERSION);
                if (welcomelen > E.screencols) welcomelen = E.screencols;
                int padding = (E.screencols - welcomelen) / 2;
                if (padding) {
                    abAppend(ab, "~", 1);
                    padding--;
                }
                while (padding--) abAppend(ab, " ", 1);
                abAppend(ab, welcome, welcomelen);
            } else {
                abAppend(ab, "~", 1);
            }
        } else {
            int len = E.row[filerow].rsize - E.coloff;
            if (len < 0) len = 0;
            if (len > E.screencols) len = E.screencols;
            abAppend(ab, &E.row[filerow].render[E.coloff], len);
        }
        abAppend(ab, "\x1b[K", 3);
        abAppend(ab, "\r\n", 2);
    }
}

void editorDrawStatusBar(struct abuf *ab) {
    abAppend(ab, "\x1b[7m", 4);  // Invert colors
    char status[80], rstatus[80];
    int len = snprintf(status, sizeof(status), "%.20s - %d lines",
        E.filename ? E.filename : "[No Name]", E.numrows);
    int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d",
        E.cy + 1, E.numrows);
    if (len > E.screencols) len = E.screencols;
    abAppend(ab, status, len);
    while (len < E.screencols) {
        if (E.screencols - len == rlen) {
            abAppend(ab, rstatus, rlen);
            break;
        } else {
            abAppend(ab, " ", 1);
            len++;
        }
    }
    abAppend(ab, "\x1b[m", 3);  // Reset text style after the status bar
    abAppend(ab, "\r\n", 2);    // Newline after status bar
}


void editorDrawMessageBar(struct abuf *ab) {
    abAppend(ab, "\x1b[K", 3);
    int msglen = strlen(E.statusmsg);
    if (msglen > E.screencols) msglen = E.screencols;
    if (msglen && time(NULL) - E.statusmsg_time < 5)
        abAppend(ab, E.statusmsg, msglen);
}

void refreshScreen() {
    editorScroll();  // Update E.rx based on E.cx

    struct abuf ab = ABUF_INIT;
    abAppend(&ab, "\x1b[?25l", 6);  // Hide the cursor
    abAppend(&ab, "\x1b[H", 3);     // Move cursor to the top-left corner

    drawRows(&ab);                  // Draw all the rows
    editorDrawStatusBar(&ab);       // Draw the status bar
    editorDrawMessageBar(&ab);      // Draw the message bar

    // Move the cursor to the correct position based on E.cy and E.rx
    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, (E.rx - E.coloff) + 1);
    abAppend(&ab, buf, strlen(buf));

    abAppend(&ab, "\x1b[?25h", 6);  // Show the cursor
    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);

    fflush(stdout);  // Ensure all output is flushed to the terminal
}


void editorSetStatusMessage(const char *msg) {
    snprintf(E.statusmsg, sizeof(E.statusmsg), "%s", msg);
    E.statusmsg_time = time(NULL);
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

// Input Handling Functions STDIN_FILENO
int editorReadKey() {
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





void moveCursor(int key) {
    erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];

    switch (key) {
        case ARROW_UP:
            if (E.cy != 0) {
                E.cy--;
            }
            break;
        case ARROW_DOWN:
            if (E.cy < E.numrows - 1) {
                E.cy++;
            }
            break;
        case ARROW_LEFT:
            if (E.cx != 0) {
                E.cx--;
            } else if (E.cy > 0) {
                E.cy--;
                E.cx = E.row[E.cy].size;  // Move to the end of the previous row
            }
            break;
        case ARROW_RIGHT:
            if (row && E.cx < row->size) {
                E.cx++;
            } else if (row && E.cx == row->size) {
                E.cy++;
                E.cx = 0;
            }
            break;
    }

    // Adjust the cursor if it's beyond the end of the current row
    row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
    int rowlen = row ? row->size : 0;
    if (E.cx > rowlen) {
        E.cx = rowlen;
    }
}


void editorProcessKeypress() {
    fflush(stdout);
    fflush(stdin);
    int c = editorReadKey();
    //printf("Key pressed: %d, cx: %d, cy: %d\n", c, E.cx, E.cy);  // Debug output
    switch (c) {
        case CTRL_KEY('q'):
            write(1, "\x1b[2J", 4);  // Clear screen before quitting
            write(1, "\x1b[H", 3);   // Move cursor home
            exit(0);
            break;
            
        case CTRL_KEY('s'):
            editorSave();
            break;
        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            moveCursor(c);
            break;
        
        case CTRL_KEY('l'):
        case '\x1b':
             break;
        
        default:
            editorInsertChar(c);
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

void editorAppendRow(char *s, size_t len) {
    E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
    int at = E.numrows;
    E.row[at].size = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';
    E.row[at].rsize = 0;
    E.row[at].render = NULL;
    editorUpdateRow(&E.row[at]);
    E.numrows++;
}

void editorRowInsertChar(erow *row, int at, int c) {
  if (at < 0 || at > row->size) at = row->size;
  row->chars = realloc(row->chars, row->size + 2);
  memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
  row->size++;
  row->chars[at] = c;
  editorUpdateRow(row);
}

void editorInsertChar(int c) {
    if (E.cy == E.numrows) {
        editorAppendRow("", 0);
    }
    editorRowInsertChar(&E.row[E.cy], E.cx, c);
    E.cx++;
}

void editorUpdateRow(erow *row) {
    int tabs = 0;
    for (int j = 0; j < row->size; j++)
        if (row->chars[j] == '\t') tabs++;
    
    free(row->render);
    row->render = malloc(row->size + tabs*(KILO_TAB_STOP - 1) + 1);
    
    int idx = 0;
    for (int j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') {
            row->render[idx++] = ' ';
            while (idx % KILO_TAB_STOP != 0) row->render[idx++] = ' ';
        } else {
            row->render[idx++] = row->chars[j];
        }
    }
    row->render[idx] = '\0';
    row->rsize = idx;
}

int editorRowCxToRx(erow *row, int cx) {
    int rx = 0;
    for (int j = 0; j < cx; j++) {
        if (row->chars[j] == '\t') {
            rx += (KILO_TAB_STOP - 1) - (rx % KILO_TAB_STOP);
        }
        rx++;
    }
    return rx;
}


char *editorRowsToString(int *buflen) {
  int totlen = 0;
  int j;
  for (j = 0; j < E.numrows; j++)
    totlen += E.row[j].size + 1;
  *buflen = totlen;
  char *buf = malloc(totlen);
  char *p = buf;
  for (j = 0; j < E.numrows; j++) {
    memcpy(p, E.row[j].chars, E.row[j].size);
    p += E.row[j].size;
    *p = '\n';
    p++;
  }
  return buf;
}


void editorOpen(char *filename) {
    free(E.filename);
    E.filename = strdup(filename);
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("fopen");
        exit(1);
    }
    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    while ((linelen = getline(&line, &linecap, fp)) != -1) {
        while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
            linelen--;
        editorAppendRow(line, linelen);
    }
    free(line);
    fclose(fp);
}


void editorSave() {
    if (E.filename == NULL) return;

    int len;
    char *buf = editorRowsToString(&len);

    FILE *fp = fopen(E.filename, "w");
    if (fp == NULL) {
        free(buf);
        return;
    }

    fwrite(buf, 1, len, fp);
    fclose(fp);
    free(buf);
}


void initEditor() {
    E.cx = 0;
    E.cy = 0;
    E.rx = 0;
    E.rowoff = 0;
    E.coloff = 0;
    E.numrows = 0;
    E.row = NULL;
    E.filename = NULL;
    E.statusmsg[0] = '\0';
    E.statusmsg_time = 0;

    if (getWinSize(&E.screencols, &E.screenrows) == -1) {
        write(1, "\x1b[2J", 4);
        write(1, "\x1b[H", 3);
        perror("Init");
        exit(1);
    }
    E.screenrows -= 2;
}

// Main Loop
int main(int argc, char *argv[]) {
    enableRawMode();
    initEditor();

    if (argc > 1) {
        editorOpen(argv[1]);
    }
    editorSetStatusMessage("HELP: Ctrl-Q = quit");
    while (1) {
        refreshScreen();
        editorProcessKeypress();
    }

    disableRawMode();
    return 0;
}
