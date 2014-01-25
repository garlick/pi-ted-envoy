#include <stdio.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <zmq.h>

#include "zmq.h"
#include "util.h"
#include "emon.h"

#define OPTIONS ""
#define HAVE_GETOPT_LONG 1

#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long (ac,av,opt,lopt,NULL)
static const struct option longopts[] = {
    {0, 0, 0, 0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt (ac,av,opt)
#endif


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

    while ((c = GETOPT (argc, argv, OPTIONS, longopts)) != -1) {
        switch (c) {
            default:
                usage ();
        }
    }
    if (optind < argc)
        usage ();

    zctx = _zmq_init (1);
    zs = _zmq_socket (zctx, ZMQ_SUB);
    _zmq_connect (zs, PUB_URI);
    _zmq_subscribe (zs, "");

    for (;;) {
        zmq_msg_t msg;
        char *s;

        _zmq_msg_init (&msg);
        _zmq_recv(zs, &msg, 0);
        s = xzmalloc (zmq_msg_size (&msg) + 1);
        memcpy (s, zmq_msg_data (&msg), zmq_msg_size (&msg));

        fprintf (stderr, "%s\n", s);

        free (s);
        _zmq_msg_close (&msg); 
    }

    _zmq_close (zs);
    _zmq_term (zctx);

    exit (0);
}
/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */

