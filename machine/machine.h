/* Clear the cache and deallocate all associated memory. */
void mn_clear_cache(void);

/* Get an owned reference to the hostname. */
char *mn_hostname(void);

/* Get a reference to neofetch's formatted output. Callers must *not* free() the
 * returned value. The returned value is NULL if neofetch is not in PATH.
 * Otherwise, the returned output looks identical to neofetch's output but
 * is represented as a simple c-string without ANSI escapes or cursor motion.
 *
 * TODO: keep the output from info as well as the logo. */
char *mn_neofetch(void);

