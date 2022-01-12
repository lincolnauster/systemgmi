#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <openssl/ssl.h>

#include "com.h"
#include "../log/log.h"

#include "types.h"

static void com_listen_inner(struct listen_conf *arg);
static SSL_CTX *basic_openssl_context(void);

struct fallible_tls_ctx
setup_tls(void)
{
	SSL_library_init();

	struct fallible_tls_ctx f;
	struct tls_ctx *s;
	char *key_loc, *cert_loc;

	SSL_CTX *ssl_ctx;

	f.ok = NULL;

	s = malloc(sizeof(struct tls_ctx));
	if (!s) {
		f.err = COM_E_NOMEM;
		goto err_out;
	}

	ssl_ctx = basic_openssl_context();
	if (!ssl_ctx) {
		f.err = COM_E_OPENSSL_FAILURE;
		goto err_out;
	}

	s->ssl_ctx = ssl_ctx;

	cert_loc = getenv("SYSTEMGMI_TLS_CERT");
	if (!cert_loc) {
		f.err = COM_E_NO_CERT;
		goto err_out;
	}

	key_loc = getenv("SYSTEMGMI_TLS_KEY");
	if (!key_loc) {
		f.err = COM_E_NO_KEY;
		goto err_out;
	}

	if (SSL_CTX_use_certificate_file(
		ssl_ctx, cert_loc, SSL_FILETYPE_PEM
	) <= 0) {
		f.err = COM_E_BAD_CERT;
		goto err_out;
	}

	if (SSL_CTX_use_PrivateKey_file(
		ssl_ctx, key_loc, SSL_FILETYPE_PEM
	) <= 0) {
		f.err = COM_E_BAD_KEY;
		goto err_out;
	}

	f.ok = s;
	goto out;
err_out:
	free(s);
out:
	return f;
}

void
free_tls_ctx(struct tls_ctx *x)
{
	SSL_CTX_free(x->ssl_ctx);
	free(x);
}

struct fallible_tls_conn
com_bind(struct tls_ctx *ctx, int port)
{
	struct fallible_tls_conn f;
	struct tls_conn *c;
	int s;
	struct sockaddr_in saddr;

	f.ok = NULL;

	c = malloc(sizeof(struct tls_conn));
	c->ctx = ctx->ssl_ctx;

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		f.err = COM_E_CANT_MAKE_SOCKET;
		goto err_out;
	}

	int true = 1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &true, sizeof(true));

	c->sock = s;

	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(s, (struct sockaddr *) &saddr, sizeof(saddr)) < 0) {
		f.err = COM_E_CANT_BIND;
		goto err_out;
	}

	if (listen(s, 1) < 0) {
		f.err = COM_E_CANT_LISTEN;
		goto err_out;
	}

	f.ok = c;
	goto out;
err_out:
	free(c);
out:
	return f;
}

void
free_tls_conn(struct tls_conn *c)
{
	close(c->sock);
	free(c);
}

pthread_t
com_listen(struct listen_conf *arg)
{
	pthread_t thread;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_create(
		&thread, &attr, (void *(*)(void *)) com_listen_inner, arg
	);

	return thread;
}

static void
com_listen_inner(struct listen_conf *arg)
{
	int client;
	struct sockaddr_in saddr;
	struct tls_str *stream;
	SSL *ssl;

	struct tls_conn *c = arg->con;
	void (*callback)(struct tls_str *) = arg->callback;

	unsigned int len = sizeof(saddr);

	while (1) {
		client = accept(c->sock, (struct sockaddr *) &saddr, &len);
		if (client < 0) {
			log_error("Couldn't accept a connection.");
			continue;
		}

		ssl = SSL_new(c->ctx);
		SSL_set_fd(ssl, client);

		// TODO: nice errors here. With whom did it fail? How did it
		// fail? etc
		if (SSL_accept(ssl) <= 0) log_error("TLS handshake failed.");
		else {
			stream = malloc(sizeof(struct tls_str));
			stream->ssl = ssl;
			stream->sok = client;
			callback(stream);
		}
	}
}

int
com_write(struct tls_str *s, const char *msg)
{
	return SSL_write(s->ssl, msg, strlen(msg));
}

void
com_close(struct tls_str *s)
{
	shutdown(s->sok, SHUT_RDWR);
	close(s->sok);
	SSL_shutdown(s->ssl);
	SSL_free(s->ssl);
	free(s);
}

const char *
str_err(enum COM_ERR e)
{
	switch (e) {
	case COM_E_NOMEM:
		return "Out of memory.";
	case COM_E_OPENSSL_FAILURE:
		return "[awful error message] OpenSSL failure reported.";
	case COM_E_NO_KEY:
		return "No key was found. Is SYSTEMGMI_TLS_KEY set and valid?";
	case COM_E_NO_CERT:
		return "No certificate was found. Is SYSTEMGMI_TLS_CERT set and valid?";
	case COM_E_BAD_KEY:
		return "The given private key could not be used. Is it valid?";
	case COM_E_BAD_CERT:
		return "The given certificate could not be used. Is it valid?";
	case COM_E_CANT_MAKE_SOCKET:
		return "[awful error message] A socket couldn't be opened.";
	case COM_E_CANT_BIND:
		return "A socket couldn't be bound to the target port.";
		// TODO: what target port? Error enum and handling needs to be
		// slightly complicated to handle this.
	case COM_E_CANT_LISTEN:
		return "A socket couldn't listen(2).";
		// TOODO: why not? Error struct/enum should include errno.
	default:
		return "No error message present (unreachable case).";
	}
}

static SSL_CTX *
basic_openssl_context(void)
{
	SSL_library_init();
	const SSL_METHOD *m;
	SSL_CTX *s;

	m = TLS_server_method();
	s = SSL_CTX_new(m);
	if (!s) return NULL;

	SSL_CTX_set_min_proto_version(s, TLS1_3_VERSION);

	if (!SSL_CTX_set_min_proto_version(s, TLS1_3_VERSION)) return NULL;

	return s;
}
