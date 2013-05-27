/* zmq.c - wrapper functions for zmq prototyping */

#define _GNU_SOURCE
#include <stdio.h>
#include <zmq.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <json/json.h>
#include <assert.h>

#include "zmq.h"
#include "util.h"

/**
 ** zmq wrappers
 **/

void _zmq_close (void *socket)
{
    if (zmq_close (socket) < 0) {
        fprintf (stderr, "zmq_close: %s\n", zmq_strerror (errno));
        exit (1);
    }
}

void _zmq_term (void *ctx)
{
    if (zmq_term (ctx) < 0) {
        fprintf (stderr, "zmq_term: %s\n", zmq_strerror (errno));
        exit (1);
    }
}

void *_zmq_init (int nthreads)
{
    void *ctx;

    if (!(ctx = zmq_init (nthreads))) {
        fprintf (stderr, "zmq_init: %s\n", zmq_strerror (errno));
        exit (1);
    }
    return ctx;
}

void *_zmq_socket (void *ctx, int type)
{
    void *sock;

    if (!(sock = zmq_socket (ctx, type))) {
        fprintf (stderr, "zmq_socket(%d): %s\n", type, zmq_strerror (errno));
        exit (1);
    }
    return sock;
}

void _zmq_bind (void *sock, const char *endpoint)
{
    if (zmq_bind (sock, endpoint) < 0) {
        fprintf (stderr, "zmq_bind %s: %s\n", endpoint, zmq_strerror (errno));
        exit (1);
    }
}

void _zmq_connect (void *sock, const char *endpoint)
{
    if (zmq_connect (sock, endpoint) < 0) {
        fprintf (stderr, "zmq_connect %s: %s\n", endpoint, zmq_strerror(errno));
        exit (1);
    }
}

void _zmq_subscribe_all (void *sock)
{
    if (zmq_setsockopt (sock, ZMQ_SUBSCRIBE, NULL, 0) < 0) {
        fprintf (stderr, "zmq_setsockopt ZMQ_SUBSCRIBE: %s\n",
                 zmq_strerror (errno));
        exit (1);
    }
}

void _zmq_subscribe (void *sock, char *tag)
{
    if (zmq_setsockopt (sock, ZMQ_SUBSCRIBE, tag, tag ? strlen (tag) : 0) < 0) {
        fprintf (stderr, "zmq_setsockopt ZMQ_SUBSCRIBE: %s\n",
                 zmq_strerror (errno));
        exit (1);
    }
}

void _zmq_unsubscribe (void *sock, char *tag)
{
    if (zmq_setsockopt (sock, ZMQ_UNSUBSCRIBE, tag, strlen (tag)) < 0) {
        fprintf (stderr, "zmq_setsockopt ZMQ_UNSUBSCRIBE: %s\n",
                 zmq_strerror (errno));
        exit (1);
    }
}

void _zmq_mcast_loop (void *sock, bool enable)
{
    uint64_t val = enable ? 1 : 0;

    if (zmq_setsockopt (sock, ZMQ_MCAST_LOOP, &val, sizeof (val)) < 0) {
        fprintf (stderr, "zmq_setsockopt ZMQ_MCAST_LOOP: %s\n",
                zmq_strerror (errno));
        exit (1);
    }
}

void _zmq_msg_init_size (zmq_msg_t *msg, size_t size)
{
    if (zmq_msg_init_size (msg, size) < 0) {
        fprintf (stderr, "zmq_msg_init_size: %s\n", zmq_strerror (errno));
        exit (1);
    }
}

void _zmq_msg_init (zmq_msg_t *msg)
{
    if (zmq_msg_init (msg) < 0) {
        fprintf (stderr, "zmq_msg_init: %s\n", zmq_strerror (errno));
        exit (1);
    }
}

void _zmq_msg_close (zmq_msg_t *msg)
{
    if (zmq_msg_close (msg) < 0) {
        fprintf (stderr, "zmq_msg_close: %s\n", zmq_strerror (errno));
        exit (1);
    }
}

void _zmq_send (void *socket, zmq_msg_t *msg, int flags)
{
    if (zmq_send (socket, msg, flags) < 0) {
        fprintf (stderr, "zmq_send: %s\n", zmq_strerror (errno));
        exit (1);
    }
}

void _zmq_recv (void *socket, zmq_msg_t *msg, int flags)
{
    if (zmq_recv (socket, msg, flags) < 0) {
        fprintf (stderr, "zmq_recv: %s\n", zmq_strerror (errno));
        exit (1);
    }
}

void _zmq_getsockopt (void *socket, int option_name, void *option_value,
                      size_t *option_len)
{
    if (zmq_getsockopt (socket, option_name, option_value, option_len) < 0) {
        fprintf (stderr, "zmq_getsockopt: %s\n", zmq_strerror (errno));
        exit (1);
    }
}

bool _zmq_rcvmore (void *socket)
{
    int64_t more;
    size_t more_size = sizeof (more);

    _zmq_getsockopt (socket, ZMQ_RCVMORE, &more, &more_size);

    return (bool)more;
}

void _zmq_msg_dup (zmq_msg_t *dest, zmq_msg_t *src)
{
    _zmq_msg_init_size (dest, zmq_msg_size (src));
    memcpy (zmq_msg_data (dest), zmq_msg_data (src), zmq_msg_size (dest));
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */

