#!/usr/bin/env sh
/usr/bin/gcc -fdiagnostics-color=always -g \
	-Wall -Wextra -lm -L/home/admin/git/timecode-generator/include \
	-lpbplots -Wl,-rpath=/home/admin/git/timecode-generator/include \
	-I/home/admin/git/timecode-generator/include /home/admin/git/timecode-generator/mytime.c \
	-o /home/admin/git/timecode-generator/mytime
