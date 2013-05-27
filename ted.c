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
/* ted.c - decode messages from TED 1000 current sensor PLM */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <stdint.h>

#include "ted.h"

static FILE *ted = NULL;

static int32_t raw_power(uint8_t *pkt)
{
	uint32_t i;
	
	i = (uint32_t)pkt[3] | (uint32_t)pkt[4] << 8 | (uint32_t)pkt[5] << 16;
	if (i & 0x00800000)
		i |= 0xff000000;
	return (int32_t)i;
}

static int32_t raw_voltage(uint8_t *pkt)
{
	uint32_t i;

	i = (uint32_t)pkt[6] | (uint32_t)pkt[7] << 8 | (uint32_t)pkt[8] << 16;
	if (i & 0x00800000)
		i |= 0xff000000;
	return (int32_t)i;
}

static int verify_cksum(uint8_t *pkt)
{
	int sum = 0;
	int i;

	for (i = 0; i < 11; i++)
		sum += pkt[i];
	sum &= 0xff;

	return (sum == pkt[9]);
}
		
int ted_read(int *addrp, int *countp, int *wattsp, int *voltsp)
{
	int cc, i;
	uint8_t c, pkt[11];
	double power, voltage; /* kW and V */

	i = 0;
	while (i < 11 && (cc = fgetc(ted)) != EOF) {
		c = ~(uint8_t)cc;
		if (c != 0x55 && i == 0)
			continue; /* resync */
		pkt[i++] = c;
	} 
	if (cc == EOF) {
		if (errno == 0)
			errno = EIO;
		return -1;
	}

	if (!verify_cksum (pkt)) {
		errno = EINVAL;
		return -1;
	}

	/* per http://gangliontwitch.com/ted/ */
	voltage = 123.6 + (raw_voltage(pkt)/256 - 27620) / 85 * 0.4;
	power = 1.19 + 0.84 * ((raw_power(pkt)/256 - 288.0) / 204.0);

	if (addrp)
		*addrp = pkt[1];
	if (countp)
		*countp = pkt[2];
	if (wattsp)
		*wattsp = power * 1000;
	if (voltsp)
		*voltsp = voltage;
	return 0;
}

int ted_init(char *devname)
{
	int fd;
	struct termios tio;

	fd = open(devname, O_RDWR);
	if (fd < 0)
		return -1;
	if (flock(fd, LOCK_EX | LOCK_NB) < 0) {
		close (fd);
		return -1;
	}
	if (tcgetattr(fd, &tio) < 0) {
		close (fd);
		return -1;
	}
	cfsetspeed(&tio, B1200);/* 1200 baud */
	tio.c_cflag &= ~CSIZE;	/* 8 bits */
	tio.c_cflag |= CS8;
	tio.c_cflag &= ~CSTOPB;	/* 1 stop bit */
	tio.c_cflag &= ~PARENB;	/* no parity */
	cfmakeraw(&tio);
	tio.c_cc[VMIN] = 1;
	tio.c_cc[VTIME] = 0;
	if (tcsetattr(fd, TCSANOW, &tio) < 0) {
		close (fd);
		return -1;
	}
	if ((ted = fdopen(fd, "r")) == NULL)
		return -1;
	return 0;
}

void ted_fini (void)
{
	fclose (ted);
}

#if 0
int main (int argc, char *argv[])
{
	int addr, count, volts, watts;

	if (ted_init ("/dev/ttyAMA0") < 0) {
		perror ("/dev/ttyAMA0");
		return 1;
	}

	for (;;) {
		if (ted_read (&addr, &count, &volts, &watts) < 0) {
			if (errno != EINVAL) {
				perror ("ted_read");
				break;
			}
			fprintf (stderr, "bad packet\n");
		}
		printf ("addr=%d count=%d volts=%d watts=%d\n",
			addr, count, volts, watts);
	}

	ted_fini ();

	return 0;
}
#endif
