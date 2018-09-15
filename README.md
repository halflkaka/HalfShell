# HalfShell
## Compile
`gcc shell.c -o w4118_sh`  
`./w4118_sh`  
别问makefile, 问了就是懒得写了
## Function
1. The shell should run continuously, and display a prompt when waiting for input. e.g `$/bin/ls -lha /home/w4118/my_docs`  
2. The shell should read a line from stdin one at a time  
3. After parsing and lexing the command, your shell should execute it. A command can either be a reference to an executable OR a built-in shell command.  
## Examples
1. `cd [dir]`  
2. `history [-c] [number]`  
3. history (without arguments) displays the last 100 commands the user ran, with an offset next to each command  
4. history -c clears the entire history, removing all entries.  
5. `!!` executes the most recent command in history.  
6. `!string` executes the most recent command in history starting with string.  
`$cd /home/w4118  
$/bin/ls  
my_file.txt  
$history  
0 cd /home/w4118  
1 /bin/ls  
2 history  
$!/  
my_file.txt  
$!his  
0 cd /home/w4118  
1 /bin/ls  
2 history  
3 /bin/ls  
4 history  
$history -c  
$history  
6 history  
$`  
## 说明
OS的HW1，最后还是选了分布式，还剩下pipe功能没写，下学期选了的话再写吧。。。。。。
