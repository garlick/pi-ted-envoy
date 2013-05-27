void _zmq_close (void *socket);
void _zmq_term (void *ctx);
void *_zmq_init (int nthreads);
void *_zmq_socket (void *ctx, int type);
void _zmq_bind (void *sock, const char *endpoint);
void _zmq_connect (void *sock, const char *endpoint);
void _zmq_subscribe (void *sock, char *tag);
void _zmq_subscribe_all (void *sock);
void _zmq_unsubscribe (void *sock, char *tag);
void _zmq_msg_init_size (zmq_msg_t *msg, size_t size);
void _zmq_msg_init (zmq_msg_t *msg);
void _zmq_msg_close (zmq_msg_t *msg);
void _zmq_send (void *socket, zmq_msg_t *msg, int flags);
void _zmq_recv (void *socket, zmq_msg_t *msg, int flags);
void _zmq_getsockopt (void *socket, int option_name, void *option_value,
                      size_t *option_len);
bool _zmq_rcvmore (void *socket);
void _zmq_msg_dup (zmq_msg_t *dest, zmq_msg_t *src);
void _zmq_mcast_loop (void *sock, bool enable);
