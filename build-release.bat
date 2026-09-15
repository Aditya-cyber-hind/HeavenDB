@echo off
gcc -Wall -Wextra -Wno-switch -O2 -std=c99 -o heavendb.exe src\*.c -lws2_32 -lbcrypt
