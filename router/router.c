#include <pthread.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "../com/com.h"
#include "../machine/machine.h"
#include "../systemd/systemd.h"
#include "router.h"

static enum route_type match_route(const char *);

static void unit_write(struct tls_str *, struct sd_unit *);

struct route
route_url(const char *url)
{
	struct route r;
	const char *without_scheme, *without_host;

	for (
		without_scheme = url;
		strncmp(without_scheme, "://", 3) != 0;
		without_scheme++
	) {}

	without_scheme += 3; // remove starting ://

	for (
		without_host = without_scheme;
		*without_host != '/' && *without_host != '\0';
		without_host++
	) {}

	without_host++; // remove starting slash

	r.type = match_route(without_host);
	r.md = NULL;
	r.mdcp = 0;
	return r;
}

static enum route_type
match_route(const char *res)
{
	if (*res == '\0' || strcmp(res, "/") == 0)
		return ROUTE_INDEX;
	else if (strcmp(res, "systemctl/list-units/") == 0)
		return ROUTE_LIST_UNITS;
	else
		return ROUTE_NOT_FOUND;
}

void
write_page(struct tls_str *s, struct route r)
{
	if (r.type == ROUTE_NOT_FOUND)
		com_write(s, "51\r\n");
	else if (r.type == ROUTE_INDEX) {
		char *hn, *nf, *buf;
		int tmp;

		com_write(s, "20 text/gemini\r\n");

		hn = mn_hostname();
		tmp = strlen(hn);
		buf = malloc(tmp + 5);
		sprintf(buf, "# %s\n\n", hn);
		com_write(s, buf);

		nf = mn_neofetch();
		if (nf != NULL) {
			com_write(s, "```Output of neofetch.\n");
			com_write(s, nf);
			com_write(s, "```\n");
		}

		com_write(s, "=> journald/sys/50       Recent journald logs\n");
		com_write(s, "=> journald/sys/all      Streaming realtime journal\n");
		com_write(s, "=> systemctl/list-units/ Status by unit\n");

		free(buf);
	} else if (r.type == ROUTE_LIST_UNITS) {
		char *hn, *head;
		struct sd_unit_iter *iter;
		struct sd_unit *unit;

		hn = mn_hostname();

		com_write(s, "20 text/gemini\r\n");

		head = malloc(sizeof("# Units on ") + strlen(hn) + 1);
		sprintf(head, "# Units on %s\n", hn);
		com_write(s, head);

		iter = sd_unit_iterate();
		while ((unit = sd_unit_iter_next(iter)))
			unit_write(s, unit);

		sd_unit_iterate_free(iter);
		free(head);
	}
}

static void
unit_write(struct tls_str *s, struct sd_unit *u)
{
	com_write(s, "## ");
	com_write(s, u->name);
	com_write(s, "\n");
	if (u->loaded) com_write(s, "[loaded] ");
	if (u->active) com_write(s, "[active] ");
	if (u->loaded || u->active) com_write(s, "\n");
	com_write(s, u->desc);
	com_write(s, "\n");
}
