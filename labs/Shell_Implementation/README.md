# Simple Shell (Multi-Processing)

## 1. Objectives

- Gain familiarity with system calls in the Unix environment.
- Introduction to processes and multi-processing.
- Learn how to handle signals in Unix-based systems.

## 2. Problem Statement

### Overview:
The task is to implement a custom Unix shell that can execute commands in both the foreground and background. The shell should support several key features, including internal commands, background processes, and proper signal handling.

### Required Features:
- **Internal Commands:**
  - Implement the `exit` command to terminate the shell.
  
- **Command Execution:**
  - Execute commands with no arguments (e.g., `ls`, `cp`, `rm`).
  - Handle commands with arguments (e.g., `ls -l`).
  - Support background processes, initiated by appending `&` (e.g., `firefox &`), where the shell should return immediately without waiting for the process to finish.

- **Shell Builtins:**
  - Implement built-in commands:
    - `cd` (change directory).
    - `echo` (print text to the terminal).
    - `export` (set environment variables).
  
- **Expression Evaluation:**
  - Implement `export` to set values to variables and print their values.
  - Handle commands like `export x="value"` and `echo "$x"`.

### System Behavior:
- The shell should handle commands by forking a child process, executing the command using `execvp()`, and waiting for it to complete using `waitpid()`.
- When a command is run in the background, the shell should not wait for the child process to finish and should immediately return the prompt.
- The shell should maintain a log file where it appends a message whenever a child process terminates. This log file will be updated upon receiving the `SIGCHLD` signal.

---

## 3. Problem Description

The shell program should take the user’s input (command and its parameters), fork a child process, and execute the command in that child process. The parent process waits for the child to finish unless the command is being executed in the background.

### Key Components:
1. **Command Parsing:**
   - Read and parse the user input into command arguments.
   
2. **Forking and Executing Commands:**
   - Use `fork()` to create a child process.
   - The child process uses `execvp()` to execute the command.

3. **Waiting for Completion:**
   - The parent process uses `waitpid()` to wait for the child process to complete in the foreground.

4. **Background Processes:**
   - If a command is followed by `&`, the shell should immediately return control to the user without waiting for the child process.

5. **Signal Handling:**
   - Implement a signal handler to handle the `SIGCHLD` signal, which indicates that a child process has terminated. When this signal is received, the shell logs "Child process was terminated" in a log file.

### Additional Requirements:
- Prevent zombie processes by handling the `SIGCHLD` signal properly.
- The shell should provide a log file with entries for terminated child processes.

---

## 4. Deliverables

- Complete and well-commented C source code.
- A log file showing entries for terminated child processes.

---

## 5. Test Cases

1. **Basic Command Execution:**
   - Execute `ls`, `mkdir test`, `ls` again, `ls -a -l -h`, and `ls $x` after setting `export x="-a -l -h"`.
   
2. **Foreground Process Test:**
   - Run `firefox`, show that the shell is stuck and can't execute other commands until the process terminates.
   
3. **Background Process Test:**
   - Run `firefox &`, demonstrate that the shell can execute other commands while Firefox runs in the background.
   - View the log file to see entries for terminated child processes.

4. **System Monitor:**
   - Open the system monitor and verify that the shell and child processes (like Firefox) appear as background processes.

5. **Error Handling:**
   - Execute an invalid command (e.g., `heyy`) and verify that the shell returns an error message.
   
6. **Exit Command:**
   - Run the `exit` command and verify that the shell terminates correctly.

---

## 6. Requirements

- Implement using **C programming language**.
- Use **Ubuntu** as the development environment.
---

## 7. References and Resources

- [Zombie Processes in Linux](#)
- [Understanding Linux Signals](#)
- [Linux `waitpid()` and Process States](#)
- [sigaction(2) Manual](https://man7.org/linux/man-pages/man2/sigaction.2.html)
- [signal(2) Manual](https://man7.org/linux/man-pages/man2/signal.2.html)

---
