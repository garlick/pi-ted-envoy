/*****************************************************************************
 *  Copyright (C) 2013 Jim Garlick
 *  Written by Jim Garlick <garlick.jim@gmail.com>
 *  All Rights Reserved.
 *
 *  This file is part of pi-ted-envoy.
 *  For details, see <https://github.com/garlick/pi-ted-envoy>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License (as published by the
 *  Free Software Foundation) version 2, dated June 1991.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA or see
 *  <http://www.gnu.org/licenses/>.
 *****************************************************************************/
/* oled.c - interface Digole 128x64 I2C OLED module to raspberry pi */

/* FIXME: support graphics modes */

#define _GNU_SOURCE /* for vasprintf */
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "oled.h"

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

/* clear screen, set display pos=0,0, default font, cursor off */
void
oled_clear(int fd)
{
	uint8_t buf[] = { 'C', 'L' };
	_write(fd, buf, sizeof (buf));
}

/* 0=off, 1=on */
void
oled_cursor_set (int fd, bool val)
{
	uint8_t buf[] = { 'C', 'S', val ? 1 : 0 };
	_write(fd, buf, sizeof (buf));
}

/* 0=screen off, 1=screen on */
void
oled_sleep_set (int fd, bool val)
{
	uint8_t buf[] = { 'S', 'O', 'O', val ? 1 : 0 };
	_write(fd, buf, sizeof (buf));
}

/* zero origin */
void
oled_text_pos_set (int fd, uint8_t x, uint8_t y)
{
	uint8_t buf[] = { 'T', 'P', x, y };
	_write(fd, buf, sizeof (buf));
}

static void
_oled_puts (int fd, char *s)
{
	int len = strlen (s) + 1;
	uint8_t *buf = malloc (len + 2);
	if (!buf) {
		fprintf (stderr, "out of memory\n");
		exit (1);
	}
	buf[0] = 'T';
	buf[1] = 'T';
	memcpy(&buf[2], s, len);
	_write(fd, buf, len + 2);
	free (buf);
}

void
oled_printf (int fd, const char *fmt, ...)
{
	va_list ap;
	char *s;
	int n;

	va_start (ap, fmt);
	n = vasprintf (&s, fmt, ap);
	if (n < 0) {
		fprintf (stderr, "out of memory\n");
		exit (1);
	}
	va_end (ap);
	_oled_puts (fd, s);
	free (s);
}

void
oled_addr_set (int oldaddr, int newaddr)
{
	uint8_t buf[] = { 'S', 'I', '2', 'C', 'A', newaddr };
	int fd;

	fd = oled_init (oldaddr);
	_write(fd, buf, sizeof (buf));
	oled_fini (fd);
}

int
oled_init(int addr)
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
oled_fini(int fd)
{
	close (fd);
}

#if 0
int main (int argc, char *argv[])
{
	int fd;

	//oled_addr_set (0x27, 0x28);

	fd = oled_init (0x28);
	oled_clear (fd);
	usleep (1000*500);
	oled_printf (fd, "Hello world\n");
	oled_fini (fd);

	return 0;
}
#endif
