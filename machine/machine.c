#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <unicode/ubrk.h>

#include "../log/log.h"

struct dimensions {
	int x, y;
	int x_bytes;
};

static int nf_cached = 0;
static char *nf_buf = NULL;

static int hn_cached = 0;
static char *hn_buf = NULL;

/* Return a string (must be free(2)'d by the caller) containing the logo
 * obtained from neofetch. */
static char *neofetch_logo(void);

/* Return a string (must be free(2)'d by the caller) containing the text content
 * obtained from neofetch. */
static char *neofetch_text(void);

/* Interweave two strings next to each other so that when printed, they appear
 * to be adjacent. The return value is caller-owned, and the given strings are
 * callee-owned (ownership is not taken). */
static char *weave_adjacent_strs(const char *, const char *, int);

/* Given a string and whether or not it starts inside an ANSI escape code (the
 * second int param), remove all ANSI escape codes from it. */
static int remove_ansi_escapes(char *, int);

static void strip_trailing_whitespace(char *);

/* Calculate the physical dimensions of a string. This accounts for
 * mulit-codepoint Unicode graphemes (though it assumes UTF-8. */
static struct dimensions dimensions_from_string(const char *);

/* Count the number of graphemes in a string. If len is given as NULL, the
 * string is assumed to be null-terminated. Otherwise, only len bytes are
 * accounted for. */
static int grapheme_count(const char *s, int *len);

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
		char *nf_logo;
		char *nf_text;
		if (!(nf_logo = neofetch_logo())) {
			log_info("neofetch isn't installed or is inaccessible via /bin/sh.");
			log_info("No neofetch output will be present on route /.");
			goto out;
		}

		if (!(nf_text = neofetch_text())) {
			// XXX: We really *should* be able to handle this, but
			// for simplicity, just end. Something has gone very
			// wrong if the command has failed.
			log_fatal("neofetch text output couldn't be read. Can't handle.");
		}

		nf_buf = weave_adjacent_strs(nf_logo, nf_text, 2);
		free(nf_logo);
		free(nf_text);
out:
		nf_cached = 1;
		log_debug("Cached neofetch output.");
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

static char *
neofetch_text(void)
{
	FILE *nft_out;
	char *nft_buf, *nft_cur, *pr_buf;
	int l, sl;

	nft_out = popen("neofetch --stdout", "r");

	// FIXME(mem safety): we don't realloc this if it's too little. Probably
	// fine given that neofetch output is pretty reasonable, but not
	// technically correct.
	nft_cur = nft_buf = calloc(1, 1024);
	pr_buf = malloc(512);

	while ((l = fread(pr_buf, 1, 511, nft_out)) > 0) {
		pr_buf[l] = '\0';

		sl = strlen(pr_buf);
		memcpy(nft_cur, pr_buf, sl);
		nft_cur += sl;
	}

	fclose(nft_out);
	free(pr_buf);

	strip_trailing_whitespace(nft_buf);
	sl = strlen(nft_buf);
	nft_buf[sl++] = '\n';
	nft_buf[sl] = '\0';

	return nft_buf;
}

static char *
weave_adjacent_strs(const char *a, const char *b, int padding)
{
	char *buf, *buf_cursor;
	struct dimensions ad, bd;
	int buf_size, target_width_bytes, target_height_bytes;
	int a_line_start, a_line_len, a_line_len_graphemes;
	int b_line_len, b_line_len_graphemes;
	int req_whitespace;

	ad = dimensions_from_string(a);
	bd = dimensions_from_string(b);

	target_width_bytes = ad.x_bytes + bd.x_bytes;
	target_height_bytes = ad.y > bd.y
                            ? ad.y * ad.x_bytes
                            : bd.y * bd.x_bytes;

	buf_size = target_width_bytes * target_height_bytes + 1;

	// Calloc means we canonically don't need to care about null termination.
	buf_cursor = buf = calloc(1, buf_size);

	a_line_start = 0;
	for (int i = 0; a[i] != '\0'; i++) {
		if (a[i] == '\n' || a[i] == '\0') {
			a_line_len = i - a_line_start;
			memcpy(buf_cursor, a + a_line_start, a_line_len);

			buf_cursor += a_line_len;

			a_line_len_graphemes = grapheme_count(a + a_line_start, &a_line_len);
			req_whitespace = padding + ad.x - a_line_len_graphemes;

			// pad the line of a to the required length
			for (int i = 0; i < req_whitespace; i++) {
				*buf_cursor = ' ';
				buf_cursor++;
			}

			for (int j = 0; 1; j++) {
				if (b[j] == '\n' || b[j] == '\0') {
					b_line_len = j;
					memcpy(buf_cursor, b, b_line_len);

					buf_cursor += b_line_len;
					b_line_len_graphemes = grapheme_count(b, &b_line_len);
					req_whitespace = bd.x - b_line_len_graphemes;

					for (int j = 0; j < req_whitespace; j++) {
						*buf_cursor = ' ';
						buf_cursor++;
					}

					b += b_line_len + 1;
					*buf_cursor = '\n';
					buf_cursor++;
					break;
				}

				if (b[j] == '\0') break;
			}

			a_line_start = i + 1;
		}
	}

	return buf;
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

static struct dimensions
dimensions_from_string(const char *s)
{
	char c;
	int y, i, l;
	int maxlen, maxbytelen, linestart, line_bytelen, len;
	maxlen = maxbytelen = 0;
	linestart = 0;
	y = 0;

	i = 0;
	l = strlen(s);

	while (i < l) {
		c = s[i];
		switch (c) {
		case '\n': y++;
		case '\0':
			line_bytelen = i - linestart;
			len = grapheme_count(s + linestart, &line_bytelen);

			if (len > maxlen) maxlen = len;
			if (line_bytelen > maxbytelen)
				maxbytelen = line_bytelen;

			linestart = i + 1;
			break;
		}

		i++;
	}

	struct dimensions d = {
		.x = maxlen,
		.y = y,
		.x_bytes = maxbytelen,
	};

	return d;
}

static int
grapheme_count(const char *s, int *len)
{
	int l, graphemes;

	UBreakIterator *ubi;
	UText *ut;

	// XXX: it may be nice to actually do error checking
	UErrorCode uec = U_ZERO_ERROR;

	graphemes = 0;
	l = len ? *len : -1;

	ut = utext_openUTF8(NULL, s, l, &uec);

	ubi = ubrk_open(UBRK_CHARACTER, NULL, NULL, l, &uec);
	ubrk_setUText(ubi, ut, &uec);

	while (ubrk_next(ubi) != UBRK_DONE) graphemes++;

	ubrk_close(ubi);
	utext_close(ut);
	return graphemes;
}
