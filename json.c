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
/* json.c - json encode/decode functions for emon messages */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <json/json.h>

#include "json.h"

int ted_serialize (char *s, int len, int addr, int count, int watts, int volts)
{
    json_object *o = NULL;
    json_object *no = NULL;
    const char *sp;

    o = json_object_new_object ();
    if (!o)
        goto nomem;

    no = json_object_new_int (addr);
    if (!no)
        goto nomem;
    json_object_object_add (o, "addr", no);

    no = json_object_new_int (count);
    if (!no)
        goto nomem;
    json_object_object_add (o, "count", no);

    no = json_object_new_int (watts);
    if (!no)
        goto nomem;
    json_object_object_add (o, "watts", no);

    no = json_object_new_int (volts);
    if (!no)
        goto nomem;
    json_object_object_add (o, "volts", no);

    /* storage for s is internal to o */
    sp = json_object_to_json_string (o);
    assert (sp != NULL);
    if (len < strlen(sp) + 1)
        goto erange;
    strcpy (s, sp);
    json_object_put (o);
    return 0;
nomem: 
    if (o)
        json_object_put (o);
    errno = ENOMEM;
    return -1;
erange:
    if (o)
        json_object_put (o);
    errno = ERANGE;
    return -1;
}

int ted_deserialize (char *s, int *addrp, int *countp, int *wattsp, int *voltsp)
{
    json_object *o = NULL;
    int addr, count, watts, volts;
    json_object_iter iter;
    int n = 0;

    o = json_tokener_parse (s);
    if (!o)
        goto inval;
    json_object_object_foreachC (o, iter) {
        if (!strcmp (iter.key, "addr")) {
            if (json_object_get_type (iter.val) != json_type_int)
                goto inval;
            addr = json_object_get_int (iter.val);
            n++;
        } else if (!strcmp (iter.key, "count")) {
            if (json_object_get_type (iter.val) != json_type_int)
                goto inval;
            count = json_object_get_int (iter.val);
            n++;
        } else if (!strcmp (iter.key, "watts")) {
            if (json_object_get_type (iter.val) != json_type_int)
                goto inval;
            watts = json_object_get_int (iter.val);
            n++;
        } else if (!strcmp (iter.key, "volts")) {
            if (json_object_get_type (iter.val) != json_type_int)
                goto inval;
            volts = json_object_get_int (iter.val);
            n++;
        }
    }
    if (n != 4)
        goto inval;
    *addrp = addr;
    *countp = count;
    *wattsp = watts;
    *voltsp = volts;
    json_object_put (o);
    return 0;
inval:
    if (o)
        json_object_put (o);
    errno = EINVAL;
    return -1;
}

int envoy_deserialize (char *s, int *lp, int *wp, int *dp, int *cp)
{
    json_object *o = NULL;
    int l, w, d, c;
    json_object_iter iter;
    int n = 0;

    o = json_tokener_parse (s);
    if (!o)
        goto inval;
    json_object_object_foreachC (o, iter) {
        if (!strcmp (iter.key, "lifetime_energy")) {
            if (json_object_get_type (iter.val) != json_type_int)
                goto inval;
            l = json_object_get_int (iter.val);
            n++;
        } else if (!strcmp (iter.key, "weekly_energy")) {
            if (json_object_get_type (iter.val) != json_type_int)
                goto inval;
            w = json_object_get_int (iter.val);
            n++;
        } else if (!strcmp (iter.key, "daily_energy")) {
            if (json_object_get_type (iter.val) != json_type_int)
                goto inval;
            d = json_object_get_int (iter.val);
            n++;
        } else if (!strcmp (iter.key, "current_power")) {
            if (json_object_get_type (iter.val) != json_type_int)
                goto inval;
            c = json_object_get_int (iter.val);
            n++;
        }
    }
    if (n != 4)
        goto inval;
    *lp = l;
    *wp = w;
    *dp = d;
    *cp = c;
    json_object_put (o);
    return 0;
inval:
    if (o)
        json_object_put (o);
    errno = EINVAL;
    return -1;
}

#if 0
int main(int argc, char **argv)
{
    char *s;
    int addr, count, watts, volts;

    s = ted_serialize (1, 2, 4034, 122);
    if (!s) {
        perror ("ted_serialize");
        exit (1);
    }
    printf("%s\n", s);
    if (ted_deserialize (s, &addr, &count, &watts, &volts) < 0) {
        perror ("ted_deserialize");
        exit (1);
    }
    assert (addr == 1);
    assert (count == 2);
    assert (watts == 4034);
    assert (volts == 122);

    return 0;
}
#endif

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */

