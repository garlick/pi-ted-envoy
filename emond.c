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
/* emond.c - energy monitor daemon */

/* Get data from Envoy PV controller and TED 1001 (with serial hack),
 * display data on i2c OLED.
 * Listen for JSON from envoy_scrape.pl run by cron job on zs_envoy 0MQ socket.
 * listen for JSON from TED thread on zs_ted 0MQ socket.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <zmq.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <getopt.h>
#include <unistd.h>

#define WITH_ZTLED  0

#include "oled.h"
#if WITH_ZTLED
#include "led.h"
#endif
#include "json.h"
#include "util.h"
#include "zmq.h"
#include "ted.h"

#define TED_URI     "inproc://ted"
#define ENVOY_URI   "ipc:///tmp/emond"

#define TED_PORT    "/dev/ttyAMA0"

#define OLED_ADDR   0x28
#define LED_ADDR    0x27

typedef struct {
    void *zctx;
    void *zs_ted;
    void *zs_envoy;
    int oled;
#if WITH_ZTLED
    int led;
#endif
    int envoy_current_power;
    int envoy_daily_energy;
    int envoy_weekly_energy;
    int envoy_lifetime_energy;
    struct timeval envoy_last;
    int ted_addr;
    int ted_count;
    int ted_watts;
    int ted_volts;
    struct timeval ted_last;
} server_t;

typedef struct {
    void *zs_ted;
    pthread_t t;
} tedctx_t;

const struct timeval ted_stale = { .tv_sec = 30, .tv_usec = 0 };
const struct timeval envoy_stale = { .tv_sec = 600, .tv_usec = 0 };
static server_t *ctx;
static tedctx_t *tctx;

#define OPTIONS "fd"
#define HAVE_GETOPT_LONG 1

#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long (ac,av,opt,lopt,NULL)
static const struct option longopts[] = {
    {"foreground",      no_argument,        0, 'f'},
    {"debug",           no_argument,        0, 'd'},
    {0, 0, 0, 0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt (ac,av,opt)
#endif

static void usage (void)
{
    fprintf (stderr,
"Usage: emond [OPTIONS]\n"
"   -f,--foreground    do not fork and diassociate with tty\n"
"   -d,--debug         show messages on stderr\n"
    );
    exit (1);
}

static void *_ted_thread (void *arg)
{
    char buf[256];
    zmq_msg_t msg;
    int addr, count, volts, watts;

    if (ted_init ("/dev/ttyAMA0") < 0) {
        fprintf (stderr, "_ted_thread: /dev/ttyAMA0: %s", strerror (errno));
        exit (1);
    }

    for (;;) {
        if (ted_read (&addr, &count, &volts, &watts) < 0) {
            if (errno != EINVAL) {
                perror ("ted_read");
                exit (1);
            }
            //fprintf (stderr, "bad packet\n");
            continue;
        }
        if (ted_serialize (buf, sizeof (buf), addr, count, volts, watts) < 0) {
            //perror ("ted_serialize");
            continue;
        }
        _zmq_msg_init_size (&msg, strlen (buf));
        memcpy (zmq_msg_data (&msg), buf, strlen (buf));
        _zmq_send (tctx->zs_ted, &msg, 0);
    }

    ted_fini ();
    return NULL;
}

static void _ted_init (void)
{
    int err;

    tctx = xzmalloc (sizeof (*tctx));
    tctx->zs_ted = _zmq_socket (ctx->zctx, ZMQ_PUSH);
    _zmq_connect (tctx->zs_ted, TED_URI);

    err = pthread_create (&tctx->t, NULL, _ted_thread, NULL);
    if (err) {
        fprintf (stderr, "pthread_create: %s\n", strerror (err));
        exit (1);
    }
}

static void _ted_fini (void)
{
    /* FIXME */
}

static void _server_init (void)
{
    ctx = xzmalloc (sizeof (*ctx));

    ctx->zctx = _zmq_init (1);
    ctx->zs_envoy = _zmq_socket (ctx->zctx, ZMQ_PULL);
    _zmq_bind (ctx->zs_envoy, ENVOY_URI);

    ctx->zs_ted = _zmq_socket (ctx->zctx, ZMQ_PULL);
    _zmq_bind (ctx->zs_ted, TED_URI);

    _ted_init ();
#if WITH_ZTLED
    ctx->led = led_init (LED_ADDR);
    led_printf (ctx->led, "----"); 
#endif
    ctx->oled = oled_init (OLED_ADDR);
    oled_clear (ctx->oled);
}

