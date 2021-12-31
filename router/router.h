enum route_type {
	/* no metadata */
	ROUTE_NOT_FOUND,
	ROUTE_INDEX,
	ROUTE_LIST_UNITS,
	/* metadata is a string representing the amount of logs to send, which
	 * either matches /[0-9]+/ (that number of log lines has been requested)
	 * or thhe literal "all", requesting a realtime tail stream of logs. */
	ROUTE_JOURNAL_SYS,
};

/* Store a route as a fully enumerate type with potential extra metadata whose
 * precise meaning is dependent on the exact route. */
struct route {
	enum route_type type;
	char *md;
	int mdcp;
};

struct route route_url(const char *);

void write_page(struct tls_str *, struct route);
