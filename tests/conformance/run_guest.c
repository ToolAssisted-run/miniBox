/* End-to-end conformance test: loads guest.wbx through the miniBox C host,
 * runs it, seals, and proves the savestate round-trip - the whole phase-1
 * stack (ELF load, __wbxsysinfo handshake, guest execution via the interop
 * trampolines, guest syscalls, sealed/invisible memory, save/load) working
 * together with a real gcc-built guest. */
#include "minibox.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* file reader for wbx_create_host */
typedef struct { FILE *f; } freader;
static intptr_t file_read(uintptr_t ud, uint8_t *data, uintptr_t size) {
	freader *r = (freader *)ud;
	size_t got = fread(data, 1, size, r->f);
	return (intptr_t)got;
}

/* growable membuf for save/load */
typedef struct { uint8_t *buf; size_t len, cap, pos; } membuf;
static int32_t mem_write(uintptr_t ud, const uint8_t *data, uintptr_t n) {
	membuf *m = (membuf *)ud;
	if (m->len + n > m->cap) { m->cap = (m->len + n) * 2; m->buf = realloc(m->buf, m->cap); }
	memcpy(m->buf + m->len, data, n); m->len += n; return 0;
}
static intptr_t mem_read(uintptr_t ud, uint8_t *data, uintptr_t n) {
	membuf *m = (membuf *)ud;
	uintptr_t avail = m->len - m->pos; if (n > avail) n = avail;
	memcpy(data, m->buf + m->pos, n); m->pos += n; return (intptr_t)n;
}

static int fails = 0;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

/* guest export thunk types (sysv, <=6 int/ptr args) */
typedef int      (*init_fn)(void);
typedef uint32_t (*step_fn)(uint32_t);
typedef uint64_t (*getacc_fn)(void);

static uintptr_t proc(mb_host *h, const char *name) {
	mb_return r; wbx_get_proc_addr(h, name, &r);
	if (r.error_message[0]) { fprintf(stderr, "get_proc_addr(%s): %s\n", name, r.error_message); exit(2); }
	return r.data;
}

int main(int argc, char **argv) {
	const char *path = argc > 1 ? argv[1] : "guest.wbx";
	FILE *f = fopen(path, "rb");
	if (!f) { fprintf(stderr, "cannot open %s\n", path); return 1; }

	mb_memory_layout_template layout = {
		.sbrk_size = 16 << 20, .sealed_size = 16 << 20, .invis_size = 16 << 20,
		.plain_size = 16 << 20, .mmap_size = 32 << 20,
	};
	freader fr = { f };
	mb_return r;
	wbx_create_host(&layout, "guest.wbx", file_read, (uintptr_t)&fr, &r);
	fclose(f);
	if (r.error_message[0]) { fprintf(stderr, "create_host: %s\n", r.error_message); return 1; }
	mb_host *h = (mb_host *)r.data;

	wbx_activate_host(h, &r); CHECK(!r.error_message[0]);

	init_fn Init = (init_fn)proc(h, "Init");
	step_fn Step = (step_fn)proc(h, "Step");
	getacc_fn GetAcc = (getacc_fn)proc(h, "GetAcc");

	CHECK(Init() == 1);

	/* seal, then run a few deterministic steps */
	wbx_deactivate_host(h, &r);
	wbx_seal(h, &r);
	if (r.error_message[0]) { fprintf(stderr, "seal: %s\n", r.error_message); return 1; }
	wbx_activate_host(h, &r);

	uint32_t s1 = Step(0x11111111);
	uint32_t s2 = Step(0x22222222);
	uint64_t acc_at_save = GetAcc();
	printf("after 2 steps: step results %08x %08x, acc %016llx\n", s1, s2, (unsigned long long)acc_at_save);

	/* savestate here */
	membuf state = {0};
	wbx_deactivate_host(h, &r);
	wbx_save_state(h, mem_write, (uintptr_t)&state, &r);
	if (r.error_message[0]) { fprintf(stderr, "save_state: %s\n", r.error_message); return 1; }
	wbx_activate_host(h, &r);

	/* advance further, changing state */
	uint32_t s3 = Step(0x33333333);
	uint64_t acc_after = GetAcc();
	CHECK(acc_after != acc_at_save);
	(void)s3;

	/* load: state must return to the save point */
	state.pos = 0;
	wbx_deactivate_host(h, &r);
	wbx_load_state(h, mem_read, (uintptr_t)&state, &r);
	if (r.error_message[0]) { fprintf(stderr, "load_state: %s\n", r.error_message); return 1; }
	wbx_activate_host(h, &r);

	CHECK(GetAcc() == acc_at_save);

	/* determinism: replaying the same step from the restored point reproduces s3 */
	uint32_t s3b = Step(0x33333333);
	CHECK(s3b == s3);

	wbx_deactivate_host(h, &r);
	wbx_destroy_host(h, &r);
	free(state.buf);

	if (fails == 0) printf("run_guest: all checks passed\n");
	else printf("run_guest: %d checks FAILED\n", fails);
	return fails ? 1 : 0;
}
