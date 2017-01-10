veles - A new age tool for binary analysis
==========================================

See our home page at https://codisec.com/.

See binglide's `readme for an other explanation of the n-gram
principle in visual reverse engineering.
<https://github.com/wapiflapi/binglide

Usage
=====

To move around the view:

- Mouse click/drag to rotate. +Shift to keep turning.
- WASDQE or Arrow keys & Page-Up/Down (shift+up/down == page-up/down)

To change the projections use the buttons. They are also mapped in the
same position to the 123456 keys on the keypad.

Building, installing
--------------------

Dependencies:

- ``cmake`` >= 3.1.0
- ``zlib``
- ``qt`` >= 5.5

Optional dependencies needed for running tests:

- ``gtest``

If your distribution has -dev or -devel packages, you'll also need ones
corresponding to the dependencies above.

On ubuntu it can be done like this::

    apt-get install cmake zlib zlib-dev qtbase5 qtbase5-dev

To build ::

    $ mkdir build
    $ cd build
    $ cmake -D CMAKE_BUILD_TYPE=Release ..
    $ make

To install [which is optional], use ::

    $ make install

If you want to install to a non-default directory, you'll also need to pass
it as an option to cmake before building, eg.::

    $ cmake -D CMAKE_INSTALL_PREFIX:PATH=/usr/local ..
