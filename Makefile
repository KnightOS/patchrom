CFLAGS=-Wall -Wextra -pedantic -std=c99 -O2 -g

all: bin/patchrom bin/patchrom.1

bin/patchrom:main.o
	mkdir -p bin/
	$(CC) $(CFLAGS) $^ -o $@

bin/patchrom.1:patchrom.1.scdoc
	scdoc < patchrom.1.scdoc > bin/patchrom.1

DESTDIR=/usr/local
BINDIR=$(DESTDIR)/bin/
MANDIR=$(DESTDIR)/share/man/

install: bin/patchrom
	mkdir -p $(BINDIR)
	cp bin/patchrom $(BINDIR)

install_man: bin/patchrom.1
	mkdir -p $(MANDIR)/man1/
	cp bin/patchrom.1 $(MANDIR)/man1/

uninstall:
	$(RM) $(BINDIR)/patchrom $(MANDIR)/man1/patchrom.1

clean:
	$(RM) bin *.o -rv
