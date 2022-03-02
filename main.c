#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include <unicode/uclean.h>

#include "cli.h"
#include "com/com.h"
#include "com/util.h"
#include "log/log.h"
#include "machine/machine.h"
#include "router/router.h"
#include "systemd/systemd.h"
#include "threading/threading.h"

/* Given two C-strings, return 1 if they can be considered equivalent as CLI
 * arguments; i.e., --version matches -v, but not --ver or --v. a is expected to
 * be user input, and b is expected to be the 'full' argument. */
static int cli_matches(const char *a, const char *b);

static void block_till_kill(void);

static void serve(struct tls_str *);
static void serve_inner(struct tls_str *x);

static int version(void);
static int help(void);

int
main(int argc, char **argv)
{
	parse_args(argc, argv);

	if (argc > 1 && cli_matches(argv[1], "--version"))
		return version();
	if (argc > 1 && cli_matches(argv[1], "--help"))
		return help();

	log_stage("CACHE POPULATION");
	mn_neofetch();
	mn_hostname();

	if (sd_init() != 0) {
		sd_uninit();
		log_fatal("Couldn't set up systemd connection.");
	}

	log_stage("THREADPOOL INIT");
	threader_init();

	log_stage("NETWORK SETUP");

	struct fallible_tls_ctx f_tls_ctx = setup_tls();
	struct tls_ctx *tls_ctx = f_tls_ctx.ok;
	if (!tls_ctx) log_fatal(str_err(f_tls_ctx.err));

	log_debug("A valid TLS context was acquired and configured.");

	struct fallible_tls_conn f_tls_conn = com_bind(tls_ctx, 1965);
	if (!f_tls_conn.ok) log_fatal(str_err(f_tls_conn.err));
	struct tls_conn *tls_conn = f_tls_conn.ok;

	log_debug("Systemgmi bound to port 1965.");

	struct listen_conf lc = {
		.con = tls_conn,
		.callback = serve,
	};

	log_stage("SERVING");
	pthread_t server = com_listen(&lc);

	// Once we start serving, we want to delay cleanup until CTRL-C has been
	// pressed.
	block_till_kill();

	log_stage("CLEANUP");

	pthread_cancel(server);
	pthread_join(server, NULL);
	threader_close();
	log_debug("All threads terminated.");

	sd_uninit();
	log_debug("Uninitialized dbus connection.");

	free_tls_ctx(tls_ctx);
	free_tls_conn(tls_conn);
	mn_clear_cache();
	u_cleanup();
	log_debug("All known heap objects freed.");

	return 0;
}

static int
cli_matches(const char *a, const char *b)
{
	if (strlen(a) < 2 || strlen(b) < 2) return 1;
	else if (a[1] == b[2]) return 1;
	else return strcmp(a, b) == 0;
}

static void
block_till_kill(void)
{
	int tmp;
	sigset_t sigset;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGINT);
	sigaddset(&sigset, SIGTERM);
	sigprocmask(SIG_BLOCK, &sigset, NULL);
	sigwait(&sigset, &tmp);
}

static void
serve(struct tls_str *x)
{
	struct work *work = malloc(sizeof(struct work));
	work->args = x;

	work->routine = (void (*)(void *)) serve_inner,
	// serve_inner cleans up all internal resources, so we can defer the
	// rest of the cleanup to a simple free(2). It's a bit of a hack, but it
	// actually works great.
	work->cleanup = (void (*)(struct work *)) free;

	threader_queue(work);
}

static void
serve_inner(struct tls_str *x)
{
	char *url, *log_msg;
	int log_len;

	url = com_until_crlf(x);
	struct route r = route_url(url);
	write_page(x, r);
	com_close(x);

	if (url) {
		log_len = strlen("Served resource: ") + strlen(url) + 1;
		log_msg = malloc(log_len);
		memcpy(log_msg, "Served resource: ", strlen("Served resource: "));
		memcpy(log_msg + strlen("Served resource: "), url, strlen(url) + 1);
		log_info(log_msg);
		free(url);
		free(log_msg);
	}
}

static int
version(void)
{
	puts("systemgmi v0.0.1");
	return 0;
}

static int
help(void)
{
	puts("    systemgmi v0.0.1");
	puts("");
	puts("Required environment variables:\n");
	puts("    SYSTEMGMI_TLS_KEY  => File location for a TLS key.");
	puts("    SYSTEMGMI_TLS_CERT => File location for a TLS certificate.");
	puts("");
	puts("CLI arguments:\n");
	puts("    --version | -v => print the version and exit.");
	puts("    --help    | -h => print this help message and exit.");
	puts("");
	return 0;
}
