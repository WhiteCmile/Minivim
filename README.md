## How to use minivim

By simply type minivim [options] <filename>, you can then open a file with minivim. You can view text file in `Normal Mode`, insert characters in `Insert Mode` and input command in `Command Mode`. 

In minivim, there are three options:
- `-t`: open a file in truncation mode.
- `-w`: open a file, you can either write or read.
- `-R`: open a file in read-only mode.

## Base Terminal User Interface Introduction

There are three windows on the screen: File window, Information window and Command window.

The first window is File window. It shows you the content of the file you want to edit.

The second window is Information window. It shows you information of the file like filename, the mode you are presently in and position of the cursor.

The third window is Command window. It display the command you input and warning if you had input invalid commands.

## Modes Introduction

Minivim supports three mode.

The first one is `Normal Mode`. In this mode, you can only browse content of the file, while you can't modify any information in it. You can use arrow keys, 'home', 'end' to browse the content in it. We also provide some shortcut keys to help you.
- `dd`: delete the entire line that the cursor is currently on.
- `O`: Move the cursor to the beginning of the line.
- `$`: Move the cursor to the end of the line.
- `w`: Move forward one word (delimited by a white space or a new line).
- `b`: Move backward one word (delimited by a white space or a new line).

The second one is `Insert Mode`. In this mode, you can edit file by moving cursor. Edit file include append, modify and deletion.

The third one is `Command Mode`. In command mode, we can execute some command.
- `:-w`: Save the file.
- `:-q`: Quit. Warn user if the file is changed and is unsaved.
- `:q!`: Force quit(i.e. Do not save the file and force quit.). If the file does not exist and was created by MiniVim, then this operation will also undo the creation.
- `:wq`: Save then quit.
- `:jmp LineNumber`: Because Minivim supports displaying line number at the beginning of a line, you can use this command to jump to a specific line. The specific line is displayed on the top.
- `:sub "A" "B"`: Minivim suppoprts searching for "A" in the full file and substitute it to "B". But because of the trivial implementation, you may find "allowed" being "alloyoud" if you input ":sub "we" "you"". So carefully use this command.
Also, Minivim supports browse command previously entered in Command Mode by press arrow keys "UP" and "DOWN".

# Being happy to use Minivim!!!!!!