static void _server_fini (void)
{
#if WITH_ZTLED
    led_fini (ctx->led);
#endif
    oled_fini (ctx->oled);
    _ted_fini ();

    _zmq_close (ctx->zs_ted);
    _zmq_close (ctx->zs_envoy);

    _zmq_term (ctx->zctx);

    free (ctx);
}

static void _read_envoy (int dopt)
{
    zmq_msg_t msg;
    char *s;

    _zmq_msg_init (&msg);
    _zmq_recv(ctx->zs_envoy, &msg, 0);
    s = xzmalloc (zmq_msg_size (&msg) + 1);
    memcpy (s, zmq_msg_data (&msg), zmq_msg_size (&msg));
    if (dopt)
        fprintf (stderr, "_read_envoy: %s\n", s);
    (void) envoy_deserialize (s, &ctx->envoy_lifetime_energy,
                                 &ctx->envoy_weekly_energy,
                                 &ctx->envoy_daily_energy,
                                 &ctx->envoy_current_power);
    free (s);
    _zmq_msg_close (&msg);
    xgettimeofday (&ctx->envoy_last, NULL);
}

static void _read_ted (int dopt)
{
    zmq_msg_t msg;
    char *s;

    _zmq_msg_init (&msg);
    _zmq_recv(ctx->zs_ted, &msg, 0);
    s = xzmalloc (zmq_msg_size (&msg) + 1);
    memcpy (s, zmq_msg_data (&msg), zmq_msg_size (&msg));
    if (dopt)
        fprintf (stderr, "_read_ted: %s\n", s);
    (void) ted_deserialize (s, &ctx->ted_addr, &ctx->ted_count,
                            &ctx->ted_watts, &ctx->ted_volts);
    free (s);
    _zmq_msg_close (&msg);
    xgettimeofday (&ctx->ted_last, NULL);
}

static void _update_display (void)
{
    int current_usage = ctx->ted_watts + ctx->envoy_current_power;
    struct timeval now, t;
    bool tstale = false, estale = false;

    xgettimeofday (&now, NULL);
    timersub (&now, &ctx->ted_last, &t);
    if (timercmp (&t, &ted_stale, >))
        tstale =true;
    timersub (&now, &ctx->envoy_last, &t);
    if (timercmp (&t, &envoy_stale, >))
        estale =true;

    oled_clear (ctx->oled);

    oled_text_pos_set (ctx->oled, 0, 0);
    oled_printf (ctx->oled, "gen %+0.3f kW%s",
              (float)ctx->envoy_current_power / 1000.0, estale ? "*" : " ");

    oled_text_pos_set (ctx->oled, 0, 1);
    oled_printf (ctx->oled, "use %+0.3f kW%s",
              -1.0*(float)current_usage / 1000.0, tstale ? "*" : " ");


    oled_text_pos_set (ctx->oled, 0, 4);
    oled_printf (ctx->oled, "day %+0.3f kWh%s",
              (float)ctx->envoy_daily_energy / 1000.0, estale ? "*" : " ");
#if WITH_ZTLED
    led_printf (ctx->led, "%+0.3f", -1.0*(float)ctx->ted_watts / 1000.0);
#endif
}

static void _poll (int dopt)
{
    zmq_pollitem_t zpa[] = {
{ .socket = ctx->zs_envoy,      .events = ZMQ_POLLIN, .revents = 0, .fd = -1 },
{ .socket = ctx->zs_ted,        .events = ZMQ_POLLIN, .revents = 0, .fd = -1 },
    };
    long tmout = 60*1000000; /* 60s */
    int rc;

    if ((rc = zmq_poll (zpa, 2, tmout)) < 0) {
        fprintf (stderr, "zmq_poll: %s\n", zmq_strerror (errno));
        exit (1);
    }
    if (rc > 0) {
        if (zpa[0].revents & ZMQ_POLLIN)
            _read_envoy (dopt);
        if (zpa[1].revents & ZMQ_POLLIN)
            _read_ted (dopt);
    }
    _update_display ();
}

int main (int argc, char *argv[])
{
    int c;
    int fopt = 0;
    int dopt = 0;

    while ((c = GETOPT (argc, argv, OPTIONS, longopts)) != -1) {
        switch (c) {
            case 'f':
                fopt = 1;
                break;
            case 'd':
                dopt = 1;
                break;
            default:
                usage ();
        }
    }

    if (!fopt) {
        if (daemon(0, 1) < 0) {
            perror ("daemon");
            exit (1);
        }
    }
    _server_init ();
    for (;;)
        _poll (dopt);
    _server_fini ();
    return 0;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
