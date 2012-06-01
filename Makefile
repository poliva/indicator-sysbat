CC=gcc
CFLAGS=-Wall -Wextra -Wwrite-strings -O -g $(shell pkg-config --cflags --libs gtk+-3.0 appindicator3-0.1 libgtop-2.0) -lsensors

all: indicator-sysbat

indicator-sysbat: indicator-sysbat.c
	$(CC) $< $(CFLAGS) -o $@

clean:
	rm -f *.o indicator-sysbat
