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

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */

