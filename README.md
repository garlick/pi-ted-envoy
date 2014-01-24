
pi-ted-envoy
==============

Home Energy Monitor on Raspberry Pi

When we installed solar panels, we discovered our TED 1001 home energy
monitor measured the _net_ home energy usage (usage minus production).
This was OK and not unexpected, but the TED only displayed the absolute
value of the measurement.  The sign could only be determined by turning
on a light and observing whether the reading jumped up or down.
The TED current sensor, which regularly transmits a sample over the
power line to a power line modem (PLM) inside the TED, does transmit
negative values though.

To build a monitor which can display energy consumption, production,
and net energy for the home, the main board was extracted from the TED
unit and mounted in a project box beside a Raspberry Pi.
The 3V3 serial output from the TED PLM was wired directly to the Pi
serial port (P-1 pins 6=TxD and 10=RxD).
See [this page](http://gangliontwitch.com/ted/) for how to tap the TED PLM
and begin the warranty voiding process.

A tiny 128x64 OLED display from Digole (ebay) was interfaced to the Pi's
I2C port (P-1 pins 3=SDA and 5=SCL), along with two Kozig (ebay)
4-digit I2C LED modules.

To obtain energy production information, I modified a 
[perl script](http://sandeen.net/wordpress/energy/solar-monitoring/)
posted by another Enphase Envoy user which can scrape data from the Envoy's
local monitoring page.  

For fun, even though it's a bit of overkill for this simple project,
the [ZeroMQ](http://www.zeromq.org/) message library was used to
tie the pieces together.  A daemon called _emond_ listens for messages
on a TCP port from the Envoy-scraping perl script, run from cron once a
minute; and for messages on shared memory from a thread watching the
TED serial port.
Both messages are encoded as JSON.  Each time the main program receives
a message, it updates displays.  The OLED display looks like this:
```
    DAILY ENERGY
    use 20.003 kWH
    gen 8.830 kWH

    TED -1906W 123V
```
The _use_ value is energy use since midnight, accumulated in memory
based on TED and Envoy values sampled during the day.
The _gen_ value reflects energy production since midnight and is scraped
directly from the Envoy.
The TED line shows the most recent raw TED sample.

The LEDs display the instantaneous consumption and production in kW.

The following packages, available in the Raspbian wheezy distro,
are prerequisites for this project:
```
    libzeromq-perl
    libzmq-dev
    libzmq1
    libjson-perl
    libjson0
    libjson0-dev
    liblwp-protocol-https-perl
```

Addendum: fridge temps
======================

Since the energy monitor sits on top of my fridge, it seemed natrual
that it should grow some 1-wire temperature sensors.

To this end, I soldered a DS2484 1-wire I2C interface chip and one DS1820B
temperature sensor to a Pi Plate, and mounted an RJ11 connector out the back
of the energy monitor.  Potted DS1820B sensors (ebay) were mounted in the
freezer and refrigerator compartments.  Pi GPIO4 was connected to the SLPZ
line on the DS2484.

The following runes are needed in /etc/rc.local:
```
    echo "4"    >/sys/class/gpio/export
    echo "out"  >/sys/class/gpio/gpio4/direction
    echo "1"    >/sys/class/gpio/gpio4/value
    modprobe ds2482
    modprobe w1_therm strong_pullup=0
    echo ds2482 0x18 >/sys/bus/i2c/devices/i2c-1/new_device
```

The emond.c now monitors these sensors as well, and switches the display
mode between energy monitor and fridge monitor when a switch on the front
panel is depressed.
