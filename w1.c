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
/* w1.c - 1-wire interface */

#define _GNU_SOURCE
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
#include <math.h>

#include "w1.h"

#define W1_PATH_TMPL	"/sys/bus/w1/devices/%s/w1_slave"

/* Example:
35 ff 4b 46 7f ff 0b 10 0a : crc=0a YES
35 ff 4b 46 7f ff 0b 10 0a t=-12687
*/

/* Return temp probe sample in degrees C */
double w1_therm_get (const char *addr)
{
	char *path = NULL;
	FILE *f = NULL;
	double val, ret = NAN;
	int crc;

	if (asprintf (&path, W1_PATH_TMPL, addr) < 0) {
		fprintf (stderr, "Out of memory\n");
		exit (1);
	}
	if (!(f = fopen (path, "r")))
		goto done;
	if (fscanf (f, "%*x %*x %*x %*x %*x %*x %*x %*x %*x : crc=%x YES",
								&crc) != 1) {
		errno = EPROTO;
		goto done;
	}
	if (fscanf (f, "%*x %*x %*x %*x %*x %*x %*x %*x %*x t=%lf",
						&val) != 1 || val == 85000) {
		errno = EPROTO;
		goto done;
	}
	ret = val/1000;
done:
	if (f)
		fclose (f);
	if (path)
		free (path);
	return ret;
}

double c2f (double c)
{
	return 9.0*c/5.0 + 32.0;
}
