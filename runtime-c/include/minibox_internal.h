/* miniBox runtime (C port) - internal types and constants.
 * Phase 1: Linux x86-64, single guest thread, C guests. Faithful to
 * MACHINE-SPEC.md and to the Rust reference in runtime/.
 *
 * Derived from BizHawk's waterboxhost (MIT). See ../LICENSE, ../ATTRIBUTION.md.
 */
#ifndef MINIBOX_INTERNAL_H
#define MINIBOX_INTERNAL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define MB_PAGESIZE 0x1000u
#define MB_PAGEMASK 0xfffu
#define MB_PAGESHIFT 12

static inline uintptr_t mb_align_down(uintptr_t p) { return p & ~(uintptr_t)MB_PAGEMASK; }
static inline uintptr_t mb_align_up(uintptr_t p)   { return ((p - 1) | MB_PAGEMASK) + 1; }

/* A half-open address range [start, start+size). */
typedef struct { uintptr_t start; uintptr_t size; } mb_range;
static inline uintptr_t mb_range_end(mb_range r) { return r.start + r.size; }
static inline bool mb_range_contains(mb_range r, uintptr_t a) { return a >= r.start && a < mb_range_end(r); }
static inline mb_range mb_range_align_expand(mb_range r) {
	mb_range o; o.start = mb_align_down(r.start); o.size = mb_align_up(mb_range_end(r)) - o.start; return o;
}

/* Memory layout injected into the guest via __wbxsysinfo. Keep in sync with
 * emulibc's __WbxSysLayout and the Rust WbxSysLayout (8 {u64 start,u64 size}). */
typedef struct {
	mb_range elf, main_thread, alt_thread, sbrk, sealed, invis, plain, mmap_arena;
} mb_layout;
static inline mb_range mb_layout_all(const mb_layout *l) {
	mb_range r; r.start = l->elf.start; r.size = mb_range_end(l->mmap_arena) - l->elf.start; return r;
}

/* Host-supplied heap sizes (bytes, page-aligned). Mirrors MemoryLayoutTemplate. */
typedef struct { uintptr_t sbrk_size, sealed_size, invis_size, plain_size, mmap_size; } mb_layout_template;

/* Guest-visible protection of an allocated page. */
typedef enum { MB_PROT_NONE, MB_PROT_R, MB_PROT_RW, MB_PROT_RX, MB_PROT_RWX, MB_PROT_RWSTACK } mb_prot;

/* ---- PAL (pal_linux.c): thin wrappers over the OS. All ranges page-aligned. ---- */
typedef struct { int fd; } mb_handle;         /* memfd backing a block */
int      mb_pal_open_handle(uintptr_t size, mb_handle *out);   /* 0 ok */
void     mb_pal_close_handle(mb_handle h);
/* map_handle: start==0 -> OS chooses; else fixed. Returns actual range, no access. */
int      mb_pal_map_handle(mb_handle h, mb_range in, mb_range *out);
void     mb_pal_unmap_handle(mb_range addr);
int      mb_pal_map_anon(mb_range in, mb_prot prot, mb_range *out);
void     mb_pal_unmap_anon(mb_range addr);
int      mb_pal_protect(mb_range addr, mb_prot prot);          /* 0 ok */

/* ---- sha256.c ---- */
typedef struct { uint32_t h[8]; uint64_t len; uint8_t buf[64]; size_t fill; } mb_sha256;
void mb_sha256_init(mb_sha256 *c);
void mb_sha256_update(mb_sha256 *c, const void *data, size_t n);
void mb_sha256_final(mb_sha256 *c, uint8_t out[32]);

/* ---- Stream callbacks (match the wbx ReadCallback/WriteCallback ABI) ---- */
typedef intptr_t (*mb_read_cb)(uintptr_t userdata, uint8_t *data, uintptr_t size);
typedef int32_t  (*mb_write_cb)(uintptr_t userdata, const uint8_t *data, uintptr_t size);

/* ---- Memory block (memblock.c) ---- */
typedef enum { MB_SNAP_NONE, MB_SNAP_ZERO, MB_SNAP_DATA } mb_snap_kind;
typedef struct {
	uint8_t status;      /* MB_ST_* below (Free) or MB_ST_ALLOC | (prot+1) */
	bool dirty;
	bool invisible;
	mb_snap_kind snap_kind;
	uint8_t *snap_data;  /* MB_PAGESIZE bytes when snap_kind==DATA, else NULL */
} mb_page;

/* status byte encoding (also what page_info reports, minus dirty/invis bits) */
#define MB_ST_FREE      0x00
#define MB_ST_NONE      0x20  /* allocated, no access (guard) */
#define MB_ST_R         0x01
#define MB_ST_RW        0x03
#define MB_ST_RX        0x05
#define MB_ST_RWX       0x07
#define MB_ST_RWSTACK   0x13

typedef struct mb_block {
	mb_page *pages;
	size_t npages;
	mb_range addr;       /* fixed guest address range */
	mb_range mirror;     /* always-RW second mapping, OS-chosen */
	bool sealed;
	bool swapped_in;
	bool active;
	uint8_t hash[32];
	mb_handle handle;
} mb_block;

mb_block *mb_block_new(mb_range addr);
void      mb_block_free(mb_block *b);
void      mb_block_activate(mb_block *b);
void      mb_block_deactivate(mb_block *b);

/* syscall-shaped memory ops; return 0 on success or -errno. */
int  mb_block_mmap_fixed(mb_block *b, mb_range addr, mb_prot prot, bool no_replace);
long mb_block_mmap(mb_block *b, mb_range addr, mb_prot prot, mb_range arena, bool no_replace); /* addr or -errno */
int  mb_block_mprotect(mb_block *b, mb_range addr, mb_prot prot);
int  mb_block_munmap(mb_block *b, mb_range addr);
int  mb_block_madvise_dontneed(mb_block *b, mb_range addr);
long mb_block_mremap(mb_block *b, mb_range addr, uintptr_t new_size, mb_range arena);
int  mb_block_mark_invisible(mb_block *b, mb_range addr);
int  mb_block_copy_from_external(mb_block *b, const uint8_t *src, uintptr_t start, uintptr_t len);
int  mb_block_seal(mb_block *b);

size_t  mb_block_page_len(const mb_block *b);
uint8_t mb_block_page_info(const mb_block *b, size_t index);

/* Savestate (structure per MACHINE-SPEC.md section 6). Return 0 on success. */
int mb_block_save_state(mb_block *b, mb_write_cb w, uintptr_t ud);
int mb_block_load_state(mb_block *b, mb_read_cb r, uintptr_t ud);

/* ---- tripguard.c ---- */
void mb_tripguard_register(mb_block *b);
void mb_tripguard_unregister(mb_block *b);

/* Internal helpers shared with tripguard (memblock.c). */
mb_prot mb_page_native_prot(const mb_page *p);
void    mb_page_maybe_snapshot(mb_page *p, uintptr_t mirror_addr);

#endif
