# recfsusb2n   for Linux OSes
2017-01-22, Ver. 0.1.13


## What is this?

* record TS data from uSUNpTV (ISDB-T/S TV Tuner), FSUSB2N (ISDB-T TV Tuner, NOT IMPLEMENTED)
* monitor signal status, channel status



## System Requirements

Linux-based OS
usbdevfs (usbfs) with USB 2.0



## Files

src/
	* source code files (gcc C99)
	Makefile{,.dep}
	*.{c,h}

/
	readMe.txt
	doc.txt
		* instructions, usage, FAQ



## User Agreement

USE AT YOUR OWN RISK, no warranty

You can ...
* use this package for any purposes.
* redistribute a copy of this package without modifying.
* modify this package.
* redistribute the modified package. You should write the modification clearly.

based on GPLv3



## How to compile and build?

I recommend the GCC C compiler. Please install "gcc".

  $ cd src
run GNU "make" command.
  $ make

* "make B25=1"  if you use the STD-B25 interface library

You will get a executable file "recfsusb2n".
Check to see if it is working properly.

Last, install to "/usr/local/bin/" directory.
  $ make install



## End

(c) 2016 trinity19683
signed by "trinity19683.gpg-pub" 2015-12-13
