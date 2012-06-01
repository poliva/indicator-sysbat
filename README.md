indicator-sysbat
================

This is an Ubuntu appindicator that displays CPU load, CPU temperature, Proximity temperature, fan speed, memory usage and % of battery left.
At the moment most sensors will only work on Macbook based computers (I wrote this for my own MacbookAir), i'll try to implement a more configurable sensor selector in the future, so it will be more generic.


License
-------

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.



Compile
-------

1. install build time dependancies:

    apt-get install libgtop2-dev libgtk-3-dev libappindicator3-dev libsensors4-dev

2. run `make'
