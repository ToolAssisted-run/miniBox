/* Platform abstraction layer, Windows x86-64. Mirrors BizHawk waterboxhost src/memory_block/
 * pal.rs (the win module). CROSS-COMPILE-CHECKED with mingw-w64; not
 * runtime-validated on Linux. */
#ifdef _WIN32
#include "minibox_internal.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

static uint32_t prot_to_native(mb_prot prot) {
	switch (prot) {
		case MB_PROT_NONE:    return PAGE_NOACCESS;
		case MB_PROT_R:       return PAGE_READONLY;
		case MB_PROT_RW:      return PAGE_READWRITE;
		case MB_PROT_RX:      return PAGE_EXECUTE_READ;
		case MB_PROT_RWX:     return PAGE_EXECUTE_READWRITE;
		case MB_PROT_RWSTACK: return PAGE_READWRITE | PAGE_GUARD;
	}
	return PAGE_NOACCESS;
}

int mb_pal_open_handle(uintptr_t size, mb_handle *out) {
	HANDLE m = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_EXECUTE_READWRITE,
	                              (DWORD)(size >> 32), (DWORD)size, NULL);
	if (m == NULL) return -1;
	out->h = (uintptr_t)m;
	return 0;
}

void mb_pal_close_handle(mb_handle h) { CloseHandle((HANDLE)h.h); }

int mb_pal_map_handle(mb_handle h, mb_range in, mb_range *out) {
	void *p = MapViewOfFileEx((HANDLE)h.h, FILE_MAP_ALL_ACCESS | FILE_MAP_EXECUTE,
	                          0, 0, in.size, (void *)in.start);
	if (p == NULL) return -1;
	out->start = (uintptr_t)p;
	out->size = in.size;
	/* map_handle returns no-access memory; protect explicitly like the reference */
	DWORD old;
	VirtualProtect(p, in.size, PAGE_NOACCESS, &old);
	return 0;
}

void mb_pal_unmap_handle(mb_range addr) { UnmapViewOfFile((void *)addr.start); }

int mb_pal_map_anon(mb_range in, mb_prot prot, mb_range *out) {
	void *p = VirtualAlloc((void *)in.start, in.size, MEM_RESERVE | MEM_COMMIT, prot_to_native(prot));
	if (p == NULL) {
		/* Callers abort on failure, and an aborting GUI process leaves the user
		 * with an instant silent exit. Say what failed before that happens: the
		 * address, the size, and what Windows thought of it. */
		MEMORY_BASIC_INFORMATION mbi;
		DWORD err = GetLastError();
		if (in.start && VirtualQuery((void *)in.start, &mbi, sizeof mbi)) {
			fprintf(stderr, "miniBox: VirtualAlloc(%p, %llu) failed, error %lu "
			                "(that address is currently state=%lx protect=%lx type=%lx)\n",
			        (void *)in.start, (unsigned long long)in.size, (unsigned long)err,
			        (unsigned long)mbi.State, (unsigned long)mbi.Protect, (unsigned long)mbi.Type);
		} else {
			fprintf(stderr, "miniBox: VirtualAlloc(%p, %llu) failed, error %lu\n",
			        (void *)in.start, (unsigned long long)in.size, (unsigned long)err);
		}
		fflush(stderr);
		return -1;
	}
	out->start = (uintptr_t)p;
	out->size = in.size;
	return 0;
}

void mb_pal_unmap_anon(mb_range addr) { VirtualFree((void *)addr.start, 0, MEM_RELEASE); }

int mb_pal_protect(mb_range addr, mb_prot prot) {
	DWORD old;
	return VirtualProtect((void *)addr.start, addr.size, prot_to_native(prot), &old) ? 0 : -1;
}

/* Query a region for RWStack dirtiness: the guard bit is cleared once the page
 * has been written (the guard trip auto-clears it). */
int mb_pal_get_stack_dirty(uintptr_t start, uintptr_t *out_size, bool *out_dirty) {
	MEMORY_BASIC_INFORMATION mbi;
	if (VirtualQuery((void *)start, &mbi, sizeof(mbi)) != sizeof(mbi)) return -1;
	*out_size = (uintptr_t)mbi.RegionSize;
	*out_dirty = (mbi.Protect & PAGE_GUARD) == 0;
	return 0;
}
#endif
