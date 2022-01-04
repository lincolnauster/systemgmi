#include <stdlib.h>
#include <strings.h>

#include <systemd/sd-bus.h>

#include "systemd.h"
#include "../log/log.h"

struct sd_unit {
	char *name;
	char *desc;
	int loaded;
	int active;
};

struct sd_unit_arr {
	int len;
	int cap;
	struct sd_unit *buf;
};

static struct {
	sd_bus *bus;
	struct sd_unit_arr *units;
} ctx = { NULL };

static struct sd_unit_arr *list_units(void);

int
sd_connect(void)
{
	log_debug("Connecting to the user bus.");

	if (sd_bus_default_user(&ctx.bus) < 0) return 1;

	log_debug("Caching listed units.");

	if ((ctx.units = list_units()) == NULL) return 1;

	log_debug("[TODO] subscribing to systemd changes");

	return 0;
}

void
sd_disconnect(void)
{
	sd_bus_unref(ctx.bus);

	free(ctx.units->buf);
	ctx.units->cap = 0;
	ctx.units->len = 0;
	ctx.units->buf = NULL;
	free(ctx.units);
	ctx.units = NULL;
}

static struct sd_unit_arr *
list_units(void)
{
	int r;
	sd_bus_message *reply;
	sd_bus_error error = SD_BUS_ERROR_NULL;

	struct sd_unit_arr *ret = malloc(sizeof(struct sd_unit_arr));

	ret->len = 0;
	ret->cap = 512; /* this is a guess; might not be optimal. */
	ret->buf = malloc(sizeof(struct sd_unit) * ret->cap);

	if (sd_bus_call_method(
		ctx.bus,
		"org.freedesktop.systemd1",
		"/org/freedesktop/systemd1",
		"org.freedesktop.systemd1.Manager",
		"ListUnits",
		&error,
		&reply,
		""
	) < 0) {
		log_error("Couldn't call ListUnits.");
		log_error("Sd-bus gave the following error:");
		log_error(error.message);
		return NULL;
	};

	if ((r = sd_bus_message_enter_container(
		reply,
		SD_BUS_TYPE_ARRAY,
		"(ssssssouso)"
	)) < 0) {
		r = ~r + 1;
		char msg[] = "Systemd ListUnits read initialization failed (          ).";
		sprintf(msg, "Systemd ListUnits read initialization failed (%d).", r);
		log_debug(msg);
		return NULL;
	}

	do {
		const char *loaded, *active;
		struct sd_unit entry;

		if ((r = sd_bus_message_read(
			reply,
			"(ssssssouso)",
			&entry.name,
			&entry.desc,
			&loaded,
			&active,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL
		)) < 0) {
			r = ~r + 1;
			char msg[] = "Systemd ListUnits read failed (          ).";
			sprintf(msg, "Systemd ListUnits read failed (%d).", r);
			log_debug(msg);
			return NULL;
		}

		entry.loaded = strcmp(loaded, "loaded") == 0;
		entry.active = strcmp(loaded, "active") == 0;
		ret->len++;
	} while (r != 0);

	sd_bus_message_unref(reply);

	return ret;
}
