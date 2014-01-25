/*****************************************************************************
 *  Copyright (C) 2013-14 Jim Garlick
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
 * display data on i2c OLED/LEDs.
 * Listen for JSON from envoy_scrape.pl run by cron job on zs_envoy 0MQ socket.
 * Listen for JSON from TED, temp, and key threads on zs_other 0MQ socket.
 */

#include <sys/types.h>
#include <sys/stat.h>
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
#include <json/json.h>
#include <math.h>

#include "oled.h"
#include "led.h"
#include "util.h"
#include "zmq.h"
#include "ted.h"
#include "gpio.h"
#include "w1.h"
#include "encode.h"
#include "emon.h"

#define OTHER_URI       "inproc://other"
#define ENVOY_URI       "ipc:///tmp/emond"

#define SER_TED        "/dev/ttyAMA0"

#define I2C_W1          0x18 /* not used here, for doc only */
#define I2C_OLED        0x28
#define I2C_LED_A       0x30
#define I2C_LED_B       0x27

#define W1_TEMP_CASE    "28-000002bf1574"
#define W1_TEMP_FRIDGE  "28-0000059d3842"
#define W1_TEMP_FREEZER "28-0000059dec96"

#define GPIO_MODE_PIN   27

typedef enum { MODE_POWER, MODE_TEMP } dispmode_t;

typedef struct {
    void *zs_other;
    pthread_t t;
} thdctx_t;

typedef struct {
    /* ZeroMQ context and sockets
     */
    void *zctx;
    void *zs_other;
    void *zs_envoy;
    void *zs_pub;
    /* file descriptors for I2C devices
     */
    int oled;
    int led_a;
    int led_b;
    /* most recent data obtained from envoy
     */
    int envoy_current_power;
    int envoy_daily_energy;
    int envoy_weekly_energy;
    int envoy_lifetime_energy;
    time_t envoy_last;
    /* most recent data obtained from ted
     */
    int ted_addr;
    int ted_count;
    int ted_watts;
    int ted_volts;
    time_t ted_last;
    /* most recent temp data
     */
    double temp_case;
    double temp_fridge;
    double temp_freezer;
    /* misc
     */
    int wattsec;                        /* energy used since midnight */
    thdctx_t kctx;                      /* key thread state */
    thdctx_t pctx;                      /* TED thread state */
    thdctx_t Tctx;                      /* temp thread state */
    dispmode_t mode;                    /* display mode */
} server_t;

const int ted_stale = 30;       /* sec */
const int envoy_stale = 600;    /* sec */

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

/* Wait for momentary, active-low switch to be activated,
 * then send key message on thread socket.
 */
static void *key_thread (void *arg)
{
    thdctx_t *tctx = (thdctx_t *)arg;
    zmq_msg_t msg;
    char *s;

    for (;;) {
        gpio_keypress (GPIO_MODE_PIN, 0);
        s = key_serialize (GPIO_MODE_PIN);
        _zmq_msg_init_size (&msg, strlen (s));
        memcpy (zmq_msg_data (&msg), s, strlen (s));
        _zmq_send (tctx->zs_other, &msg, 0);
        free (s);
    }
    return NULL;
}

static void key_thread_init (server_t *ctx)
{
    int err;

    ctx->kctx.zs_other = _zmq_socket (ctx->zctx, ZMQ_PUSH);
    _zmq_connect (ctx->kctx.zs_other, OTHER_URI);

    err = pthread_create (&ctx->kctx.t, NULL, key_thread, &ctx->kctx);
    if (err) {
        fprintf (stderr, "pthread_create: %s\n", strerror (err));
        exit (1);
    }
}

/* Read TED samples from serial port and retransmit them on thread socket
 * as JSON messages.
 */
static void *ted_thread (void *arg)
{
    thdctx_t *tctx = (thdctx_t *)arg;
    zmq_msg_t msg;
    int addr, count, volts, watts;
    char *s;

    if (ted_init (SER_TED) < 0) {
        fprintf (stderr, "ted_thread: %s: %s\n", SER_TED, strerror (errno));
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
        s = ted_serialize (addr, count, volts, watts);
        _zmq_msg_init_size (&msg, strlen (s));
        memcpy (zmq_msg_data (&msg), s, strlen (s));
        _zmq_send (tctx->zs_other, &msg, 0);
        free (s);
    }

    ted_fini ();
    return NULL;
}

static void ted_thread_init (server_t *ctx)
{
    int err;

    ctx->pctx.zs_other = _zmq_socket (ctx->zctx, ZMQ_PUSH);
    _zmq_connect (ctx->pctx.zs_other, OTHER_URI);

    err = pthread_create (&ctx->pctx.t, NULL, ted_thread, &ctx->pctx);
    if (err) {
        fprintf (stderr, "pthread_create: %s\n", strerror (err));
        exit (1);
    }
}

