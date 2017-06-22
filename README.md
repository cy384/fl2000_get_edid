fl2000_get_edid
===============
NOTE: This is superseded by [FL's release of a bunch of interface code](https://github.com/fresco-fl2000/fl2000)!

A minimal, experimental program to read EDID from a Fresco Logic FL2000 display adapter (i.e. VID/PID 1d5c:2000).  Tested only on Linux (Ubuntu 14.04/amd64) but should work anywhere libusb does.  Further details on my FL2000 efforts can be found [here](http://www.cy384.com/projects/fl2000dx-driver.html).

caveats
-------
This is a reverse engineered piece of software: it does some things because "it needs to do this to work" and not necessarily anything I understand further.  The Windows driver does some dubious things that libusb cannot be persuaded into, so this is not exactly the same as the official version.  It is touchy and possibly broken under some conditions.  I don't think it's possible to brick an FL2000 device (surely I would have managed it already), but by running this program you are accepting any resultant risk.

setup/build
-----------
You will need a C compiler, libusb with headers etc., and a udev rule to allow non-root access to the device.  Optionally, the read-edid package for parsing the output.  Define the DEBUG macro to build in debug mode, which will print human-readable information about the transfers happening (e.g. add -DDEBUG).  For Ubuntu 14.04:

	sudo apt-get install build-essential libusb-1.0-0-dev read-edid
	sudo cp 80-fl2000-dev.rules /etc/udev/rules.d/
	gcc fl2000_get_edid.c -o fl2000_get_edid -I/usr/include/libusb-1.0 -lusb-1.0 -Wall

usage
-----
Run program, recieve EDID.  If a monitor is not attached when the program runs, it will wait.  Invocation examples:

	./fl2000_get_edid | parse-edid
or:

	./fl2000_get_edid > edid.bin

license
-------
This code is dual-licensed; you may use, distribute, or modify it under either the terms of the BSD license (see LICENSE file), or the GPL version 2 (see LICENSE_GPL file).
