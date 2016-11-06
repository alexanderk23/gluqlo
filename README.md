# Gluqlo: Fliqlo for Linux

[![Gluqlo](http://alexanderk23.github.io/gluqlo/images/gluqlo.png)](https://www.youtube.com/watch?v=XhT7PBwpMIo)

Gluqlo (or Глюкало, if you prefer) is a SDL remake of well-known awesome [Fliqlo screensaver](http://9031.com/goodies/#fliqlo) which is originally avaliable for Windows and OSX platforms only.
Gluqlo is inspired by (and to some extent is based on) [noflipqlo](https://github.com/bhm/noflipqlo) aka Now Open Flipqlo 2.0 by [Jacek Kuźniarski](https://github.com/bhm).
Currently it's very close to original Fliqlo (as I hope).

## Requirements

- SDL 1.2
- SDL_gfx 1.2
- SDL_ttf 2.0
- XScreensaver (optional)

## Installing from PPA (Ubuntu from Vivid up to Xenial)

    $ sudo apt-add-repository ppa:alexanderk23/ppa
    $ sudo apt-get update
    $ sudo apt-get install gluqlo

## Building from source (Other distros)

First, install build-time dependencies (Ubuntu/Debian):

    $ sudo apt-get install build-essential libsdl1.2-dev libsdl-ttf2.0-dev libsdl-gfx1.2-dev libx11-dev

Then compile and install as usual:

    $ make && sudo make install

## Usage

If you want to use Gluqlo as a screensaver, you may need to remove gnome-screensaver (which nowadays just does nothing)
and install XScreensaver (if you're using Ubuntu, see [this page](http://www.howtogeek.com/114027/how-to-add-screensavers-to-ubuntu-12.04/) for detailed instructions).
Don't forget to add gluqlo to your ~/.xscreensaver config file (at `programs:` section):

    gluqlo -root \n\

Otherwise, you can just run it as is.

## Contributing

1. Fork it!
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create new Pull Request!
