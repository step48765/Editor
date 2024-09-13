
# Terminal-Based Text Editor

This is a simple terminal-based text editor written in C. It allows you to open a file, edit its content, and save changes. The editor supports basic text manipulation and navigation commands, all controlled from the terminal.

## How to Use

### 1. Compiling the Editor
To compile the editor, simply use the `make` command in the project directory:

```
make
```

This will compile all the source files and create an executable named `editor`.

### 2. Running the Editor
To open a file in the editor, run the following command:

```
./editor <filename>
```

- If the `<filename>` exists, the editor will open it for editing.
- If the file does not exist, the editor will create a new file with the given name.

### 3. Editor Controls
- **CTRL + S**: Save the current file.
- **CTRL + Q**: Quit the editor.
- **Arrow Keys**: Navigate the file (move the cursor).
- **Backspace**: Delete the character to the left of the cursor.

### 4. Saving and Exiting
- Press `CTRL + S` to save the file.
- Press `CTRL + Q` to quit the editor.

## Project Structure

The project consists of the following files:
- **editor.c**: Contains the main logic and initialization.
- **input.c**: Handles user input (such as keypresses).
- **row.c**: Handles row manipulation and text rendering.
- **screen.c**: Handles drawing to the terminal screen.
- **buffer.c**: Manages the dynamic append buffer for output.
- **utils.c**: Contains utility functions such as error handling.
- **terminal.c**: Manages terminal modes and settings.
- **editor.h**: Header file containing function declarations and structure definitions.

### Compilation Details
- The project is compiled with `gcc` using `-Wall`, `-Wextra`, and `-std=c99` flags to ensure warnings and strict adherence to the C99 standard.
- A `Makefile` is provided to automate the build process.

### Example Usage
To edit a file called `example.txt`:

```
./editor example.txt
```

Edit the content, and when finished, press `CTRL + S` to save and `CTRL + Q` to quit.

## Notes
- The editor is basic and intended as a learning project for terminal-based text editing. It does not support advanced features like syntax highlighting or search functionality.
