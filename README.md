flash-renesas
===============

Programmer for M16C micro controller.

Prerequisites
-------------

You must have installed the QT 5.3 runtime (see http://qt-project.org/downloads).

Installation
------------

```
  git clone https://github.com/patgithub/flash-renesas 
  cd flash-renesas
  qmake flash-renesas.pro
  make
```
You'll find the binary in ```./unix/release/flash-renesas/bin/```.

Usage
-----

Use Option -h to get help.

```
  flash-renesas -h
```

To load an display a MOT file just start:

```
  flash-renesas image.mot
```

To program a micro processor via serial port use:

```
  flash-renesas image.mot /dev/ttyUSB0
```

The program will try to unlock with the following IDs if needed: ```00:00:00:00:00:00:00``` and ```FF:FF:FF:FF:FF:FF:FF```.
Replace the ```/dev/ttyUSB0``` by your devce path or ```COM1```, ```COM2```. Append a explicit ID to unlock the processor:

```
  flash-renesas image.mot /dev/ttyUSB0 1:12:23:34:45:56:67
```

