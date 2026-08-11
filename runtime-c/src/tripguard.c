/* Dirty-page fault handler (Linux). A logically-writable clean page is mapped
 * read-only; its first write faults here, we snapshot the pre-write content,
 * mark dirty, and reprotect writable. Faithful port of
 * runtime/src/memory_block/tripguard.rs (unix path). */
#define _GNU_SOURCE
#include "minibox_internal.h"
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ucontext.h>

/* Phase 1: single-threaded host, so a plain array + no lock is sufficient.
 * (The Rust reference guards a global block list with a mutex for multi-core
 * hosting; not needed until then.) */
#define MAX_BLOCKS 64
static mb_block *g_blocks[MAX_BLOCKS];
static int g_nblocks = 0;
static bool g_initialized = false;
static struct sigaction g_old_sa;

static uintptr_t mirror_of(const mb_block *b, uintptr_t guest) {
	return guest - b->addr.start + b->mirror.start;
}

/* returns true if handled */
static bool trip(uintptr_t addr) {
	mb_block *b = NULL;
	for (int i = 0; i < g_nblocks; i++)
		if (mb_range_contains(g_blocks[i]->addr, addr)) { b = g_blocks[i]; break; }
	if (!b) return false;
	uintptr_t page_start = addr & ~(uintptr_t)MB_PAGEMASK;
	size_t pi = (addr - b->addr.start) >> MB_PAGESHIFT;
	mb_page *p = &b->pages[pi];
	uint8_t s = p->status;
	if (!(s == MB_ST_RW || s == MB_ST_RWX || s == MB_ST_RWSTACK)) {
		__builtin_trap();
		return false;
	}
	mb_page_maybe_snapshot(p, mirror_of(b, page_start));
	p->dirty = true;
	mb_range r = { page_start, MB_PAGESIZE };
	if (mb_pal_protect(r, mb_page_native_prot(p)) != 0) { __builtin_trap(); abort(); }
	return true;
}

static void handler(int sig, siginfo_t *info, void *ucontext) {
	uintptr_t fault = (uintptr_t)info->si_addr;
	ucontext_t *uc = (ucontext_t *)ucontext;
	bool write = (uc->uc_mcontext.gregs[REG_ERR] & 2) != 0;
	bool rethrow = !write || !trip(fault);
	if (rethrow) {
		if (g_old_sa.sa_flags & SA_SIGINFO)
			g_old_sa.sa_sigaction(sig, info, ucontext);
		else if (g_old_sa.sa_handler == SIG_DFL || g_old_sa.sa_handler == SIG_IGN) {
			/* restore default and re-raise to get the normal crash */
			signal(sig, SIG_DFL);
			raise(sig);
		} else
			g_old_sa.sa_handler(sig);
	}
}

static void initialize(void) {
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = handler;
	sa.sa_flags = SA_ONSTACK | SA_SIGINFO;
	sigfillset(&sa.sa_mask);
	if (sigaction(SIGSEGV, &sa, &g_old_sa) != 0) { perror("miniBox sigaction"); abort(); }
}

void mb_tripguard_register(mb_block *b) {
	if (!g_initialized) { initialize(); g_initialized = true; }
	if (g_nblocks < MAX_BLOCKS) g_blocks[g_nblocks++] = b;
	else { fprintf(stderr, "miniBox: too many blocks registered\n"); abort(); }
}

void mb_tripguard_unregister(mb_block *b) {
	for (int i = 0; i < g_nblocks; i++)
		if (g_blocks[i] == b) { g_blocks[i] = g_blocks[--g_nblocks]; return; }
}
