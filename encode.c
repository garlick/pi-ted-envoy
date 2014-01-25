#include <stdbool.h>
#include <json/json.h>
#include "util.h"
#include "encode.h"

static void add_double (json_object *o, const char *name, double x)
{
    json_object *no;

    if (!(no = json_object_new_double (x)))
        oom ();
    json_object_object_add (o, name, no);
}

static bool get_double (json_object *o, const char *name, double *xp)
{
    json_object *no = json_object_object_get (o, name);
    if (no) {
        *xp = json_object_get_double (no);
        return true;
    }
    return false;
}

static void add_int (json_object *o, const char *name, int i)
{
    json_object *no;

    if (!(no = json_object_new_int (i)))
        oom ();
    json_object_object_add (o, name, no);
}

static bool get_int (json_object *o, const char *name, int *ip)
{
    json_object *no = json_object_object_get (o, name);
    if (no) {
        *ip = json_object_get_int (no);
        return true;
    }
    return false;
}

char *temp_serialize (double c, double fr, double fz)
{
    json_object *o, *no;
    char *s;

    if (!(no = json_object_new_object ()))
        oom ();
    add_double (no, "case", c);
    add_double (no, "fridge", fr);
    add_double (no, "freezer", fz);
    if (!(o = json_object_new_object ()))
        oom ();
    json_object_object_add (o, "temp", no);
    s = xstrdup (json_object_to_json_string (o));
    json_object_put (o);
    return s;
}

bool temp_deserialize (const char *s, double *cp, double *frp, double *fzp)
{
    json_object *no, *o;
    double c, fr, fz;
    bool ret = false;

    if (!(o = json_tokener_parse (s)))
        goto done;
    if (!(no = json_object_object_get (o, "temp")))
        goto done;
    if (!get_double (no, "case", &c) || !get_double (no, "fridge", &fr)
                                     || !get_double (no, "freezer", &fz))
        goto done;
    ret = true;
    *cp = c;
    *frp = fr;
    *fzp = fz;
done:
    if (o)
        json_object_put (o);
    return ret;
}

char *ted_serialize (int a, int c, int w, int v)
{
    json_object *o, *no;
    char *s = NULL;

    if (!(no = json_object_new_object ()))
        oom ();
    add_int (no, "addr", a);
    add_int (no, "count", c);
    add_int (no, "watts", w);
    add_int (no, "volts", v);
    if (!(o = json_object_new_object ()))
        oom ();
    json_object_object_add (o, "ted", no);
    s = xstrdup (json_object_to_json_string (o));
    json_object_put (o);
    return s;
}

bool ted_deserialize (const char *s, int *ap, int *cp, int *wp, int *vp)
{
    json_object *no, *o;
    int a, c, w, v;
    bool ret = false;

    if (!(o = json_tokener_parse (s)))
        goto done;
    if (!(no = json_object_object_get (o, "ted")))
        goto done;
    if (!get_int (no, "addr", &a) || !get_int (no, "count", &c)
        || !get_int (no, "watts", &w) || !get_int (no, "volts", &v))
        goto done;
    ret = true;
    *ap = a;
    *cp = c;
    *wp = w;
    *vp = v;
done:
    if (o)
        json_object_put (o);
    return ret;
}

char *key_serialize (int n)
{
    json_object *o, *no;
    char *s = NULL;

    if (!(no = json_object_new_object ()))
        oom ();
    add_int (no, "num", n);
    if (!(o = json_object_new_object ()))
        oom ();
    json_object_object_add (o, "key", no);
    s = xstrdup (json_object_to_json_string (o));
    json_object_put (o);
    return s;
}

bool key_deserialize (const char *s, int *np)
{
    json_object *no, *o;
    int n;
    bool ret = false;

    if (!(o = json_tokener_parse (s)))
        goto done;
    if (!(no = json_object_object_get (o, "key")))
        goto done;
    if (!get_int (no, "num", &n))
        goto done;
    ret = true;
    *np = n;
done:
    if (o)
        json_object_put (o);
    return ret;
}

/* envoy is serialized in a perl script */

bool envoy_deserialize (const char *s, int *lp, int *wp, int *dp, int *cp)
{
    json_object *no, *o;
    int l, w, d, c;
    bool ret = false;

    if (!(o = json_tokener_parse (s)))
        goto done;
    if (!(no = json_object_object_get (o, "envoy")))
        goto done;
    if (!get_int (no, "lifetime_energy", &l)
        || !get_int (no, "weekly_energy", &w)
        || !get_int (no, "daily_energy", &d)
        || !get_int (no, "current_power", &c))
        goto done;
    ret = true;
    *lp = l;
    *wp = w;
    *dp = d;
    *cp = c;
done:
    if (o)
        json_object_put (o);
    return ret;
}

