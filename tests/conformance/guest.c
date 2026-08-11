/* miniBox conformance guest - a minimal waterbox core proving the toolchain
 * (gcc + musl + emulibc + linkscript) and the host end to end. Not an emulator;
 * a deterministic integer "machine" whose whole state lives in savestated
 * memory, plus a sealed constant table and an invisible scratch buffer, so it
 * exercises every phase-1 mechanism. */
#include <emulibc.h>
#include <stdint.h>
#include <stdio.h>

/* Savestated state: a plain global (lands in .bss / savestated memory). */
static uint64_t g_acc;
static uint32_t g_step;

/* Sealed: a lookup table filled at init, read-only and frozen after seal. */
static uint32_t *g_table;      /* alloc_sealed */
/* Invisible: scratch that must never enter a savestate. */
static uint32_t *g_scratch;    /* alloc_invisible */

/* Guest->host callback slot (proves the extcall path if the host installs one). */
ECL_ENTRY void (*g_log_cb)(uint32_t value) = 0;

ECL_EXPORT void SetLogCallback(ECL_ENTRY void (*cb)(uint32_t)) { g_log_cb = cb; }

ECL_EXPORT int Init(void)
{
	g_table = (uint32_t *)alloc_sealed(256 * sizeof(uint32_t));
	if (!g_table) return 0;
	for (int i = 0; i < 256; i++) g_table[i] = (uint32_t)(i * 2654435761u); /* Knuth hash */
	g_scratch = (uint32_t *)alloc_invisible(1024 * sizeof(uint32_t));
	if (!g_scratch) return 0;
	g_acc = 0;
	g_step = 0;
	/* a syscall through the trampoline: write to stderr via musl stdio */
	fprintf(stderr, "conformance guest: Init done\n");
	return 1;
}

/* One deterministic step: mixes an input into the accumulator using the sealed
 * table, stages intermediate values in invisible scratch (written and read
 * within the call), and returns the new low 32 bits. */
ECL_EXPORT uint32_t Step(uint32_t input)
{
	for (int i = 0; i < 1024; i++) g_scratch[i] = input ^ g_table[(input + i) & 0xFF];
	uint32_t mixed = 0;
	for (int i = 0; i < 1024; i++) mixed += g_scratch[i];
	g_acc += (uint64_t)mixed * (g_step + 1);
	g_step++;
	if (g_log_cb) g_log_cb((uint32_t)g_acc);
	return (uint32_t)g_acc;
}

ECL_EXPORT uint64_t GetAcc(void) { return g_acc; }
ECL_EXPORT uint32_t GetStep(void) { return g_step; }

int main(void) { return 0; } /* never called; satisfies the CRT link */
