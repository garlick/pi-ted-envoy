/*****************************************************************************
 *  Copyright (C) 2013 Jim Garlick
 *  Written by Jim Garlick <garlick.jim@gmail.com>
 *  All Rights Reserved.
 *
 *  This file is part of pi-gpio-uinput.
 *  For details, see <https://github.com/garlick/pi-gpio-uinput>
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
/* gpio.c - raspberry pi general purpose io functions */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <linux/input.h> /* for KEY_ definitions only */

#include "gpio.h"

#ifndef PATH_MAX
#define PATH_MAX    1024
#endif
#define INTBUFLEN   16

#define DEBOUNCE_MS 5

static void _export (int pin)
{
    struct stat sb;
    char path[PATH_MAX];
    char msg[INTBUFLEN];
    FILE *fp;

    snprintf (path, sizeof (path), "/sys/class/gpio/gpio%d", pin);
    if (stat (path, &sb) == 0)
        return;
    snprintf (path, sizeof (path), "/sys/class/gpio/export");
    fp = fopen (path, "w");
    if (!fp) {
        perror (path);
        exit (1);
    }
    snprintf (msg, sizeof (msg), "%d", pin);
    if (fputs (msg, fp) < 0) {
        perror (path);
        exit (1);
    }
    if (fclose (fp) != 0) {
        perror (path);
        exit (1);
    }
}

static void _unexport (int pin)
{
    struct stat sb;
    char path[PATH_MAX];
    char msg[INTBUFLEN];
    FILE *fp;

    snprintf (path, sizeof (path), "/sys/class/gpio/gpio%d", pin);
    if (stat (path, &sb) < 0)
        return;
    snprintf (path, sizeof (path), "/sys/class/gpio/unexport");
    fp = fopen (path, "w");
    if (!fp) {
        perror (path);
        exit (1);
    }
    snprintf (msg, sizeof (msg), "%d", pin);
    if (fputs (msg, fp) < 0) {
        perror (path);
        exit (1);
    }
    if (fclose (fp) != 0) {
        perror (path);
        exit (1);
    }
}

static void _direction (int pin, const char *direction)
{
    char path[PATH_MAX];
    FILE *fp;

    snprintf (path, sizeof (path), "/sys/class/gpio/gpio%d/direction", pin);
    fp = fopen (path, "w");
    if (!fp) {
        perror (path);
        exit (1);
    }
    if (fputs (direction, fp) < 0) {
        perror (path);
        exit (1);
    }
    if (fclose (fp) != 0) {
        perror (path);
        exit (1);
    }
}

static void _edge (int pin, char *edge)
{
    char path[PATH_MAX];
    FILE *fp;

    snprintf (path, sizeof (path), "/sys/class/gpio/gpio%d/edge", pin);
    fp = fopen (path, "w");
    if (!fp) {
        perror (path);
        exit (1);
    }
    if (fputs (edge, fp) < 0) {
        perror (path);
        exit (1);
    }
    if (fclose (fp) != 0) {
        perror (path);
        exit (1);
    }
}

static int _read_value (int fd)
{
    char c;

    if (lseek (fd, 0, SEEK_SET) < 0) {
        perror ("lseek");
        exit (0);
    }
    if (read (fd, &c, 1) != 1) {
        perror ("read");
        exit (0);
    }
    return c == '0' ? 0 : 1;
}

static int _open_value (int pin, int flags)
{
    char path[PATH_MAX];
    int fd;

    snprintf (path, sizeof (path), "/sys/class/gpio/gpio%d/value", pin);
    fd = open (path, flags);
    if (fd < 0) {
        perror (path);
        exit (1);
    }
    return fd;
}

static void _write_value (int fd, int val)
{
    char c = val ? '1' : '0';

    if (lseek (fd, 0, SEEK_SET) < 0) {
        perror ("lseek");
        exit (0);
    }
    if (write (fd, &c, 1) != 1) {
        perror ("write");
        exit (0);
    }
}

void gpio_pin_pulse (int pin, int msec, int active_value)
{
    int fd;

    _export (pin);
    _direction (pin, "out");
    fd = _open_value (pin, O_RDWR);
    _write_value (fd, active_value);
    usleep (msec * 1000);
    _write_value (fd, !active_value);
    close (fd);
    _unexport (pin);
}

void gpio_keypress (int pin, int active_value)
{
    struct pollfd pfd[1];
    int val, fd;

    memset (pfd, 0, sizeof (pfd));

    _export (pin);
    _direction (pin, "in");
    _edge (pin, "both");
    fd = _open_value (pin, O_RDONLY);
    val = _read_value (fd);

    do {
        pfd[0].fd = fd;
        pfd[0].revents = 0;
        pfd[0].events = POLLPRI;

        if (poll (pfd, 1, -1) < 0) {
            perror ("poll");
            exit (1);
        }

        usleep (1000 * DEBOUNCE_MS);
        val = _read_value (fd); 
    } while (val != active_value);

    close (fd);
    _unexport (pin);
}        

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
