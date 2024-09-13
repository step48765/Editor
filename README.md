
# NBS_Editor (No BullShit Editor)

NBS_Editor is a simple terminal-based text editor with no bells and whistles. It's designed to give you a straightforward text-editing experience without the extra features that modern editors often include. Just open a file, edit it, save, and quit—nothing more, nothing less.

## How to Use

### 1. Compiling the Editor
To compile the editor, simply use the `make` command in the project directory:

```
make
```

This will compile all the source files and create an executable named `NBS_Editor`.

### 2. Running the Editor
To open a file in NBS_Editor, run the following command:

```
./NBS_Editor <filename>
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
./NBS_Editor example.txt
```

Edit the content, and when finished, press `CTRL + S` to save and `CTRL + Q` to quit.

## Notes
- NBS_Editor is designed to be a no-nonsense, simple text editor. It doesn't include extra features like syntax highlighting or search functionality—just the basics.
