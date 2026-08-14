# Operating-Systems-Commands - 2025 (Pushed from VS Code)
# Custom Unix Shell

## Overview

This project implements a custom Unix shell in C. The goal was to create a functional command-line shell that supports common Unix commands along with additional features such as command history, input/output redirection, background processes, pipelines, and interactive command navigation.

The project focuses heavily on Unix system calls and process management, including `fork()`, `execvp()`, `waitpid()`, `pipe()`, `dup2()`, `open()`, `chdir()`, and `read()`.

---

## Features

The shell supports:

* Executing Unix commands
* Built-in `cd` command
* `exit` command
* Command history
* `!!` to repeat the previous command
* History navigation using arrow keys
* Input redirection using `<`
* Output redirection using `>`
* Pipelines using `|`
* Background execution using `&`
* Interactive character-by-character input
* Dynamic memory allocation
* Restoring terminal settings when the shell exits

---

## Libraries Used

The project uses several standard and Unix-specific libraries:

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
```

These libraries provide functionality for:

| Library    | Purpose                                                       |
| ---------- | ------------------------------------------------------------- |
| `stdio.h`  | Input/output operations                                       |
| `stdlib.h` | Memory management and general utilities                       |
| `string.h` | String manipulation                                           |
| `unistd.h` | Unix system calls such as `fork()`, `execvp()`, and `chdir()` |
| `fcntl.h`  | File control and file-opening flags                           |

The `fcntl.h` library was particularly important for file redirection because functions such as:

```c
open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
```

require the file-control definitions provided by this header.

---

## Constants and Memory Management

The shell defines constants for managing command history and command input.

* `hsize` — maximum number of commands stored in history
* `maxlines` — maximum number of input lines
* `maxargs` — maximum number of arguments allowed for a command

These limits help prevent excessive memory usage while keeping the shell manageable.

Dynamic memory is also used when storing and processing commands. Functions such as `malloc()` and `strdup()` are used to safely allocate memory for command strings and arguments.

---

## Command History

Command history is implemented using an array:

```c
history[hsize]
```

A counter keeps track of the number of commands currently stored, while an index determines where the next command should be inserted.

The history operates as a **circular buffer**. Once the buffer becomes full, new commands overwrite the oldest commands.

### `cmdhistory()`

The `cmdhistory()` function adds commands to the history buffer. When the buffer reaches its maximum size, it wraps around and replaces older entries.

### `gethistory()`

The `gethistory()` function retrieves previous commands based on their position relative to the most recent command.

This functionality supports commands such as:

```text
!!
```

which recalls the most recently executed command.

History navigation is also supported through arrow keys. A variable called `navigation` keeps track of the current location within the history buffer and is initialized to `-1`.

---

## Shell Prompt

The `printing()` function displays the shell prompt.

It uses:

```c
getcwd()
```

to retrieve the current working directory.

Instead of displaying the entire path, the shell extracts the last component of the directory and displays it in the prompt.

For example:

```text
osc:project1>
```

If `getcwd()` fails, the shell uses a default prompt:

```text
osc>
```

`fflush(stdout)` is also used throughout the program to ensure output is immediately displayed in the terminal.

---

## Command Parsing

The `characters()` function is responsible for parsing user input.

It separates a command line into individual arguments and recognizes special shell operators:

```text
<
>
|
&
```

Spaces and tabs are treated as separators.

The special characters are stored separately so that later functions can determine whether the user requested:

* Input redirection
* Output redirection
* A pipeline
* Background execution

Memory for parsed commands is dynamically allocated using `malloc()` and `strdup()`.

Whitespace is skipped to prevent empty arguments from being created.

---

## Command Execution

The `commands()` function handles the execution of commands entered by the user.

The general process is:

1. Check whether there is input to process.
2. Count the command arguments.
3. Check for background execution using `&`.
4. Handle built-in commands such as `cd`.
5. Process input and output redirection.
6. Create a child process using `fork()`.
7. Use `execvp()` to execute external commands.
8. Have the parent process wait for the child using `waitpid()`.

### Changing Directories

The `cd` command is handled directly by the shell.

If no directory is provided, an error is displayed.

Otherwise, the shell attempts to change directories using:

```c
chdir()
```

This must be handled by the shell process itself because changing directories inside a child process would not change the working directory of the shell.

---

## Input and Output Redirection

The shell supports input and output redirection.

### Input Redirection

The `<` operator allows a command to receive input from a file.

For example:

```bash
sort < input.txt
```

### Output Redirection

The `>` operator sends command output to a file.

For example:

```bash
ls > output.txt
```

Files are opened using:

```c
open()
```

and the standard input/output file descriptors are replaced using:

```c
dup2()
```

The project required debugging and testing of `dup2()` because file descriptor management was one of the more challenging parts of implementing the shell.

---

## Process Creation

External commands are executed using Unix processes.

The shell creates a child process using:

```c
fork()
```

The child then executes the requested program using:

```c
execvp()
```

The parent process waits for the child to finish with:

```c
waitpid()
```

This allows the shell to execute standard Unix commands while maintaining control over the shell process itself.

---

## Background Processes

The shell also supports background execution using `&`.

For example:

```bash
sleep 10 &
```

When a command ends with `&`, the shell recognizes it as a background process and removes the `&` from the command arguments before execution.

This allows the shell to continue accepting commands without waiting for the background process to finish.

---

## Pipelines

Pipelines are implemented through the `pipes()` function.

A pipeline allows the output of one command to become the input of another command.

For example:

```bash
ls | grep txt
```

The command line is split at:

```text
|
```

Both commands are then tokenized using the `characters()` function.

### Creating the Pipe

The shell creates a pipe using:

```c
pipe()
```

This produces two file descriptors:

* One for reading
* One for writing

Two child processes are then created.

### First Child

The first child redirects its standard output to the pipe and executes the left-side command.

### Second Child

The second child redirects its standard input from the pipe and executes the right-side command.

### Parent Process

The parent closes both ends of the pipe and waits for both children using:

```c
waitpid()
```

This creates the connection between the two commands.

---

## Interactive Input

The `read_line()` function handles interactive input.

Instead of using a standard line-based input function, the implementation uses:

```c
read()
```

to read one character at a time.

This provides immediate feedback when the user types.

For example, when a printable character is entered:

1. The character is added to the input buffer.
2. The character is immediately displayed.
3. The shell continues reading input.

This approach was also useful for implementing arrow-key history navigation.

When the user navigates through command history, the current line can be cleared, a previous command can be copied into the buffer, and the command can be displayed again for editing.

---

## Main Function

The `main()` function initializes and controls the shell.

The general execution loop is:

```text
Initialize history
       ↓
