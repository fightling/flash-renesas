flasher-renesas
===============

Programmer for M16C micro controller.

Prerequisites
-------------

Download, build and install qextserialport library from google code:

```
  git clone https://code.google.com/p/qextserialport
  cd qextserialport
  qmake qextserialport.pro
  make
  make install
```

Installation
------------

```
  git clone https://github.com/patgithub/flasher-renesas 
  cd flasher-renesas
  qmake flasher-renesas.pro
  make
```
You'll find the binary in ./unix/release/flasher-renesas/bin/.

Usage
-----

Use Option -h to get help.

```
  flasher-renesas -h
```

To load an display a MOT file just start:

```
  flasher-renesas image.mot
```

To program a micro processor via serial port use:

```
  flasher-renesas image.mot /dev/ttyUSB0
```

The program will try to unlock with the following IDs if needed: 00:00:00:00:00:00:00 and FF:FF:FF:FF:FF:FF:FF.
Replace the /dev/ttyUSB0 by your devce path or COM1, COM2. Append a explicit ID to unlock the processor:

```
  flasher-renesas image.mot /dev/ttyUSB0 1:12:23:34:45:56:67
```