static void *temp_thread (void *arg)
{
    thdctx_t *tctx = (thdctx_t *)arg;
    zmq_msg_t msg;
    char *s;

    while (1) {
        s = temp_serialize (w1_therm_get (W1_TEMP_CASE),
                            w1_therm_get (W1_TEMP_FRIDGE),
                            w1_therm_get (W1_TEMP_FREEZER));
        _zmq_msg_init_size (&msg, strlen (s));
        memcpy (zmq_msg_data (&msg), s, strlen (s));
        _zmq_send (tctx->zs_other, &msg, 0);
        free (s);
        sleep (10);
    }
    return NULL;
}

static void temp_thread_init (server_t *ctx)
{
    int err;

    ctx->Tctx.zs_other = _zmq_socket (ctx->zctx, ZMQ_PUSH);
    _zmq_connect (ctx->Tctx.zs_other, OTHER_URI);

    err = pthread_create (&ctx->Tctx.t, NULL, temp_thread, &ctx->Tctx);
    if (err) {
        fprintf (stderr, "pthread_create: %s\n", strerror (err));
        exit (1);
    }
}

static server_t *server_init (void)
{
    server_t *ctx = xzmalloc (sizeof (*ctx));

    ctx->mode = MODE_POWER;

    umask (777);

    ctx->zctx = _zmq_init (1);
    ctx->zs_envoy = _zmq_socket (ctx->zctx, ZMQ_PULL);
    _zmq_bind (ctx->zs_envoy, ENVOY_URI);
    ctx->zs_pub = _zmq_socket (ctx->zctx, ZMQ_PUB);
    _zmq_bind (ctx->zs_pub, PUB_URI);
    ctx->zs_other = _zmq_socket (ctx->zctx, ZMQ_PULL);
    _zmq_bind (ctx->zs_other, OTHER_URI);

    ctx->led_a = led_init (I2C_LED_A);
    led_sleep_set (ctx->led_a, 0);
    led_brightness_set (ctx->led_a, 0x20);

    ctx->led_b = led_init (I2C_LED_B);
    led_sleep_set (ctx->led_b, 0);
    led_brightness_set (ctx->led_b, 0x20);

    ctx->oled = oled_init (I2C_OLED);
    oled_clear (ctx->oled);

    ted_thread_init (ctx);
    key_thread_init (ctx);
    temp_thread_init (ctx);

    return ctx;
}

static void server_fini (server_t *ctx)
{
    led_fini (ctx->led_b);
    led_fini (ctx->led_a);
    oled_fini (ctx->oled);

    _zmq_close (ctx->zs_other);
    _zmq_close (ctx->zs_pub);
    _zmq_close (ctx->zs_envoy);
    _zmq_term (ctx->zctx);

    free (ctx);
}

/* Message is ready on socket that Envoy perl script transmits on.
 * Read it and update envoy sample data in the server context.
 */
static void read_envoy (server_t *ctx, int dopt)
{
    zmq_msg_t msg;
    char *s;

    _zmq_msg_init (&msg);
    _zmq_recv(ctx->zs_envoy, &msg, 0);
    s = xzmalloc (zmq_msg_size (&msg) + 1);
    memcpy (s, zmq_msg_data (&msg), zmq_msg_size (&msg));
    if (dopt)
        fprintf (stderr, "%s\n", s);
    if (envoy_deserialize (s, &ctx->envoy_lifetime_energy,
                                 &ctx->envoy_weekly_energy,
                                 &ctx->envoy_daily_energy,
                                 &ctx->envoy_current_power))
        ctx->envoy_last = time (NULL);
    free (s);
    if (ctx->zs_pub) {
        _zmq_send (ctx->zs_pub, &msg, 0);
    } else
        _zmq_msg_close (&msg);
}

/* Message is ready on socket that threads transmit on.
 * If TED, update TED sample data in the server context and recalc wattsec.
 * If key, switch mode
 * If temp, update temp sample data in the server
 */
