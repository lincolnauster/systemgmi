#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "log/log.h"

int cli_use_system_bus = 0;

static void help(void);
static void warn_unknown_arg(const char *);

void
parse_args(int argc, char **argv)
{

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--help") == 0) help();
		if (strcmp(argv[i], "--system") == 0) cli_use_system_bus = 1;
		else warn_unknown_arg(argv[i]);
	}
}

static void
help(void)
{
	puts(
		"systemgmi: a systemd monitor over gemini\n\n"
		"Usage: systemgmi [--help] [--system]\n\n"
		"--system may be specified to connect to the system bus instead "
		"of to the user bus."
	);
	exit(0);
}

static
void warn_unknown_arg(const char *x)
{
	char *msg;
	msg = malloc(sizeof("Didn't recognize argument ``.") + strlen(x));
	sprintf(msg, "Didn't recognize argument `%s`.", x);
	log_info(msg);

	free(msg);
}
