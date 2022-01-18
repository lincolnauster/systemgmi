#include <stdlib.h>
#include <string.h>

#include <openssl/ssl.h>

#include "com.h"
#include "util.h"

#include "types.h"

char *
com_until_crlf(struct tls_str *s)
{
	char *buf;
	int l;

	// The spec specifies that the max URL length is 1024 bytes (thus a
	// 1025-byte buffer with a null terminator.
	buf = malloc(1025);

	l = SSL_read(s->ssl, buf, 1024);

	// Look for a CRLF.
	for (int i = l; i > 1; i--) {
		if (buf[i - 1] == '\r' && buf[i] == '\n') {
			buf[i - 1] = '\0';
			goto out;
		}
	}

	// If the PC gets here, then we've not found a CRLF.
	free(buf);
	buf = NULL;
out:
	return buf;
}
