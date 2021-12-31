/* gemini communication primitives
 *
 * This provides a wrapper of openssl to wrap communication over a TCP
 * connection with TLS.
 */

enum COM_ERR {
	COM_E_NOMEM, COM_E_OPENSSL_FAILURE, COM_E_NO_KEY, COM_E_NO_CERT,
	COM_E_BAD_KEY, COM_E_BAD_CERT, COM_E_CANT_MAKE_SOCKET, COM_E_CANT_BIND,
	COM_E_CANT_LISTEN,
};

/* An opaque handle for a TLS context. */
struct tls_ctx;

/* An object encapsulating a stream that may be read from and written to. */
struct tls_str;

/* An opaque handle for dispatching connections off of a TLS connection. */
struct tls_conn;

/* Store a tls_ctx or an error. If ok == NULL, then an error was encountered.
 * This has similar ergonomics (or lack thereof) to Go's error handling. */
struct fallible_tls_ctx {
	struct tls_ctx *ok;
	enum COM_ERR err;
};

struct fallible_tls_conn {
	struct tls_conn *ok;
	enum COM_ERR err;
};

struct listen_conf {
	struct tls_conn *con;
	void (*callback)(struct tls_str *);
};

struct fallible_tls_ctx setup_tls(void);
void free_tls_ctx(struct tls_ctx *);
struct fallible_tls_conn com_bind(struct tls_ctx *, int port);
void free_tls_conn(struct tls_conn *);

int  com_write(struct tls_str *, const char *msg);
char *com_read(struct tls_str *, int len);

/* Close an SSL connection. This includes free()'ing all owned heap objects,
 * sending close_notify, and closing the fd associated with the socket. */
void com_close(struct tls_str *);

/* Given a bound connection, dispatch each stream to a callback. This
 * runs in a new thread, but runs each call of the callback *on that main
 * thread, in a blocking manner*, making the callback responsible for handling
 * concurrency. It also has ownership of the stream, and is responsible for
 * releasing resources. */
pthread_t com_listen(struct listen_conf *);

const char *str_err(enum COM_ERR);
