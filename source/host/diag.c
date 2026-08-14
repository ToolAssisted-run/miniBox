/* diag.c - saying what happened when the sandbox dies.
 *
 * A guest death is not like other bugs: there is no core dump to read, no
 * debugger attached, and often no console. On Windows the host is loaded into a
 * GUI process, so stderr goes precisely nowhere - which turns "the guest hit an
 * unimplemented syscall at frame 3" into "the emulator vanished", and the person
 * who could have fixed it in five minutes gets nothing to work from.
 *
 * So a FATAL diagnostic also goes to a file. Only fatal ones: this writes
 * nothing during a normal run, and the file appearing at all means something
 * died. MINIBOX_LOG names it; otherwise it lands beside the working directory as
 * minibox-diag.log.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "minibox_internal.h"

static const char *diag_path(void) {
	const char *env = getenv("MINIBOX_LOG");
	return (env != NULL && *env != '\0') ? env : "minibox-diag.log";
}

void mb_diag(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fflush(stderr);

	/* Appended, not truncated: a second failure after the first is a fact worth
	 * keeping, and the file is small. */
	FILE *f = fopen(diag_path(), "a");
	if (f == NULL) return;
	va_start(ap, fmt);
	vfprintf(f, fmt, ap);
	va_end(ap);
	fclose(f);
}

void mb_diag_banner(const char *what) {
	time_t now = time(NULL);
	char stamp[32];
	struct tm *tm = localtime(&now);
	if (tm == NULL || strftime(stamp, sizeof stamp, "%Y-%m-%d %H:%M:%S", tm) == 0) stamp[0] = '\0';
	mb_diag("\n=== miniBox: %s (%s) ===\n", what, stamp);
}
