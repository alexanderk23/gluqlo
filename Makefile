CC=g++
CFLAGS=-Wall -o gluqlo gluqlo.c `sdl-config --libs --cflags`
LDFLAGS=-lX11 -lSDL_ttf -lSDL_gfx

all: gluqlo

gluqlo: gluqlo.c
	$(CC) $(CFLAGS) $(LDFLAGS)

install:
	install -o root -m 0755 gluqlo /usr/lib/xscreensaver/
	install -o root -m 0644 gluqlo.ttf /usr/share/fonts/
	install -o root -m 0644 gluqlo.xml /usr/share/xscreensaver/config/

uninstall:
	rm -f /usr/share/xscreensaver/config/gluqlo.xml /usr/lib/xscreensaver/gluqlo /usr/share/fonts/gluqlo.ttf

clean:
	rm -f gluqlo
