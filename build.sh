#!/usr/bin/env sh
/usr/bin/gcc -pthread -fdiagnostics-color=always -O3 \
        -Wall -Wextra -L./include \
        -lpbplots -Wl,-rpath=./include \
        -I./include ./*.c \
        -o ./mytime -lpbplots -lm -lpbplots -lrt

# sh build.sh && sudo nice  --adjustment=-20 ./mytime && python -m http.server 8080