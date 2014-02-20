# Gluqlo: Fliqlo for Linux

![Gluqlo screenshot](http://alexanderk.ru/images/gluqlo.png)

Gluqlo (or Глюкало, if you prefer) is a SDL remake of well-known awesome [Fliqlo screensaver](http://9031.com/goodies/#fliqlo) which is originally avaliable for Windows and OSX platforms only.
Gluqlo is inspired by (and to some extent is based on) [noflipqlo](https://github.com/bhm/noflipqlo) aka Now Open Flipqlo 2.0 by [Jacek Kuźniarski](https://github.com/bhm).
Currently it's very close to original Fliqlo (as I hope); however, flipping animation is not implemented yet :)

## Requirements

- SDL 1.2
- SDL_gfx 1.2
- SDL_ttf 2.0
- XScreensaver (optional)

## Installation

First, install build-time dependencies (Ubuntu/Debian):

    $ sudo apt-get install libsdl1.2-dev libsdl-ttf2.0-dev libsdl-gfx1.2-dev libx11-dev ttf-droid

Then compile and install as usual:

    $ make && sudo make install

## Usage

If you want to use Gluqlo as a screensaver, you may need to remove gnome-screensaver (which nowadays just does nothing)
and install XScreensaver (if you're using Ubuntu, see [this page](http://www.howtogeek.com/114027/how-to-add-screensavers-to-ubuntu-12.04/) for detailed instructions;
don't forget to add gluqlo to your ~/.xscreensaver config file).
Otherwise, you can just run it as is.

## Contributing

1. Fork it!
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create new Pull Request!
