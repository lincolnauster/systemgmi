#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

#include "log.h"

static _Atomic unsigned int cnt_stage = 1;

/* Return the number of characters the stage indicator has taken up. */
static unsigned int stage_offset(void);

void
log_stage(const char *msg)
{
	fprintf(
		stderr,
		// print the stage with:
		// + a bold white counter to the left
		// + a green seperator followed by a green `msg`
		// + at the beginning of a line (useful to cover up any console
		//   input from ^C).
		"\r\033[1mSTAGE %d \033[0;32m| %s\033[0m\n",
		atomic_fetch_add(&cnt_stage, 1),
		msg
	);
}

void log_fatal(const char *msg)
{
	log_error(msg);
	exit(1);
}

void
log_error(const char *msg)
{
	fprintf(stderr,	"%*c\033[1;31m| %s\033[0m\n", stage_offset(), ' ', msg);
}

void
log_info(const char *msg)
{
	fprintf(stderr, "%*c\033[33m| %s\033[0m\n", stage_offset(), ' ', msg);
}

void
log_debug(const char *msg)
{
	fprintf(stderr,	"%*c\033[2m| %s\033[0m\n", stage_offset(), ' ', msg);
}

static unsigned int
stage_offset(void)
{
	unsigned int digits = 1;

	for (
		int stage_cp = atomic_load(&cnt_stage);
		stage_cp > 0;
		stage_cp /= 10
	) digits++;

	return digits + 6; // offset for the "STAGE: " indicator
}
