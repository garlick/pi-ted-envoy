
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
I2C port (P-1 pins 3=SDA and 5=SCL).  Later, a 4-digit I2C LED module
was added which can be seen from a greater distance.

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
a message, it updates the OLED display, which looks like this:
```
    gen: +0.269 kW
    use: -1.255 kW
    net: -0.986 kW

    day: +8.830 kWH
```
The LED display just echos the _use_ number (without the minus sign),
so it can be seen from a distance.

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
