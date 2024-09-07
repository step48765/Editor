#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <ctype.h>

#define CTRL_KEY(k) ((k) & 0x1f)

struct termios org_attr;

void disableRawMode(){
	if(tcsetattr(0, TCSAFLUSH, &org_attr) == -1 ){
		perror("tcsetattr");
		exit(1);	
	}
}

void enableRawMode(){
	
	if(tcgetattr(0, & org_attr) == -1){
		perror("tcgetattr");
		exit(1);
	}
	
	struct termios raw = org_attr;
	
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	
  	raw.c_oflag &= ~(OPOST);
  	
  	raw.c_cflag |= (CS8);
  	
  	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  	
  	raw.c_cc[VMIN] = 0;
  	
  	raw.c_cc[VTIME] = 1;
	
	if(tcsetattr(0, TCSAFLUSH, &raw) == -1){
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
    return c;
}

void editorProcessKeypress() {
	char c;
	c = editorReadKey();
		
	switch(c){
		case CTRL_KEY('q'):
			exit(0);
			break;
	}
}

int
main(){	
	enableRawMode();
	
	 while (1) {
    	editorProcessKeypress();
  	}
	
	disableRawMode();	
	return 0;

}