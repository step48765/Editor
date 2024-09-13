
#ifndef EDITOR_H
#define EDITOR_H

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

enum editorKey {
  BACKSPACE = 127,
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

extern struct editorConfig E;


// Function Prototypes
void abAppend(struct abuf *ab, const char *s, int len);
void abFree(struct abuf *ab);
void editorScroll(void);
void drawRows(struct abuf *ab);
void editorDrawStatusBar(struct abuf *ab);
void editorDrawMessageBar(struct abuf *ab);
void refreshScreen(void);
void editorSetStatusMessage(const char *msg);
void disableRawMode(void);
void enableRawMode(void);
int editorReadKey(void);
void moveCursor(int key); 
void editorProcessKeypress(void);
int getWinSize(int *cols, int *rows);
void editorInsertRow(int at, char *s, size_t len);
void editorRowInsertChar(erow *row, int at, int c);
void editorInsertChar(int c);
void editorOpen(char *filename);
void initEditor(void);
void editorUpdateRow(erow *row);
int editorRowCxToRx(erow *row, int cx);
void editorSave(void);
void editorDelChar(void);
void editorInsertNewline(void);
void editorFreeRow(erow *row);
void editorDelRow(int at);
void editorRowAppendString(erow *row, char *s, size_t len);
void editorRowDelChar(erow *row, int at);
char *editorRowsToString(int *buflen);
void die(const char *s);


#endif
