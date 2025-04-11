# swr - self-writer
# See LICENSE for copyright.
.POSIX:

include config.mk

SRC = swr.c
OBJ = $(SRC:.c=.o)

all: swr

.c.o:
	${CC} ${CFLAGS} -c $<

swr.o: swr.c
${OBJ}: config.mk

swr: clean ${OBJ}
	${CC} ${SRC} -o $@ ${CFLAGS}

clean:
	rm -f swr ${OBJ}

.PHONY: all clean
