BINDIR=/usr/local/bin

CFLAGS=-Wall -Werror -O -g
LDFLAGS=-ljson -lzmq

OBJS = emond.o ted.o oled.o util.o zmq.o json.o led.o

all: emond ztled

emond: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

ztled: ztled.o led.o
	$(CC) -o $@ ztled.o led.o

clean:
	rm -f *.o emond

install:
	sudo install -c emond $(BINDIR)
	sudo install -c enphase-scrape.pl $(BINDIR)
	sudo crontab <crontab
