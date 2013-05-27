/* led.c - interface Kozig ZT.SEG8B4A036A-V1.1 I2C LED module to raspberry pi */

/* FIXME: can't change address from 0x27 */

/* FIXME: on same bus with oled, can read version but can't light segments */

/* N.B. the "ZTSEG8B4.rar" example provided by seller doesn't work at all
 * with this part.  I had better luck extracting info from this project:
 *   https://github.com/stanleyhuangyc/MultiLCD
 * 
 * This part has an RST pin instead of ADR.  It appears to be active-low
 * reset, not the mechanism to set the address as in the non-working example.
 * The part cannot be probed with "i2cdetect -y 1" when RST is asserted.
 * With RST floating high, it appears at factory slave address of 0x27.  
 */

#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <assert.h>

#include "led.h"

/* untested */
#define STATUS_RUN            0x00
#define STATUS_RUN            0x00
#define STATUS_SLEEP          0x01
#define STATUS_SET_ADDRESS    0x02
#define STATUS_TEST           0x04
#define STATUS_BUSY           0x10


/* decimal point |= 0x80 */
static const uint8_t charset[] = {
	0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F, /* [0]: 0-9 */
	0x77,0x7C,0x39,0x5E,0x79,0x71,			   /* [10]: A-F */
	0x40,						   /* [16]: - */
};

static uint8_t
_num (uint8_t n)
{
	return charset [n % 16];
}

static uint8_t
_char (char c)
{
	return (c >= '0' && c <= '9') ? _num (c - '0')
	     : (c >= 'a' && c <= 'f') ? _num (c - 'a' + 10)
	     : (c >= 'A' && c <= 'F') ? _num (c - 'A' + 10)
	     : (c == '-') ? charset [16]
	     : 0; /* blank */
}

static void
_write(int fd, uint8_t *buf, int len)
{
	int n;

	n = write (fd, buf, len);
	if (n < 0) {
		perror ("write");
		exit (1);
	}
	if (n < len) {
		fprintf (stderr, "short write\n");
		exit (1);
	}
}

static int
_read(int fd, uint8_t *buf, int len)
{
	int n;

	n = read (fd, buf, len);
	if (n < 0) {
		perror ("read");
		exit (1);
	}
	if (n == 0) {
		fprintf (stderr, "premature EOF on read\n");
		exit (1);
	}

	return n;
}

static void
_led_display (int fd, uint8_t *val)
{
	uint8_t buf[] = { 0x02, val[3], val[2], val[1], val[0] };
	_write (fd, buf, sizeof (buf));
}

static void
_led_puts (int fd, char *s)
{
	uint8_t buf[4];
	char *p = s;
	int i = 0;

	memset (buf, 0, sizeof (buf));
	for (i = 0, p = s; *p != '\0' && i < 4; p++) {
		if (*p == '.' && i == 0)
			buf[i++] = 0x80;
		else if (*p == '.' && i > 0)
			buf[i - 1] |= 0x80;
		else
			buf[i++] = _char(*p);
	}
	_led_display (fd, buf);
}

void
led_printf (int fd, const char *fmt, ...)
{
	char s[10];
	va_list ap;

	va_start (ap, fmt);
	vsnprintf (s, sizeof (s), fmt, ap);
	va_end (ap);
	_led_puts (fd, s);
}

void
led_brightness_set (int fd, uint8_t val)
{
	uint8_t buf[] = { 0x0a, val,  0xff, 0 , 0};
	_write (fd, buf, sizeof (buf));
}

/* N.B. not working - see comment at the top */
void
led_addr_set (uint8_t oldaddr, uint8_t newaddr)
{
	uint8_t buf[] = { 0x08, newaddr };
	int fd;

	fd = led_init (oldaddr);	
	_write (fd, buf, sizeof (buf));
	led_fini (fd);
}

/* 1=sleep, 0=wake */
void
led_sleep_set (int fd, int val)
{
	uint8_t buf[] = { 0x04, val ? 0xa5 : 0xa1, 0, 0, 0 };
	_write (fd, buf, sizeof (buf));
}

uint8_t 
led_status_get (int fd)
{
	uint8_t wbuf[] = { 0x06 };
	uint8_t rbuf[1];

	_write (fd, wbuf, sizeof (wbuf));
	if (_read (fd, rbuf, sizeof (rbuf)) != 1) {
		perror ("read status byte");
		exit (1);
	}
	return rbuf[0];
}

int
led_init(int addr)
{
	const char *devname = "/dev/i2c-1";
	int fd;

	fd = open (devname, O_RDWR);
	if (fd < 0) {
		perror (devname);
		exit (1);
	}
	if (ioctl (fd, I2C_SLAVE, addr) < 0) {
		perror ("ioctl I2C_SLAVE");
		exit (1);
	}
	return fd;
}

void
led_fini(int fd)
{
	close (fd);
}

#if 0
static void
_led_version_print (int fd)
{
	uint8_t wbuf[] = { 0x1f };
	uint8_t rbuf[19];
	int n;

	_write (fd, wbuf, sizeof (wbuf));
	n = _read (fd, rbuf, sizeof (rbuf));
	printf ("%.*s\n", n, (char *)rbuf);
}

static void
_led_test (int fd)
{
	int i;

	_led_version_print (fd);
	for (i = -32; i < 32; i++) {
		led_printf (fd, "% -.3d", i);
		usleep (1000*100);
	}
	led_printf (fd, "");
	usleep (1000*1000);
	led_printf (fd, "%1.3f", 3.14159);
	usleep (1000*1000);
	led_printf (fd, "%-1.3f", .2);
}

int
main (int argc, char *argv[])
{
	int fd;

	led_addr_set (0x27, 0x29);
	fprintf (stderr, "address set to 0x29\n");

	fd = led_init (0x29);	

	_led_test (fd);

	led_fini (fd);
	exit (0);
}
#endif
