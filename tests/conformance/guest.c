/* miniBox conformance guest - a minimal waterbox core proving the toolchain
 * (gcc + musl + emulibc + linkscript) and the host end to end. Not an emulator;
 * a deterministic integer "machine" whose whole state lives in savestated
 * memory, plus a sealed constant table, an invisible scratch buffer, a guest
 * heap allocation, a mounted-file read, and a host callback - so it exercises
 * every phase-1 mechanism. */
#include <emulibc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Savestated state (plain globals -> .bss / savestated memory). */
static uint64_t g_acc;
static uint32_t g_step;
static uint8_t *g_heap;      /* malloc'd (sbrk) - savestated */

static uint32_t *g_table;    /* alloc_sealed - frozen after seal */
static uint32_t *g_scratch;  /* alloc_invisible - never savestated */

ECL_ENTRY void (*g_log_cb)(uint32_t value) = 0;
ECL_EXPORT void SetLogCallback(ECL_ENTRY void (*cb)(uint32_t)) { g_log_cb = cb; }

/* Reads an optional mounted "seed" file to prove the VFS + open/read syscalls. */
static uint32_t read_seed(void) {
	FILE *f = fopen("seed", "rb");
	if (!f) return 0x1234;
	uint32_t v = 0;
	fread(&v, 1, sizeof(v), f);
	fclose(f);
	return v;
}

ECL_EXPORT int Init(void) {
	g_table = (uint32_t *)alloc_sealed(256 * sizeof(uint32_t));
	if (!g_table) return 0;
	uint32_t seed = read_seed();
	for (int i = 0; i < 256; i++) g_table[i] = (uint32_t)((i + seed) * 2654435761u);
	g_scratch = (uint32_t *)alloc_invisible(1024 * sizeof(uint32_t));
	if (!g_scratch) return 0;
	g_heap = (uint8_t *)malloc(4096);   /* exercises brk/sbrk */
	if (!g_heap) return 0;
	memset(g_heap, 0, 4096);

	/* A big allocation takes musl past its mmap threshold, so this exercises
	 * mmap(NULL, ...) - the host picking an address and handing it back. That
	 * path returned a 32-bit-truncated address on Windows and nothing caught it,
	 * because every guest here only ever grew the heap with brk. Writing to the
	 * memory is the point: a truncated address faults immediately. */
	{
		const size_t big_size = 300u * 1024u;
		uint8_t *big = (uint8_t *)malloc(big_size);
		if (!big) return 0;
		memset(big, 0xA5, big_size);
		if (big[0] != 0xA5 || big[big_size - 1] != 0xA5) return 0;
		free(big);
	}
	g_acc = seed;
	g_step = 0;
	fprintf(stderr, "conformance guest: Init done (seed=%08x)\n", seed);
	return 1;
}

ECL_EXPORT uint32_t Step(uint32_t input) {
	for (int i = 0; i < 1024; i++) g_scratch[i] = input ^ g_table[(input + i) & 0xFF];
	uint32_t mixed = 0;
	for (int i = 0; i < 1024; i++) mixed += g_scratch[i];
	g_heap[g_step & 0xFFF] ^= (uint8_t)mixed;   /* touch the heap (savestated) */
	g_acc += (uint64_t)mixed * (g_step + 1) + g_heap[g_step & 0xFFF];
	g_step++;
	if (g_log_cb) g_log_cb((uint32_t)g_acc);
	return (uint32_t)g_acc;
}

ECL_EXPORT uint64_t GetAcc(void) { return g_acc; }
ECL_EXPORT uint32_t GetStep(void) { return g_step; }

int main(void) { return 0; }
