#!/usr/bin/env sh
/usr/bin/gcc -fdiagnostics-color=always -O3 \
        -Wall -Wextra -L./include \
        -lpbplots -Wl,-rpath=./include \
        -I./include ./mytime.c \
        -o ./mytime -lpbplots -lm -lpbplots -lwiringPi

# sh build.sh && sudo nice  --adjustment=-20 ./mytime && python -m http.server 8080