#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#define VERSION "0.0.1"
#define CTRL_KEY(k) ((k) & 0x1f)
#define ABUF_INIT {NULL, 0}  // Macro to initialize dynamic append buffer

enum editorKey{
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN
};

typedef struct erow{
    int size;
    char *chars;
} erow;

// Structures
struct abuf {
    char *b;
    int len;
};

struct editorConfig {
	int cx;
	int cy;
	int rowoff; // this is how far off from the top we are
    int screenrows; // this is the max rows i.e. how far till the bottom we have
    int screencols;
    int numrows;
    erow *row;
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

void editorScroll() {
  if (E.cy < E.rowoff) {
    E.rowoff = E.cy;
  }
  if (E.cy >= E.rowoff + E.screenrows) {
    E.rowoff = E.cy - E.screenrows + 1;
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
      int len = E.row[filerow].size;
      if (len > E.screencols) len = E.screencols;
      abAppend(ab, E.row[filerow].chars, len);
    }
    abAppend(ab, "\x1b[K", 3);
    if (y < E.screenrows - 1) {
      abAppend(ab, "\r\n", 2);
    }
  }
}
//"Welcome Michael -- version %s", VERSION
void refreshScreen() {
    editorScroll();
	char buff[32];
    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6);  // Hide cursor
    abAppend(&ab, "\x1b[H", 3);     // Move cursor to top left
    drawRows(&ab);                  // Draw rows with tildes and welcome message
    
    snprintf(buff, sizeof(buff), "\x1b[%d;%dH", E.cy + 1, E.cx + 1); //move the cursor to position (y,x)
    abAppend(&ab, buff, strlen(buff));
        
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
int editorReadKey() {
    char c;
    char seq[3];
    while(read(0, &c, 1) == -1) { //use while so that it keeps trying to read from stdin till it gets an == 1
        perror("read");
        exit(1);
    }
    if( c == '\x1b'){
        if(read(0, &seq[0], 1) != 1) return '\x1b';
        if(read(0, &seq[1], 1) != 1) return '\x1b';
        
        if(seq[0] == '['){
            switch(seq[1]){
                case 'A': return ARROW_UP;
                case 'B': return ARROW_DOWN;
                case 'C': return ARROW_RIGHT;
                case 'D': return ARROW_LEFT;
                default: return '\x1b';
            }
        }
        else{
            return '\x1b';   
        }
    }else{
        return c;
    }
}

void moveCursor(int key){
    switch(key){
        case ARROW_UP:
            E.cy--;
            break;
        case ARROW_DOWN:
            if (E.cy < E.numrows) {
                E.cy++;
            }
            break;
        case ARROW_RIGHT:
            E.cx++;
            break;
        case ARROW_LEFT:
            E.cx--;
            break;
    }
}


void editorProcessKeypress() {
    int c = editorReadKey();
    switch (c) {
        case CTRL_KEY('q'):
            write(1, "\x1b[2J", 4);  // Clear screen before quitting
            write(1, "\x1b[H", 3);   // Move cursor home
            exit(0);
            break;
        case ARROW_UP:
        case ARROW_DOWN:  
        case ARROW_LEFT:
        case ARROW_RIGHT:
            moveCursor(c);
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
  E.numrows++;
}


void editorOpen(char *filename) {
  FILE *fp = fopen(filename, "r");
  if (!fp){
      perror("fopen");
      exit(1);
  }
  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  while ((linelen = getline(&line, &linecap, fp)) != -1) {
    while (linelen > 0 && (line[linelen - 1] == '\n' ||
                           line[linelen - 1] == '\r'))
      linelen--;
    editorAppendRow(line, linelen);
  }
  free(line);
  fclose(fp);
}



void initEditor() {
	E.cx = 10;
	E.cy = 0;
	E.rowoff = 0;
	E.numrows = 0;
	E.row = NULL;
	
    if (getWinSize(&E.screencols, &E.screenrows) == -1) {
        write(1, "\x1b[2J", 4);
        write(1, "\x1b[H", 3);
        perror("Init");
        exit(1);
    }
}

// Main Loop
int main(int argc, char *argv[]) {
    enableRawMode();
    initEditor();
    
    if(argc > 1){
        editorOpen(argv[1]);
    }

    while (1) {
        refreshScreen();
        editorProcessKeypress();
    }

    disableRawMode();
    return 0;
}
