CFLAGS = -Wall -m64 -ggdb -I. -fPIC -shared

CC = cc
TARGET = libcode64.so
FNAME = libcode64

all : ${TARGET}

${TARGET} : libcode64.c libcode64.h
	$(CC) ${CFLAGS} -o ${TARGET} libcode64.c 

install :
	install -D --mode=755 libcode64.so /usr/lib
	install -D --mode=755 libcode64.h  /usr/local/include

uninstall :
	rm -f /usr/lib/libcode64.so
	rm -f /usr/local/include/libcode64.h

clean :
	rm libcode64.so
