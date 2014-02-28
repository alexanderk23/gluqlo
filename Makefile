CC=g++
FONTDIR=/usr/share/gluqlo
CFLAGS=-Wall -o gluqlo gluqlo.c `sdl-config --libs --cflags` -DFONT='"$(FONTDIR)/gluqlo.ttf"'
LDFLAGS=-lX11 -lSDL_ttf -lSDL_gfx

all: gluqlo

gluqlo: gluqlo.c
	$(CC) $(CFLAGS) $(LDFLAGS)

install:
	strip gluqlo
	mkdir -p /usr/lib/xscreensaver && install -o root -m 0755 gluqlo /usr/lib/xscreensaver/
	mkdir -p $(FONTDIR) && install -o root -m 0644 gluqlo.ttf $(FONTDIR)/
	install -o root -m 0644 gluqlo.png /usr/share/pixmaps/
	[ -d /usr/share/xscreensaver/config ] && install -o root -m 0644 gluqlo.xml /usr/share/xscreensaver/config/
	[ -d /usr/share/applications/screensavers ] && install -o root -m 0644 gluqlo.desktop /usr/share/applications/screensavers/

uninstall:
	rm -f /usr/share/xscreensaver/config/gluqlo.xml /usr/share/applications/screensavers/gluqlo.desktop \
		/usr/lib/xscreensaver/gluqlo $(FONTDIR)/gluqlo.ttf /usr/share/pixmaps/gluqlo.png

clean:
	rm -f gluqlo
