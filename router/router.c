#include <pthread.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "../com/com.h"
#include "../machine/machine.h"
#include "router.h"

static enum route_type match_route(const char *);

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
		without_host[0] != '/' && without_host[0] != '\0';
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
		hn = mn_hostname();

		com_write(s, "20 text/gemini\r\n");

		head = malloc(sizeof("# Units on ") + strlen(hn) + 1);
		sprintf(head, "# Units on %s\n", hn);

		com_write(s, head);

		free(head);
	}
}
