/* systemgmi logging primitives
 *
 * This compilation unit contains a bunch of shared state for logging. All dynamic
 * console output should go through the functions here.
 */

/* Log that the program has entered a stage with the given name. This is not
 * thread-safe, as it logs and increments the index of the stage. */
void log_stage(const char *msg);

/* Log msg as error and then exit. */
void log_fatal(const char *msg);

/* Log important information critical to program functionality. */
void log_error(const char *msg);

/* Log potentially important information. */
void log_info(const char *msg);

/* Log msg as info of little importance. */
void log_debug(const char *msg);
