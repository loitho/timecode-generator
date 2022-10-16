#!/usr/bin/env sh
/usr/bin/gcc -fdiagnostics-color=always -O3 \
        -Wall -Wextra -L./include \
        -lpbplots -Wl,-rpath=./include \
        -I./include ./mytime.c \
        -o ./mytime -lpbplots -lm
