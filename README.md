indicator-sysbat
================

This is an Ubuntu appindicator that displays CPU load, CPU temperature, Proximity temperature, fan speed, memory usage and % of battery left.

Screenshot
----------

![alt text](https://github.com/poliva/indicator-sysbat/raw/master/icons/indicator-sysbat.png "indicator-sysbat screenshot")


License
-------

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.



Compile
-------

1. install build time dependancies:

    apt-get install libgtop2-dev libgtk-3-dev libappindicator3-dev libsensors4-dev

2. run `make'


Ubuntu Pakcages
---------------
Official Ubuntu packages are available in [poliva/pof ppa](https://launchpad.net/~poliva/+archive/pof):

     sudo add-apt-repository ppa:poliva/pof
     sudo apt-get update
     sudo apt-get install indicator-sysbat

