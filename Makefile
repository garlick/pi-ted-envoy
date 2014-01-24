BINDIR=/usr/local/bin

CFLAGS=-Wall -Werror -O -g
LDFLAGS=-ljson -lzmq

OBJS = emond.o ted.o oled.o util.o zmq.o led.o gpio.o w1.o

all: emond ztled w1util tedutil

emond: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

ztled: ztled.o led.o gpio.o
	$(CC) -o $@ ztled.o led.o gpio.o

w1util: w1.o w1util.o
	$(CC) -o $@ w1.o w1util.o

tedutil: ted.o tedutil.o
	$(CC) -o $@ ted.o tedutil.o

clean:
	rm -f *.o emond w1util ztled tedutil

install:
	sudo install -c emond $(BINDIR)
	sudo install -c enphase-scrape.pl $(BINDIR)
	sudo crontab <crontab
