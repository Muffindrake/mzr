# mzr

minimal GTK+/VTE-based terminal emulator

#### installation

###### dependencies

- C11 compiler (gcc 4.7+, clang 3.6+)
- largely POSIX 2008 compatible environment
- GTK+3, Glib, Pango, VTE 2.91 (runtime and development headers)
- GNU make
- pkg-config

###### ...

    $ git clone https://github.com/muffindrake/mzr
    $ cd mzr
    $ su -c "make install"

#### usage

The program accepts a few option arguments:
- `-v` to print version and exit
- `-x` to execute a command, taking a single argument
- `-t` to set terminal title
- `-g` to set initial terminal extents (`columns`x`rows`), but not position
- `-f` to set the terminal font with a pango font family definition (`"[font
  family] [point size]"`)
- `-d` to set the working directory for the terminal

Otherwise, if `-x` is _not_ used, the remaining arguments are used as a command
to execute inside the terminal. To pass non-option arguments that start with a
dash (`-`), use a double-dash argument (`--`) prior to any arguments you wish to
pass.
The terminal lets you select text using left-click-and-drag which is copied to
the clipboard.

#### configuration

Colors and some initial settings can be set at compile-time by editing config.h
to your liking - basic C knowledge is required.

#### license

See the `GPLv2` file.

#### attribution

This work is largely based upon [sakura](https://launchpad.net/sakura) which is
yet another GTK+/VTE-based terminal, but offers a lot of features that the
author found largely useless and/or downright _dangerous_ (clicking links to
open them). As such, this program is significantly more minimal.
