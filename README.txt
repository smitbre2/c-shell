# Readme #
This is a simple shell built with c. Some features such as cd, exit and status are built-in. Other shell commands can be utilized by using exec functions.
Processes can be ran in the background by supplying an & after the command, such as: sleep 50 &

Certain signals are handled by the shell so Ctrl-Z will activate a foreground only mode which will prevent the use of background tasks, and Ctrl-C will only apply to child processes and not the parent shell.


# Compilation #
The shell can be compiled with gcc via:

		gcc -o smallsh smallsh.c -lpthread

This will create a smallsh executable.
