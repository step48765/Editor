#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <ctype.h>
#include <sys/ioctl.h>

#define CTRL_KEY(k) ((k) & 0x1f)


struct editorConfig{
	int cols;
	int rows;
	struct termios orig_attr;
};

struct editorConfig E;

int getWinSize(int *cols, int *rows){
	struct winsize ws;
	
	if(ioctl(1, TIOCGWINSZ, &ws) != -1){
		*rows = ws.ws_row;
		*cols = ws.ws_col;
		return 0;
	}else{
		return -1;
	}
}

void drawRows(){
	int y;	
	getWinSize(&E.cols, &E.rows);
	
	for(y = 0; y < E.rows; y++){
		write(1, "~\r\n", 3);
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
		refreshScreen();
		perror("tcsetattr");		
		exit(1);	
	}
}

void enableRawMode(){
	
	if(tcgetattr(0, & E.orig_attr) == -1){
		refreshScreen();
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
		refreshScreen();
		perror("tcsetattr raw");
		exit(1);
	}
}


char editorReadKey() {
	char c;
	c = '\0';
    if(read(1, &c, 1) == -1){
   		refreshScreen();
   		perror("read");
   		exit(1);
   	}
    return c;
}

void editorProcessKeypress() {
	char c;
	c = editorReadKey();
	
	//printf("%d\n\r", c);
		
	switch(c){
		case CTRL_KEY('q'):
			refreshScreen();
			exit(0);
			break;
	}
}


int
main(){	
	enableRawMode();
	
	 while (1) {
	 	refreshScreen();
    	editorProcessKeypress();
  	}
	
	disableRawMode();	
	return 0;

}