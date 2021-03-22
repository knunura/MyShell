# My Shell

## Kevin Nunura

## General Description
Simple shell program that does the following:
• Execute a single command with up to four command line arguments (including com-
mand line arguments with associated flags). For example:
– myshell>> ls –l
– myshell>> cat myfile
– myshell>> ls –al /usr/src/linux
• Execute a command in background. For example: – myshell>> ls -l &
– myshell>> ls –al /usr/src/linux & 1
• Redirect the standard output of a command to a file. For example:
– myshell>> ls -l > outfile
– myshell>> ls -l >> outfile
– myshell>> ls –al /usr/src/linux > outfile2 – myshell>> ls –al /usr/src/linux >> outfile2
• Redirect the standard input of a command to come from a file. For example: – myshell>> grep disk < outfile
– myshell>> grep linux < outfile2
• Execute multiple commands connected by a single shell pipe. For example:
– myshell>> ls –al /usr/src/linux | grep linux – myshell>> ls -la | wc -l
• Execute the cd and pwd commands – myshell>> cd some path
– myshell>> pwd

## Build Instructions
1) Clone repository
2) Open up terminal on Linux
3) cd into "MyShell"
4) Create executable file of "shell.c" through "gcc myshell.c -o myshell"

## Run Instructions
1) Enter in command line "./myshell"

