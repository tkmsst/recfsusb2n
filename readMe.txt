-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA256

# recfsusb2n   for Linux OSes
2016-02-11, Ver. 0.1.12


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
	sha1sum.txt
	sha1sum.txt.sig
		* gpg --verify sha1sum.txt.sig



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

Last, copy to ".../bin" directory.
  $ cp recfsusb2n /usr/local/bin/



## End

(c) 2016 trinity19683
signed by "trinity19683.gpg-pub" 2015-12-13

You can verify this file by using the GnuPG key.

$ gpg --import trinity19683.gpg-pub
$ gpg --verify readMe.txt
-----BEGIN PGP SIGNATURE-----
Version: GnuPG v1

iQEcBAEBCAAGBQJWu7E9AAoJEB0tpC02lUUYi/oH/RrJrDryqP3+1nHqImWYmOg0
TwzeNFMTq2Ru2soYW7FOFvmvLvah1gZqhZcShv0T3cgEAIb9CcEIyN/B//HaqoGU
w75wmV8dWil/qR0gtFw4huYkqBpCrDmWanLi4rpoOdZ55AoNMi9AIypYFiYhzvs6
YJuqe1D8LPsdZpMcBXFpkYIJ/zcPdIS1vTB9jGtkgRc+gxnkvoJfHaNUmm8LPlZC
VzM2tBDo/b90qOpatyQ3jCpEn0bOG9/KJRwI/aIKKZW+Fp9sMHSS1WOwRrmYXATJ
51ap5R0lHLV9WG9gFJakPONKhJvZZo95DLCRgkAX6ZTFnZd+8+/BL7r4WBAlkJM=
=T6Cv
-----END PGP SIGNATURE-----
