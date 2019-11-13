CFLAGS = -Wall -m64 -ggdb -I. -fPIC -shared

CC = cc
TARGET = libcode64.so
FNAME = libcode64

all : ${TARGET}

${TARGET} : code64.c code64.h
	$(CC) ${CFLAGS} -o ${TARGET} code64.c 

install :
	install -D --mode=755 libcode64.so /usr/lib
	install -D --mode=755 code64.h     /usr/local/include

uninstall :
	rm -f /usr/lib/libcode64.so
	rm -f /usr/local/include/i.h
