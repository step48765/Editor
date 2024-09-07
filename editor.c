#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <ctype.h>
#include <sys/ioctl.h>

#define CTRL_KEY(k) ((k) & 0x1f)


struct editorConfig{
	int screenrows;
	int screencols;
	struct termios orig_attr;
};

struct editorConfig E;


void drawRows(){
	int y;	
	
	for(y = 0; y < E.screenrows; y++){
		write(1, "~", 1);
    	if (y < E.screenrows - 1) {
    	  write(1, "\r\n", 2);
    	}
	}
}

void refreshScreen(){
	write(1, "\x1b[2J", 4); //clear screen
	write(1, "\x1b[H", 3); // move cursor home (top left)
	
	drawRows(); // draw tildas
	
	write(1, "\x1b[H", 3); // move cursor home
}



void disableRawMode(){
	if(tcsetattr(0, TCSAFLUSH, &E.orig_attr) == -1 ){
		write(STDOUT_FILENO, "\x1b[2J", 4);
      	write(STDOUT_FILENO, "\x1b[H", 3);
      	perror("tcsetattr");		
		exit(1);	
	}
}

void enableRawMode(){
	
	if(tcgetattr(0, & E.orig_attr) == -1){
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
	
	if(tcsetattr(0, TCSAFLUSH, &raw) == -1){
		write(STDOUT_FILENO, "\x1b[2J", 4);
      	write(STDOUT_FILENO, "\x1b[H", 3);
		perror("tcsetattr raw");
		exit(1);
	}
}


char editorReadKey() {
	char c;
	c = '\0';
    if(read(1, &c, 1) == -1){
   		perror("read");
   		exit(1);
   	}
   	printf("Key read: %c (0x%x)\n", c, c);
    return c;
}

void editorProcessKeypress() {
	char c;
	c = editorReadKey();
	
	//printf("%d\n\r", c);
		
	switch(c){
		case CTRL_KEY('q'):
			write(STDOUT_FILENO, "\x1b[2J", 4);
      		write(STDOUT_FILENO, "\x1b[H", 3);
			exit(0);
			break;
	}
}


/*
int getCursorPosition(int *rows, int *cols) {
    char buf[32];
    unsigned int i = 0;
    
    // Send the escape sequence to query cursor position
    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) {
        perror("Failed to write escape sequence");
        return -1;
    }
    
    // Read the response from the terminal
    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) {
            perror("Failed to read response");
            return -1;
        }
        printf("Read: %c (0x%x)\n", buf[i], buf[i]); // Debug each character
        if (buf[i] == 'R') break; // End of the response
        i++;
    }
    
    buf[i] = '\0'; // Null-terminate the response
    printf("Received sequence: %s\n", buf);  // Debug: Print the whole sequence
    
    // Validate the response
    if (buf[0] != '\x1b' || buf[1] != '[') {
        return -1;
    }
    
    // Parse the cursor position from the escape sequence
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) {
        return -1;
    }
    
    return 0;
}

*/
	
	

int getWinSize(int *cols, int *rows){
    struct winsize ws;
    
    if(ioctl(1, TIOCGWINSZ, &ws) != -1){
		*rows = ws.ws_row;
		*cols = ws.ws_col;
		editorReadKey();
		return 0;
	}else{
		return -1;
	}
}

	
void initEditor(){
	if(getWinSize(&E.screencols, &E.screenrows) == -1){
		write(STDOUT_FILENO, "\x1b[2J", 4);
      	write(STDOUT_FILENO, "\x1b[H", 3);
   		perror("Init");
   		exit(1);
	}
}


int
main(){	
	enableRawMode();
	initEditor();
	
	 while (1) {
	 	refreshScreen();
    	editorProcessKeypress();
  	}
	
	disableRawMode();	
	return 0;

}