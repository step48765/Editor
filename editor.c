
#include "editor.h"
struct editorConfig E;


// Main Logic
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

void initEditor(void) {
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
        editorInsertRow(E.numrows, line, linelen);
    }
    free(line);
    fclose(fp);
}

void editorSave(void) {
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

void editorSetStatusMessage(const char *msg) {
    snprintf(E.statusmsg, sizeof(E.statusmsg), "%s", msg);
    E.statusmsg_time = time(NULL);
}

void editorProcessKeypress(void) {
    fflush(stdout);
    fflush(stdin);
    int c = editorReadKey();
    switch (c) {
        case '\r':
            editorInsertNewline();
            break;
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
        case CTRL_KEY('s'):
            editorSave();
            break;
        case HOME_KEY:
            E.cx = 0;
            break;
        case END_KEY:
            if (E.cy < E.numrows)
                E.cx = E.row[E.cy].size;
            break;
        case BACKSPACE:
        case CTRL_KEY('h'):
        case DEL_KEY:
            if (c == DEL_KEY) moveCursor(ARROW_RIGHT);
            editorDelChar();
            break;
        case PAGE_UP:
        case PAGE_DOWN: {
            if (c == PAGE_UP) {
                E.cy = E.rowoff;
            } else if (c == PAGE_DOWN) {
                E.cy = E.rowoff + E.screenrows - 1;
                if (E.cy > E.numrows) E.cy = E.numrows;
            }
            int times = E.screenrows;
            while (times--)
                moveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
            break;
        }
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

void refreshScreen(void) {
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

void editorScroll(void) {
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
