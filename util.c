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
/* util.c - misc utility functions */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "util.h"

void oom (void)
{
    fprintf (stderr, "out of memory\n");
    exit (1);
}

void *xzmalloc (size_t size)
{
    void *new;

    new = malloc (size);
    if (!new)
        oom ();
    memset (new, 0, size);
    return new;
}

char *xstrdup (const char *s)
{
    char *cpy = strdup (s);
    if (!cpy)
        oom ();
    return cpy;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
