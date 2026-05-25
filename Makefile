CC=g++
FONTDIR=$(DESTDIR)/usr/share/gluqlo

XSCREENSAVER ?= 1

CFLAGS=-Wall -o gluqlo gluqlo.c `sdl2-config --libs --cflags` -DFONT='"$(FONTDIR)/gluqlo.ttf"'
LDFLAGS=-lSDL2_ttf

ifeq ($(XSCREENSAVER),1)
CFLAGS += -DXSCREENSAVER
LDFLAGS += -lX11
endif

all: gluqlo

gluqlo: gluqlo.c
	$(CC) $(CFLAGS) $(LDFLAGS)

install:
	strip gluqlo
	install -o root -m 0755 -D gluqlo $(DESTDIR)/usr/libexec/xscreensaver/gluqlo
	install -o root -m 0644 -D gluqlo.ttf $(FONTDIR)/gluqlo.ttf
	install -o root -m 0644 -D gluqlo.png $(DESTDIR)/usr/share/pixmaps/gluqlo.png
	install -o root -m 0644 -D gluqlo.xml $(DESTDIR)/usr/share/xscreensaver/config/gluqlo.xml
	install -o root -m 0644 -D gluqlo.desktop $(DESTDIR)/usr/share/applications/screensavers/gluqlo.desktop

uninstall:
	rm -f $(DESTDIR)/usr/share/xscreensaver/config/gluqlo.xml $(DESTDIR)/usr/share/applications/screensavers/gluqlo.desktop \
		$(DESTDIR)/usr/libexec/xscreensaver/gluqlo $(FONTDIR)/gluqlo.ttf $(DESTDIR)/usr/share/pixmaps/gluqlo.png

clean:
	rm -f gluqlo
