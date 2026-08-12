/* Host<->guest transitions. Maps the fixed interop blob (interop.bin, assembled
 * from BizHawk waterboxhost src/context/interop.s) at MB_ORG and drives entries through it.
 * Faithful C port of BizHawk waterboxhost src/context/{mod.rs,thunks.rs} (Linux path). */
#ifndef _WIN32
#define _GNU_SOURCE
#endif
#include "minibox_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifndef _WIN32
#include <sys/syscall.h>
#include <unistd.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#define CALL_GUEST_SIMPLE_ADDR (MB_ORG + 0x100)
#define CALL_GUEST_IMPL_ADDR   (MB_ORG + 0x200)
#define EXTCALL_THUNK_ADDR     (MB_ORG + 0x300)
#define RUNTIME_TABLE_ADDR     (MB_ORG + 0x800)

/* interop.bin lives in the sibling reference tree; embedded at build time. */
extern const unsigned char mb_interop_bin[];
extern const unsigned int  mb_interop_bin_len;

static bool g_interop_ready = false;

static void init_interop_area(void) {
	if (g_interop_ready) return;
	mb_range want = { MB_ORG, mb_interop_bin_len };
	mb_range got;
	if (mb_pal_map_anon(mb_range_align_expand(want), MB_PROT_RW, &got) != 0) {
		fprintf(stderr, "miniBox: failed to map interop area at %llx\n", (unsigned long long)MB_ORG);
		abort();
	}
	memcpy((void *)MB_ORG, mb_interop_bin, mb_interop_bin_len);
	mb_pal_protect(mb_range_align_expand(want), MB_PROT_RX);
#ifdef _WIN32
	/* register the hand-written unwind info so host SEH can unwind across the
	 * guest stack-switch (the table lives at RUNTIME_TABLE_ADDR in interop.bin) */
	if (!RtlAddFunctionTable((PRUNTIME_FUNCTION)RUNTIME_TABLE_ADDR, 2, MB_ORG)) {
		fprintf(stderr, "miniBox: RtlAddFunctionTable failed\n");
		abort();
	}
#endif
	g_interop_ready = true;
}

#ifndef _WIN32
/* Per-host-thread mini-TLS block reached via [gs:0x18] (index 3). Windows uses
 * the TEB's SubSystemTib field at gs:0x18 directly, so no setup is needed. */
static __thread uintptr_t g_tib[4];
#endif

void mb_prepare_thread(void) {
	init_interop_area();
#ifndef _WIN32
	uintptr_t gs = 0;
	if (syscall(SYS_arch_prctl, 0x1004 /*ARCH_GET_GS*/, &gs) == 0 && gs == 0) {
		syscall(SYS_arch_prctl, 0x1001 /*ARCH_SET_GS*/, (uintptr_t)&g_tib[0]);
	}
#endif
}

void mb_context_init(mb_context *c, uintptr_t guest_rsp, uintptr_t guest_rsp_alt, mb_syscall_cb dispatch) {
	memset(c, 0, sizeof(*c));
	c->guest_rsp = guest_rsp;
	c->guest_rsp_alt = guest_rsp_alt;
	c->dispatch_syscall = dispatch;
}

typedef uintptr_t (MB_SYSV *call_guest_simple_fn)(uintptr_t entry, mb_context *c);

uintptr_t mb_call_guest_simple(uintptr_t entry, mb_context *c) {
	call_guest_simple_fn f = (call_guest_simple_fn)CALL_GUEST_SIMPLE_ADDR;
	return f(entry, c);
}

uintptr_t mb_get_callback_ptr(uintptr_t slot) {
	return EXTCALL_THUNK_ADDR + slot * 16;
}

/* ---- thunk manager (thunks.rs) ---- */
#define THUNK_SIZE 32
struct mb_thunks {
	mb_range mem;
	uintptr_t entries[MB_PAGESIZE / THUNK_SIZE];
	uintptr_t ptrs[MB_PAGESIZE / THUNK_SIZE];
	size_t count;
};

mb_thunks *mb_thunks_new(void) {
	mb_thunks *t = (mb_thunks *)calloc(1, sizeof(mb_thunks));
	mb_range in = { 0, MB_PAGESIZE };
	if (mb_pal_map_anon(in, MB_PROT_RWX, &t->mem) != 0) { free(t); return NULL; }
	return t;
}

void mb_thunks_free(mb_thunks *t) {
	if (!t) return;
	mb_pal_unmap_anon(t->mem);
	free(t);
}

static void emit8(uint8_t **p, uint8_t v) { *(*p)++ = v; }
static void emit64(uint8_t **p, uintptr_t v) { memcpy(*p, &v, 8); *p += 8; }

uintptr_t mb_thunks_get(mb_thunks *t, uintptr_t guest_entry, mb_context *c) {
	for (size_t i = 0; i < t->count; i++)
		if (t->entries[i] == guest_entry) return t->ptrs[i];
	if ((t->count + 1) * THUNK_SIZE > t->mem.size) return 0; /* no room */
	uintptr_t addr = t->mem.start + t->count * THUNK_SIZE;
	uint8_t *p = (uint8_t *)addr;
	emit8(&p, 0x49); emit8(&p, 0xba); emit64(&p, (uintptr_t)c);        /* mov r10, ctx */
	emit8(&p, 0x49); emit8(&p, 0xbb); emit64(&p, guest_entry);          /* mov r11, entry */
	emit8(&p, 0x48); emit8(&p, 0xb8); emit64(&p, CALL_GUEST_IMPL_ADDR); /* mov rax, impl */
	emit8(&p, 0xff); emit8(&p, 0xe0);                                   /* jmp rax */
	t->entries[t->count] = guest_entry;
	t->ptrs[t->count] = addr;
	t->count++;
	return addr;
}
