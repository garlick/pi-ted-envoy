#include <stdio.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <zmq.h>

#include "zmq.h"
#include "util.h"
#include "emon.h"
#include "encode.h"
#include "w1.h"

#define OPTIONS "tmeE"
#define HAVE_GETOPT_LONG 1

#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long (ac,av,opt,lopt,NULL)
static const struct option longopts[] = {
    { "temperature",  no_argument, 0, 't'},
    { "ted-energy",   no_argument, 0, 'e'},
    { "envoy-energy", no_argument, 0, 'E'},
    { "monitor",      no_argument, 0, 'm'},
    {0, 0, 0, 0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt (ac,av,opt)
#endif

void mon (void *zs, bool topt, bool eopt, bool Eopt, bool mopt);

void usage (void)
{
    fprintf (stderr, "Usage: emon\n");
    exit (1);
}

int main (int argc, char *argv[])
{
    int c;
    void *zctx;
    void *zs;
    bool topt = false;
    bool mopt = false;
    bool eopt = false;
    bool Eopt = false;

    while ((c = GETOPT (argc, argv, OPTIONS, longopts)) != -1) {
        switch (c) {
            case 't': /* --temp */
                topt = true;
                break;
            case 'e': /* --ted-energy */
                eopt = true;
                break;
            case 'E': /* --envoy-energy */
                Eopt = true;
                break;
            case 'm': /* --monitor */
                mopt = true;
                break;
            default:
                usage ();
        }
    }
    if (optind < argc)
        usage ();
    if (!mopt && !Eopt && !eopt && !topt)
        Eopt = eopt = topt = true;

    zctx = _zmq_init (1);
    zs = _zmq_socket (zctx, ZMQ_SUB);
    _zmq_connect (zs, PUB_URI);
    _zmq_subscribe (zs, "");

    mon (zs, topt, eopt, Eopt, mopt);

    _zmq_close (zs);
    _zmq_term (zctx);

    exit (0);
}

void mon (void *zs, bool topt, bool eopt, bool Eopt, bool mopt)
{
    int tcount = 0;
    int ecount = 0;
    int Ecount = 0;

    for (;;) {
        zmq_msg_t msg;
        char *s;

        _zmq_msg_init (&msg);
        _zmq_recv(zs, &msg, 0);
        s = xzmalloc (zmq_msg_size (&msg) + 1);
        memcpy (s, zmq_msg_data (&msg), zmq_msg_size (&msg));

        if (mopt) {
            printf ("%s\n", s); /* print undecoded JSON */
        } else {
            if (topt && tcount == 0) {
                double c, fr, fz;
                if (temp_deserialize (s, &c, &fr, &fz)) {
                    printf ("Fridge top case temp   %.1lf F\n", c2f (c));
                    printf ("Fridge temp            %.1lf F\n", c2f (fr));
                    printf ("Freezer temp           %.1lf F\n", c2f (fz));
                    tcount++;
                }
            }
            if (eopt && ecount == 0) {
                int a, c, w, v;
                if (ted_deserialize (s, &a, &c, &w, &v)) {
                    printf ("Net power from grid    %d W\n", w);
                    printf ("Line voltage           %d V\n", v);
                    ecount++;
                }
            }
            if (Eopt && Ecount == 0) {
                int l, w, d, c;
                if (envoy_deserialize (s, &l, &w, &d, &c)) {
                    printf ("Lifetime energy gen    %.1lf kW h\n", 1E-3*l);
                    printf ("weekly energy gen      %.1lf kW h\n", 1E-3*w);
                    printf ("daily energy gen       %.1lf kW h\n", 1E-3*d);
                    printf ("Generated power        %d W\n", c);
                    Ecount++;
                } 
            }
        }
        free (s);
        _zmq_msg_close (&msg); 

        if ((!topt || tcount > 0) && (!eopt || ecount > 0)
                                  && (!Eopt || Ecount > 0))
            break;
    }
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
