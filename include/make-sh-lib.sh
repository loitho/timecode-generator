#!/usr/bin/env sh
gcc -c -Wall -fpic -O3 *.c
gcc -shared -o libpbplots.so *.o
