# devlogs

This project is a custom implementation of a shell written in C. It aims to achieve all the features of a POSIX-compliant shell. It is part of the TILDE 3.0 mentorship program where 5 students worked on this project under the mentorship of 3 mentors.

## week-1

We have implemented the basic structure of a shell and execution of some built-in and other external commands.

## Current Workflow:
1. **Read Input**: Read an input from the user.
2. **Tokenize**: Tokenize the input with " " as a delimiter.
3. **Command Execution**:
   - Check whether the first token is a built-in or external command.
   - Call the corresponding function and return the result.
4. **Loop**: Go back to reading an input infinitely until `exit` is called.

## Future Work:
- Create functions for all built-in commands.
- Extensively debug all functions with all possible arguments and flags.
- Handle signals and arrow keys.
- Add tab completion.

## week-2

[slides](week-2.pdf)

## week-3

[slides](week-3.pdf)

## week-4

[slides](week-4.pdf)

## week-5 (final review)

[slides](week-5.pdf)
