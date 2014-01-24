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
/* tedutil.c - dump ted data */

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

int main (int argc, char *argv[])
{
	int addr, count, volts, watts;

	if (ted_init ("/dev/ttyAMA0") < 0) {
		perror ("/dev/ttyAMA0");
		return 1;
	}

	for (;;) {
		if (ted_read (&addr, &count, &watts, &volts) < 0) {
			if (errno != EINVAL) {
				perror ("ted_read");
				break;
			}
			fprintf (stderr, "bad packet\n");
			continue;
		}
		printf ("addr=%d count=%d volts=%d watts=%d\n",
			addr, count, volts, watts);
	}

	ted_fini ();

	return 0;
}
