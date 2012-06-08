DESTDIR?=/
SHELL = /bin/sh
CC?=gcc
CFLAGS=-Wall -Wextra -Wwrite-strings -O -g $(shell pkg-config --cflags --libs gtk+-3.0 appindicator3-0.1 libgtop-2.0) -lsensors
INSTALL = /usr/bin/install -c
INSTALLDATA = /usr/bin/install -c -m 644
PROGNAME = indicator-sysbat

srcdir = .
prefix = $(DESTDIR)
bindir = $(prefix)/usr/bin
docdir = $(prefix)/usr/share/doc
mandir = $(prefix)/usr/share/man

all: indicator-sysbat

indicator-sysbat: indicator-sysbat.c
	$(CC) $< $(CFLAGS) -o $@

install: all
	mkdir -p $(bindir)
	$(INSTALL) $(PROGNAME) $(bindir)/$(PROGNAME)
	mkdir -p $(prefix)/etc/xdg/autostart/
	$(INSTALLDATA) $(PROGNAME).desktop $(prefix)/etc/xdg/autostart/
	mkdir -p $(docdir)/$(PROGNAME)/
	$(INSTALLDATA) $(srcdir)/README.md $(docdir)/$(PROGNAME)/
	$(INSTALLDATA) $(srcdir)/LICENSE $(docdir)/$(PROGNAME)/
#	mkdir -p $(mandir)/man1/
#	$(INSTALLDATA) $(srcdir)/$(PROGNAME).1 $(mandir)/man1/

uninstall:
	rm -rf $(bindir)/$(PROGNAME)
	rm -rf $(prefix)/etc/xdg/autostart/$(PROGNAME).desktop
	rm -rf $(docdir)/$(PROGNAME)/
#	rm -rf $(mandir)/man1/$(PROGNAME).1


clean:
	rm -f *.o indicator-sysbat
