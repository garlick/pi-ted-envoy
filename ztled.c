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
/* ztled.c - led config utility */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <getopt.h>
#include <unistd.h>

#include "led.h"

#define OPTIONS "tai:x:d:s:b:r"
#define HAVE_GETOPT_LONG 1

#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long (ac,av,opt,lopt,NULL)
static const struct option longopts[] = {
    {"test",            no_argument,        0, 't'},
    {"set-address",     no_argument,        0, 'a'},
    {"integer",         required_argument,  0, 'i'},
    {"hex",             required_argument,  0, 'x'},
    {"double",          required_argument,  0, 'd'},
    {"string",          required_argument,  0, 's'},
    {"brightness",      required_argument,  0, 'b'},
    {"reset",           no_argument,        0, 'r'},
    {0, 0, 0, 0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt (ac,av,opt)
#endif

static void usage (void)
{
    fprintf (stderr,
"Usage: ztled [OPTIONS] ADDR\n"
"   -t,--test               light each LED segment individually\n"
"   -a,--set-address        set address (part must be in addr mode)\n"
"   -i,--integer N          display integer\n"
"   -x,--hex N              display hex\n"
"   -d,--double N           display double\n"
"   -s,--string str         display string\n"
"   -b,--brightness N       set brightness (0-0xff)\n"
"   -r,--reset              reset device\n"
    );
    exit (1);
}

int main (int argc, char *argv[])
{
    int c;
    int topt = 0;
    int aopt = 0;
    int iopt = 0;
    int iopt_arg, xopt_arg;
    int xopt = 0;
    int dopt = 0;
    double dopt_arg;
    int sopt = 0;
    char *sopt_arg;
    int bopt = 0;
    int bopt_arg;
    int ropt = 0;
    int addr;
    int fd;

    while ((c = GETOPT (argc, argv, OPTIONS, longopts)) != -1) {
        switch (c) {
            case 't':
                topt = 1;
                break;
            case 'a':
                aopt = 1;
                break;
            case 'i':
                iopt = 1;
                iopt_arg = strtol (optarg, NULL, 0);
                break;
            case 'x':
                xopt = 1;
                xopt_arg = strtoul (optarg, NULL, 0);
                break;
            case 'd':
                dopt = 1;
                dopt_arg = strtod (optarg, NULL);
                break;
            case 's':
                sopt = 1;
                sopt_arg = optarg;
                break;
            case 'b':
                bopt = 1;
                bopt_arg = strtoul (optarg, NULL, 0);
                break;
            case 'r':
                ropt = 1;
                break;
            default:
                usage ();
        }
    }
    if (optind != argc - 1)
        usage ();
    if (!topt && !aopt && !iopt && !xopt && !dopt && !sopt && !ropt)
        usage ();
    if (topt && aopt)
        usage ();
    addr = strtoul (argv[optind], NULL, 0);

    if (aopt) {
        fd = led_init (LED_ADDR_ADDRMODE);
        led_addr_set (fd, addr);
        led_fini (fd);
        exit (0);
    }
     

    fd = led_init (addr);
    led_sleep_set (fd, 0);

    if (ropt)
        led_reset (fd);

    if (bopt)
        led_brightness_set (fd, bopt_arg);

    if (topt)
        led_test (fd);
    if (iopt)
        led_printf (fd, "%+.3d", iopt_arg);
    if (xopt)
        led_printf (fd, "%.4x", xopt_arg);
    if (dopt)
        led_printf (fd, "%1.3f", dopt_arg);
    if (sopt)
        led_printf (fd, "%s", sopt_arg);

    led_fini (fd);

    return 0;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