static void read_other (server_t *ctx, int dopt)
{
    zmq_msg_t msg;
    char *s;
    time_t now = time (NULL);
    struct tm tm_now, tm_last;
    int key;

    _zmq_msg_init (&msg);
    _zmq_recv(ctx->zs_other, &msg, 0);
    s = xzmalloc (zmq_msg_size (&msg) + 1);
    memcpy (s, zmq_msg_data (&msg), zmq_msg_size (&msg));
    if (dopt)
        fprintf (stderr, "%s\n", s);
    if (key_deserialize (s, &key)) {
        switch (ctx->mode) {
            case MODE_TEMP:
                ctx->mode = MODE_POWER;
                break;
            case MODE_POWER:
                ctx->mode = MODE_TEMP;
                break;
        }
        goto done;
    }
    if (temp_deserialize (s, &ctx->temp_case, &ctx->temp_fridge,
                             &ctx->temp_freezer))
        goto done;
    if (ted_deserialize (s, &ctx->ted_addr,  &ctx->ted_count,
                            &ctx->ted_watts, &ctx->ted_volts)) {
        /* N.B. although we notice if envoy or TED values are stale and
         * try to display this, the wattsec value could be innacurate if TED
         * readings are missed or Envoy scrape is not working for some time
         * during the day.
         */
        if (ctx->ted_last > 0 && ctx->envoy_last > 0) {
            localtime_r (&now, &tm_now);
            localtime_r (&ctx->ted_last, &tm_last);
            if (tm_now.tm_hour == 0 && tm_last.tm_hour == 23) /* reset @midnight */
                ctx->wattsec = 0;
            ctx->wattsec += (now - ctx->ted_last)
                          * (ctx->ted_watts + ctx->envoy_current_power);
        }
        ctx->ted_last = now;
        goto done;
    }
done:
    free (s);
    if (ctx->zs_pub)
        _zmq_send (ctx->zs_pub, &msg, 0);
    else
        _zmq_msg_close (&msg);
}

static void update_display (server_t *ctx)
{
    time_t now = time (NULL);
    bool tstale = (now - ctx->ted_last > ted_stale);
    bool estale = (now - ctx->envoy_last > envoy_stale);

    oled_clear (ctx->oled);

    oled_text_pos_set (ctx->oled, 0, 0);
    oled_printf (ctx->oled, "DAILY ENERGY");

    oled_text_pos_set (ctx->oled, 0, 1);
    oled_printf (ctx->oled, "gen %-2.3f kWh%s",
                 (float)ctx->envoy_daily_energy / 1000.0, estale ? "*" : " ");

    oled_text_pos_set (ctx->oled, 0, 2);
    oled_printf (ctx->oled, "use %-2.3f kWh%s",
                 (float)ctx->wattsec / (1000*60*60),
                 (tstale || estale) ? "*" : " ");

    oled_text_pos_set (ctx->oled, 0, 4);
    oled_printf (ctx->oled, "TED %dW %dV%s", ctx->ted_watts, ctx->ted_volts,
                 tstale ? "*" : " ");

    if (ctx->mode == MODE_POWER) {
        /* LED A: gen */
        if (estale)
            led_printf (ctx->led_a, "----"); 
        else
            led_printf (ctx->led_a, "%0.3f",
                (float)ctx->envoy_current_power / 1000.0);

        /* LED B: use */
        if (tstale || estale)
            led_printf (ctx->led_b, "----"); 
        else
            led_printf (ctx->led_b, "%0.3f",
            (float)(ctx->ted_watts + ctx->envoy_current_power) / 1000);
    } else if (ctx->mode == MODE_TEMP) {
        /* LED A: fridge */
        if (ctx->temp_fridge == NAN)
            led_printf (ctx->led_a, "----"); 
        else
            led_printf (ctx->led_a, "%0.1lf", c2f (ctx->temp_fridge));

        /* LED B: freezer */
        if (ctx->temp_freezer == NAN)
            led_printf (ctx->led_b, "----"); 
        else
            led_printf (ctx->led_b, "%0.1lf", c2f (ctx->temp_freezer));
    }
}

static void mypoll (server_t *ctx, int dopt)
{
    zmq_pollitem_t zpa[] = {
{ .socket = ctx->zs_envoy,      .events = ZMQ_POLLIN, .revents = 0, .fd = -1 },
{ .socket = ctx->zs_other,        .events = ZMQ_POLLIN, .revents = 0, .fd = -1 },
    };
    long tmout = 60*1000000; /* 60s */
    int rc;

    if ((rc = zmq_poll (zpa, 2, tmout)) < 0) {
        fprintf (stderr, "zmq_poll: %s\n", zmq_strerror (errno));
        exit (1);
    }
    if (rc > 0) {
        if (zpa[0].revents & ZMQ_POLLIN)
            read_envoy (ctx, dopt);
        if (zpa[1].revents & ZMQ_POLLIN)
            read_other (ctx, dopt);
    }
    update_display (ctx);
}

int main (int argc, char *argv[])
{
    int c;
    int fopt = 0;
    int dopt = 0;
    server_t *ctx;

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
    ctx = server_init ();
    for (;;)
        mypoll (ctx, dopt);
    server_fini (ctx);
    return 0;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
