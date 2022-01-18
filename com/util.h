/* Get a string from tls_str until a CRLF is read. If no such string exists,
 * com_until_crlf returns NULL. */
char *com_until_crlf(struct tls_str *);
