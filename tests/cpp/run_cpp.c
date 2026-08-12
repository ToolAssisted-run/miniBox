/* System test for C++ (Tier 2) guests: runs guest_cpp.wbx through the host and
 * checks that STL state behaves correctly across a whole-machine savestate.
 * Failing here means C++ guests are not safely stateful, which would break
 * determinism for any C++ core. */
#include "minibox.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { FILE *f; } freader;
static intptr_t file_read(uintptr_t ud, uint8_t *data, uintptr_t size) {
	return (intptr_t)fread(data, 1, size, ((freader *)ud)->f);
}
typedef struct { uint8_t *buf; size_t len, cap, pos; } membuf;
static int32_t mem_write(uintptr_t ud, const uint8_t *data, uintptr_t n) {
	membuf *m = (membuf *)ud;
	if (m->len + n > m->cap) { m->cap = (m->len + n) * 2 + 64; m->buf = realloc(m->buf, m->cap); }
	memcpy(m->buf + m->len, data, n); m->len += n; return 0;
}
static intptr_t mem_read(uintptr_t ud, uint8_t *data, uintptr_t n) {
	membuf *m = (membuf *)ud;
	uintptr_t avail = m->len - m->pos; if (n > avail) n = avail;
	memcpy(data, m->buf + m->pos, n); m->pos += n; return (intptr_t)n;
}

static int fails = 0;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "  FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

typedef int (*intfn)(void);
typedef void (*voidfn)(void);

static uintptr_t proc(mb_host *h, const char *n) {
	mb_return r;
	wbx_get_proc_addr(h, n, &r);
	if (r.error_message[0]) { fprintf(stderr, "proc %s: %s\n", n, r.error_message); exit(2); }
	return r.data;
}

int main(int argc, char **argv) {
	if (argc < 2) { fprintf(stderr, "usage: run_cpp <guest_cpp.wbx>\n"); return 2; }
	FILE *f = fopen(argv[1], "rb");
	if (!f) { perror(argv[1]); return 1; }

	mb_memory_layout_template layout = { 16u<<20, 16u<<20, 16u<<20, 16u<<20, 32u<<20 };
	freader fr = { f };
	mb_return r;
	wbx_create_host(&layout, "guest_cpp.wbx", file_read, (uintptr_t)&fr, &r);
	fclose(f);
	if (r.error_message[0]) { fprintf(stderr, "create: %s\n", r.error_message); return 1; }
	mb_host *h = (mb_host *)r.data;

	wbx_activate_host(h, &r);
	intfn Init = (intfn)proc(h, "Init");
	voidfn Mutate = (voidfn)proc(h, "Mutate");
	intfn Sizes = (intfn)proc(h, "Sizes");

	/* vector<int> sorted with a lambda + map<string,int> + virtual dispatch */
	CHECK(Init() == 140);
	CHECK(Sizes() == 100 * 1000 + 1);

	wbx_deactivate_host(h, &r);
	wbx_seal(h, &r);
	if (r.error_message[0]) { fprintf(stderr, "seal: %s\n", r.error_message); return 1; }
	wbx_activate_host(h, &r);

	/* savestate, mutate the STL containers, reload: the sizes must roll back,
	 * i.e. the STL heap really lives in guest memory and is captured. */
	membuf st = {0};
	wbx_deactivate_host(h, &r); wbx_save_state(h, mem_write, (uintptr_t)&st, &r); wbx_activate_host(h, &r);
	if (r.error_message[0]) { fprintf(stderr, "save: %s\n", r.error_message); return 1; }

	Mutate();
	CHECK(Sizes() == 150 * 1000 + 2);

	st.pos = 0;
	wbx_deactivate_host(h, &r); wbx_load_state(h, mem_read, (uintptr_t)&st, &r); wbx_activate_host(h, &r);
	if (r.error_message[0]) { fprintf(stderr, "load: %s\n", r.error_message); return 1; }
	CHECK(Sizes() == 100 * 1000 + 1);

	/* and it must be replayable: the same mutation after reload is deterministic */
	Mutate();
	CHECK(Sizes() == 150 * 1000 + 2);

	wbx_deactivate_host(h, &r); wbx_destroy_host(h, &r);
	free(st.buf);
	printf(fails ? "cpp guest: %d FAILED\n" : "cpp guest: OK\n", fails);
	return fails ? 1 : 0;
}
