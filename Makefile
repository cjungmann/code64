BASEFLAGS = -Wall -Werror -m64
LIB_CFLAGS = ${BASEFLAGS} -I. -fPIC -shared

LOCAL_LINK = -Wl,-R -Wl,. -lcode64

CFLAGS = -Wall -Werror -m64 -ggdb -I. -fPIC -shared

CC = cc

define install_man_pages
	cp code64.1  /usr/share/man/man1
	gzip -f      /usr/share/man/man1/code64.1
	cp code64.3  /usr/share/man/man3
	gzip -f      /usr/share/man/man3/code64.3
endef

debug : BASEFLAGS += -ggdb -DDEBUG

.PHONY: all
all : libcode64.so code64

libcode64.so : libcode64.c libcode64.h
	$(CC) ${LIB_CFLAGS} -o libcode64.so libcode64.c

code64 : code64.c libcode64.h
	$(CC) ${BASEFLAGS} -L. -o code64 code64.c ${LOCAL_LINK}

debug: libcode64.c libcode64.h code64.c codetest.c
	$(CC) ${LIB_CFLAGS} -o libcode64d.so libcode64.c
	$(CC) ${BASEFLAGS} -L. -o code64d code64.c $(LOCAL_LINK)d
	$(CC) ${BASEFLAGS} -L. -o codetest codetest.c $(LOCAL_LINK)d

install :
	install -D --mode=755 libcode64.so /usr/lib
	install -D --mode=755 libcode64.h  /usr/local/include
	install -D --mode=755 code64       /usr/local/bin
	$(call install_man_pages)

uninstall :
	rm -f /usr/lib/libcode64.so
	rm -f /usr/local/include/libcode64.h
	rm -f /usr/local/bin/code64

clean :
	rm -f libcode64.so libcode64d.so
	rm -f code64 code64d
	rm -f codetest
