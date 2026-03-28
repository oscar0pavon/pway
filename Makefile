#include "mouse.h"
# pterminal
# See LICENSE file for copyright and license details.
.POSIX:


CC = cc

FLAGS = -g


SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

all: libpway.a

.c.o:
	$(CC) $(FLAGS) -c $<

libpway.a: $(OBJ)
	ar rcs libpway.a $(OBJ)

clean:
	rm -f libpway.a $(OBJ) 

install: all
	cp -f libpway.a /usr/local/lib
	mkdir -p /usr/local/include/pway
	cp -f pway.h /usr/local/include/pway
	cp -f mouse.h /usr/local/include/pway
	cp -f copy_paste.h /usr/local/include/pway


.PHONY: all clean install
