BINDIR=/usr/local/bin

CFLAGS=-Wall -Werror -O -g
LDFLAGS=-ljson -lzmq

SRV_OBJS = emond.o ted.o oled.o util.o zmq.o led.o gpio.o w1.o encode.o
CLI_OBJS = emon.o util.o zmq.o encode.o

all: emond emon ztled w1util tedutil

emond: $(SRV_OBJS) 
	$(CC) -o $@ $(SRV_OBJS) $(LDFLAGS)

emon: $(CLI_OBJS)
	$(CC) -o $@ $(CLI_OBJS) $(LDFLAGS)

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
	sudo install -c emon $(BINDIR)
	sudo install -c enphase-scrape.pl $(BINDIR)
	sudo crontab <crontab
