#!/usr/bin/env sh
g++ -c -Wall -fpic -O3 *.c
g++ -shared -o libpbplots.so *.o
