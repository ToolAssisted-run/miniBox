/* Phase-1 self-test: exercises the memory block's dirty tracking and savestate
 * round-trip WITHOUT a guest ELF - constructs a block directly, writes through
 * the guest mapping (tripping the fault handler), seals, saves, mutates, loads,
 * and checks the machine returned to the saved point. Mirrors the coverage of
 * runtime/src/memory_block/tests.rs. */
#include "minibox_internal.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* growable in-memory stream shared by the read and write callbacks */
typedef struct { uint8_t *buf; size_t len, cap, pos; } membuf;
static int32_t mem_write(uintptr_t ud, const uint8_t *data, uintptr_t n) {
	membuf *m = (membuf *)ud;
	if (m->len + n > m->cap) { m->cap = (m->len + n) * 2; m->buf = realloc(m->buf, m->cap); }
	memcpy(m->buf + m->len, data, n); m->len += n; return 0;
}
static intptr_t mem_read(uintptr_t ud, uint8_t *data, uintptr_t n) {
	membuf *m = (membuf *)ud;
	uintptr_t avail = m->len - m->pos;
	if (n > avail) n = avail;
	memcpy(data, m->buf + m->pos, n); m->pos += n; return (intptr_t)n;
}

static int fails = 0;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

int main(void) {
	mb_range addr = { 0x36f00000000ull, 0x10000 };
	mb_block *b = mb_block_new(addr);
	CHECK(b != NULL);
	if (!b) return 1;
	mb_block_activate(b);

	/* map the whole block RW and prove writes trip dirty detection */
	CHECK(mb_block_mmap_fixed(b, addr, MB_PROT_RW, true) == 0);
	volatile uint8_t *p = (volatile uint8_t *)addr.start;
	p[0x2003] = 5;
	CHECK((mb_block_page_info(b, 2) & 0x80) != 0);   /* page 2 dirty */
	CHECK((mb_block_page_info(b, 3) & 0x80) == 0);   /* page 3 clean */

	/* fill some known content across a few pages */
	for (int i = 0; i < 0x4000; i++) p[i] = (uint8_t)(i * 7 + 1);

	/* seal: this content becomes the baseline */
	CHECK(mb_block_seal(b) == 0);

	/* after seal everything is clean again */
	CHECK((mb_block_page_info(b, 2) & 0x80) == 0);

	/* mutate post-seal, then save */
	p[0x0010] = 0xAA;
	p[0x3abc] = 0xBB;
	CHECK((mb_block_page_info(b, 0) & 0x80) != 0);
	CHECK((mb_block_page_info(b, 3) & 0x80) != 0);

	membuf state = {0};
	CHECK(mb_block_save_state(b, mem_write, (uintptr_t)&state) == 0);

	uint8_t saved0010 = p[0x0010], saved3abc = p[0x3abc];
	CHECK(saved0010 == 0xAA && saved3abc == 0xBB);

	/* mutate further away from the saved point */
	p[0x0010] = 0x11;
	p[0x3abc] = 0x22;
	p[0x1000] = 0x33;   /* dirty a page that was clean at save time */
	CHECK(p[0x0010] == 0x11);

	/* load: must restore exactly the saved point */
	state.pos = 0;
	CHECK(mb_block_load_state(b, mem_read, (uintptr_t)&state) == 0);
	CHECK(p[0x0010] == 0xAA);
	CHECK(p[0x3abc] == 0xBB);
	CHECK(p[0x1000] == (uint8_t)(0x1000 * 7 + 1));   /* reverted to baseline content */

	/* baseline content elsewhere intact */
	CHECK(p[0x2003] == (uint8_t)(0x2003 * 7 + 1));

	/* a second save/load round-trip from the restored state */
	membuf state2 = {0};
	CHECK(mb_block_save_state(b, mem_write, (uintptr_t)&state2) == 0);
	p[0x0010] = 0x99;
	state2.pos = 0;
	CHECK(mb_block_load_state(b, mem_read, (uintptr_t)&state2) == 0);
	CHECK(p[0x0010] == 0xAA);

	/* munmap zeroes and frees */
	mb_range one = { addr.start + 0x5000, 0x1000 };
	CHECK(mb_block_mmap_fixed(b, one, MB_PROT_RW, false) == 0);
	*(volatile uint8_t *)(addr.start + 0x5000) = 7;
	CHECK(mb_block_munmap(b, one) == 0);
	CHECK((mb_block_page_info(b, 5) & 0x1f) == 0);   /* free */

	mb_block_deactivate(b);
	mb_block_free(b);
	free(state.buf); free(state2.buf);

	if (fails == 0) printf("test_memblock: all checks passed\n");
	else printf("test_memblock: %d checks FAILED\n", fails);
	return fails ? 1 : 0;
}
