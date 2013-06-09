BINDIR=/usr/local/bin

CFLAGS=-Wall -Werror -O -g
LDFLAGS=-ljson -lzmq

OBJS = emond.o ted.o oled.o util.o zmq.o json.o led.o gpio.o

all: emond ztled

emond: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

ztled: ztled.o led.o gpio.o
	$(CC) -o $@ ztled.o led.o gpio.o

clean:
	rm -f *.o emond

install:
	sudo install -c emond $(BINDIR)
	sudo install -c enphase-scrape.pl $(BINDIR)
	sudo crontab <crontab
