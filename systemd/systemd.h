struct sd_unit {
	char *name;
	char *desc;
	int loaded:1;
	int active:1;
};

struct sd_unit_iter;

int sd_connect(void);
void sd_disconnect(void);

struct sd_unit_iter *sd_unit_iterate(void);
struct sd_unit *sd_unit_iter_next(struct sd_unit_iter *);

void sd_unit_iterate_free(struct sd_unit_iter *);
