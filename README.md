iwatch
======

`iwatch` is a program to watch the given command's output periodically
and watch the output change.

Installation
------------

    % git clone https://github.com/iij/iwatch
    % cd iwatch
    % ./configure
    % gmake
    % su
    # gmake install

Please remark to use "gmake" to use GNUmakefile.  Makefile is for
OpenBSD.

For OpenBSD

    % git clone https://github.com/iij/iwatch
    % cd iwatch
    % make
    % su
    # make BINDIR=/usr/local/bin MANDIR=/usr/local/man/man install


How to use
----------

    % iwatch -e netstat -s
