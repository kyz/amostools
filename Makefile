CC=gcc
CFLAGS=-O2 -Wall -Wextra #-g -fsanitize=address

all: dumpamos listamos unlockamos

dumpamos: dumpamos.c fileio.c
	$(CC) $(CFLAGS) -o $@ $<

listamos: listamos.c fileio.c amoslib.h amoslib.c \
 extensions/00_base.h extensions/01_music.h extensions/02_compact.h \
 extensions/03_request.h extensions/06_ioports.h
	$(CC) $(CFLAGS) -o $@ $< -lm

unlockamos: unlockamos.c fileio.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f dumpamos listamos unlockamos
