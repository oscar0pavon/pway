# pway
# See LICENSE file for copyright and license details.

#no .POSIX: the $(wildcard) below is a GNU extension already, and the
#dependency tracking wants a pattern rule rather than a suffix one

CC = cc

FLAGS = -g

#a .d per object, fed back by -include below. without it a change to pway.h
#rebuilt nothing and sword linked objects built against the old struct layout
FLAGS += -MMD -MP

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
DEP = $(OBJ:.o=.d)

#what pway.h and keyboard.h pull in, and nothing else
HEADERS = pway.h mouse.h keyboard.h copy_paste.h

all: libpway.a

%.o: %.c
	$(CC) $(FLAGS) -c $< -o $@

#ar r only replaces members, so a deleted source would leave its object in the
#archive for good
libpway.a: $(OBJ)
	rm -f $@
	ar rcs $@ $(OBJ)

clean:
	rm -f libpway.a $(OBJ) $(DEP)

install: all
	install -d /usr/local/lib /usr/local/include/pway
	install -m 644 libpway.a /usr/local/lib
	install -m 644 $(HEADERS) /usr/local/include/pway

-include $(DEP)

.PHONY: all clean install
