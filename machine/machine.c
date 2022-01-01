#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../log/log.h"

static int nf_cached = 0;
static char *nf_buf = NULL;

static int hn_cached = 0;
static char *hn_buf = NULL;

/* Given a string and whether or not it starts inside an ANSI escape code (the
 * second int param), remove all ANSI escape codes from it. */
static int remove_ansi_escapes(char *, int);

static void strip_trailing_whitespace(char *);

void
mn_clear_cache(void)
{
	if (nf_cached) free(nf_buf);
	if (hn_cached) free(hn_buf);
}

char *
mn_hostname(void)
{
	if (hn_cached) return hn_buf;

	log_debug("Caching machine hostname.");
	char *buf = malloc(1024);
	gethostname(buf, 1023);

	log_debug("Cached machine hostname.");
	hn_buf = buf;
	hn_cached = 1;

	return buf;
}

char *
mn_neofetch(void)
{
	if (nf_cached) return nf_buf;

	char *nf_cur, *pr_buf;
	int l, sl, inansiesc;
	FILE *nf_out;

	log_debug("Caching neofetch output.");

	// The capacities here are complete guesss, but 8192 bytes seems to be
	// enough for the logo. Calloc is used here to minimize the impact of
	// any null-terminator-related footguns.
	nf_cur = nf_buf = calloc(1, 8192);
	pr_buf = malloc(512);

	nf_out = popen("neofetch --logo 2> /dev/null", "r");

	inansiesc = 0;
	while ((l = fread(pr_buf, 1, 511, nf_out)) > 0) {
		pr_buf[l] = '\0'; // terminate

		// remove colors that gemini clients can't handle
		inansiesc = remove_ansi_escapes(pr_buf, inansiesc);

		sl = strlen(pr_buf);
		memcpy(nf_cur, pr_buf, sl);
		nf_cur += sl;
	}

	// I have no idea why, but neofetch has appended 21 blank lines of
	// output to my neofetch - let's kill it here (after removing ANSI
	// codes), but keep one trailing newline.
	strip_trailing_whitespace(nf_buf);
	sl = strlen(nf_buf);
	nf_buf[sl++] = '\n';
	nf_buf[sl] = '\0';

	fclose(nf_out);
	free(pr_buf);
	nf_cached = 1;
	log_debug("Cached neofetch output.");

	if (nf_cur == nf_buf) { /* no input was read */
		log_info("neofetch isn't installed or inaccessible via /bin/sh.");
		nf_buf = NULL;
	}

	return nf_buf;
}

static int
remove_ansi_escapes(char *x, int inansiesc)
{
	/* This is slow & tricky & not great code, but the cache makes sure it
	 * only runs once. */
	int l;

	l = strlen(x);

	for (int i = 0; x[i]; i++) {
		char c = x[i];
		if (inansiesc || c == '\033') {
			inansiesc = (
				c != 'A' && c != 'D' &&
				c != 'h' && c != 'm'
			);

			memmove(x + i, x + i + 1, l - i);
			// Because we skipped over one, move the cursor back.
			// Note that this is fine on underflow (i = 0) because
			// the addition after the iteration will bring i back to
			// 0 (addition is an abelian group).
			i -= 1;
		}
	}

	return inansiesc;
}

static void
strip_trailing_whitespace(char *x)
{
	int i, l;
	l = strlen(x);
	i = l - 1;

	while (i >= 0 && isspace(x[i])) i--;

	x[i + 1] = '\0';
}
