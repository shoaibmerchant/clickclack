Clickclack
=================================

Clickclack is a simple haptic feedback and audio feedback tool. It does
nothing more than emulate the clickclack sound and vibration when pressing keys.

This tools  reads from standard input and emits a vibration and/or a sound whenever it receives a character.  It is
intented to be used in combination with a virtual keyboard like [svkbd](https://tools.suckless.org/x/svkbd/) (X11) or
[wvkbd](https://github.com/jjsullivan5196/wvkbd) (wayland).

Installation
------------

	$ make
	$ make install

Usage
-----

You will want to use clickclack in combination with another tool, most likely a virtual keyboard.

Clickclack Options:

* ``-f`` - A wave file to load and play, note that only the first milliseconds as specified by ``-D`` are actually
	played.
* ``-V`` - Enable vibration (requires a vibration motor)
* ``-t`` *(integer)* - Audio file duration in milliseconds (defaults to 95 milliseconds), the rest will be clipped!
* ``-d`` *(integer)* - Vibration duration in milliseconds (defaults to 95 milliseconds)
* ``-D`` - Debug mode
* ``-e`` - Echo input to output (allows further chaining of tools)

Virtual keyboards [svkbd](https://tools.suckless.org/x/svkbd/) and [wvkbd](https://github.com/jjsullivan5196/wvkbd) have an extra output mode where all keypresses are printed to standard output. This allows us
to use clickclack with it as follows:

	$ svkbd-mobile-intl -o | clickclack -V -f keypress.wav

Notes
---------

When running standalone and interactively in a terminal, the terminal's buffering will interfere so you won't get the
expected functionality of hearing a sound after every keypress (but only after you submit a line).

Repository
----------

	https://git.sr.ht/~proycon/clickclack
