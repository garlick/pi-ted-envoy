
pi-ted-envoy
==============

Home Energy Monitor on Raspberry Pi

When I installed solar panels, I found my TED 1001 home energy
monitor was unable to display negative net power usage.  When
it displayed a small number, it was impossible to tell without switching
things on and off whether the value was positive or negative.  Grr.

The TED current sensor, which regularly transmits a sample over the
power line to a power line modem (PLM) inside the TED, does transmit
negative values though.

To get the sign of the value displayed, the main board was extracted
from the TED unit and mounted a project box beside a Raspberry Pi,
with the 3V3 serial output from the TED PLM wired directly to the Pi
serial port (P-1 pins 6=TxD and 10=RxD).
See [this page](http://gangliontwitch.com/ted/) for how to tap the TED PLM.

A tiny OLED display from Digole (ebay) was interfaced to the Pi's
I2C port (P-1 pins 3=SDA and 5=SCL).

Finally, why not display energy production too?
I found another Enphase Envoy user had posted a
[script](http://sandeen.net/wordpress/energy/solar-monitoring/)
to scrape data from its local monitoring page:

For fun, I used the [ZeroMQ](http://www.zeromq.org/) message library here.
A daemon called _emond_ listens for messages on a TCP port from the
envoy script, run from cron once a minute; and for messages on shared
memory from a thread watching the TED serial port.  Both messages are
encoded as JSON.  Each time the main program receives a message from
either, it updates the OLED display.

You'll need the Raspbian packages for ZeroMQ and JSON:
    libzeromq-perl
    libzmq-dev
    libzmq1
    libjson-perl
    libjson0
    libjson0-dev

Enjoy.
