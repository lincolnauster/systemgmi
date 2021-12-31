#include <stdlib.h>
#include <string.h>

#include <openssl/ssl.h>

#include "com.h"
#include "util.h"

#include "types.h"

char *
com_until_crlf(struct tls_str *s)
{
	char *buf, *peek_buf;
	int pos, read_return;

	pos = 0;
	read_return = 0;

	// The spec specifies that the max URL length is 1024 bytes, this is a
	// good starting number to minimize the possibility of allocations.
	buf = malloc(1024);
	peek_buf = malloc(1);

	while (1) {
		SSL_read(s->ssl, peek_buf, 1);
		if (*peek_buf == '\r')
			read_return = 1;
		else if (read_return && *peek_buf == '\n')
			break;

		buf[pos++] = *peek_buf;
	}

	buf[pos - 1] = '\0'; // clear out the line ending

	free(peek_buf);
	return buf;
}
