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

/* Return a string (must be free(2)'d by the caller) containing the logo
 * obtained from neofetch. */
static char *neofetch_logo(void);

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
	if (!nf_cached) {
		if (!(nf_buf = neofetch_logo())) {
			log_info("neofetch isn't installed or is inaccessible via /bin/sh.");
			log_info("No neofetch output will be present on route /.");
		} else log_debug("Cached neofetch output.");

		nf_cached = 1;
	}

	return nf_buf;
}

static char *
neofetch_logo(void)
{
	char *nfl_buf, *nfl_cur, *pr_buf;
	int l, sl, inansiesc;
	FILE *nfl_out;

	// The capacities here are complete guesss, but 8192 bytes seems to be
	// enough for the logo. Calloc is used here to minimize the impact of
	// any null-terminator-related footguns.
	nfl_cur = nfl_buf = calloc(1, 8192);
	pr_buf = malloc(512);

	nfl_out = popen("neofetch --logo 2> /dev/null", "r");

	inansiesc = 0;
	while ((l = fread(pr_buf, 1, 511, nfl_out)) > 0) {
		pr_buf[l] = '\0'; // terminate

		// remove colors that gemini clients can't handle. This is done
		// in 512-byte batches for performance reasons. (No benchmarks,
		// though. TODO).
		inansiesc = remove_ansi_escapes(pr_buf, inansiesc);

		sl = strlen(pr_buf);
		memcpy(nfl_cur, pr_buf, sl);
		nfl_cur += sl;
	}

	// I have no idea why, but neofetch has appended 21 blank lines of
	// output to my neofetch - let's kill it here (after removing ANSI
	// codes), but keep one trailing newline.
	strip_trailing_whitespace(nfl_buf);
	sl = strlen(nfl_buf);
	nfl_buf[sl++] = '\n';
	nfl_buf[sl] = '\0';

	fclose(nfl_out);
	free(pr_buf);

	if (nfl_cur == nfl_buf) { /* no input was read */
		free(nfl_buf);
		return NULL;
	} else return nfl_buf;
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