Display shell prompt
       ↓
Read user input
       ↓
Check special commands
       ↓
Parse command
       ↓
Execute command
       ↓
Store command in history
       ↓
Repeat
```

Special commands are handled directly by `main()`.

### `exit`

Terminates the shell.

### `!!`

Retrieves and executes the most recently entered command.

### `|`

Routes the command to the pipeline implementation.

Other commands are copied into a buffer, tokenized using `characters()`, and executed using `commands()`.

After execution, the command is stored in the history buffer and also saved as the last executed command.

---

## Memory Cleanup

When the shell exits, dynamically allocated memory is freed.

This includes:

* Command history
* The last executed command
* Dynamically allocated command arguments

This prevents memory leaks and follows the memory-management practices used in the course labs.

---

## Terminal Settings

Because the shell reads input character-by-character, it modifies the terminal settings during execution.

Before exiting, the original terminal settings are restored using:

```c
tcsetattr()
```

Restoring the terminal settings is important because otherwise the user's terminal could remain in an altered state after the shell terminates.

---

## System Calls Used

Some of the major Unix system calls and functions used in this project include:

| Function      | Purpose                                           |
| ------------- | ------------------------------------------------- |
| `fork()`      | Creates a child process                           |
| `execvp()`    | Executes an external command                      |
| `waitpid()`   | Waits for a child process                         |
| `pipe()`      | Creates a communication channel between processes |
| `dup2()`      | Redirects file descriptors                        |
| `open()`      | Opens files for redirection                       |
| `chdir()`     | Changes the shell's working directory             |
| `getcwd()`    | Gets the current working directory                |
| `read()`      | Reads user input character-by-character           |
| `tcsetattr()` | Restores terminal settings                        |

---

## Challenges and Debugging

One of the biggest challenges in this project was working with Unix file descriptors and process management.

In particular, understanding how `dup2()` interacts with standard input and output required additional debugging and testing.

Another challenge was implementing interactive input and arrow-key history navigation because the shell needed to process individual characters instead of waiting for an entire line.

Command history also required careful index management because the history buffer operates as a circular array.

During development, `fflush(stdout)` was added in several places to ensure terminal output appeared immediately during interactive use.

---

## Example Commands

Once running, the shell can be used similarly to a basic Unix shell.

### Basic command

```bash
ls
```

### Change directory

```bash
cd project1
```

### Input redirection

```bash
cat < input.txt
```

### Output redirection

```bash
ls > output.txt
```

### Pipeline

```bash
ls | grep txt
```

### Background process

```bash
sleep 10 &
```

### Repeat previous command

```bash
!!
```

### Exit

```bash
exit
```

---

## What I Learned

This project provided hands-on experience with Unix process management and system programming in C.

The main concepts I worked with were:

* Process creation with `fork()`
* Program execution with `execvp()`
* Process synchronization with `waitpid()`
* File descriptor management
* Input/output redirection
* Pipes and inter-process communication
* Dynamic memory allocation
* Command parsing
* Circular buffers
* Interactive terminal input
* Terminal configuration and restoration

Overall, the project helped me understand how a basic shell works internally rather than treating the terminal as a black box.

```
```
