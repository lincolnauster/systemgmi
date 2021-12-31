/* This should *not* be included by client code, and is for com/ internal use
 * only. */

struct tls_ctx {
	SSL_CTX *ssl_ctx;
};

struct tls_conn {
	int sock;
	SSL_CTX *ctx;
};

struct tls_str {
	/* Owned pointer to the SSL object. */
	SSL *ssl;
	/* Owned file descriptor of the socket over which ssl is operating. */
	int sok;
};
