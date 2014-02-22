CC=g++
FONTDIR=/usr/share/gluqlo
CFLAGS=-Wall -o gluqlo gluqlo.c `sdl-config --libs --cflags` -DFONT='"$(FONTDIR)/gluqlo.ttf"'
LDFLAGS=-lX11 -lSDL_ttf -lSDL_gfx

all: gluqlo

gluqlo: gluqlo.c
	$(CC) $(CFLAGS) $(LDFLAGS)

install:
	strip gluqlo
	[ -d /usr/lib/xscreensaver ] && install -o root -m 0755 gluqlo /usr/lib/xscreensaver/
	[ -d /usr/share/xscreensaver/config ] && install -o root -m 0644 gluqlo.xml /usr/share/xscreensaver/config/
	mkdir -p $(FONTDIR) && install -o root -m 0644 gluqlo.ttf $(FONTDIR)/

uninstall:
	rm -f /usr/share/xscreensaver/config/gluqlo.xml /usr/lib/xscreensaver/gluqlo $(FONTDIR)/gluqlo.ttf

clean:
	rm -f gluqlo
