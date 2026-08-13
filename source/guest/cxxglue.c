/* cxxglue.c - miniBox guest kit: symbols a C++ waterbox guest must supply itself.
 *
 * A guest is a static, non-PIE ELF at a fixed base, linked against the guest musl
 * but using the HOST gcc's libgcc/libgcc_eh (which are built against glibc). Two
 * things are therefore missing, and this object provides them. It is linked into
 * every C++ guest (see waterbox_guest_cxx_* in meson.build).
 */

/* Normally defined by crtbegin, which a freestanding guest does not link.
 * __cxa_atexit records it for static destructors. */
void *__dso_handle = &__dso_handle;

/* GCC 12's libgcc, built against glibc 2.35+, looks up exception tables through
 * _dl_find_object and - this is the part that matters - does NOT fall back to
 * dl_iterate_phdr when that call fails: it gives up and reports no FDE, which
 * makes the very first throw abort the guest through std::terminate. musl has no
 * _dl_find_object at all, so a C++ guest has to answer the question itself.
 *
 * Which it can, exactly: the guest is one static non-PIE object at a fixed base,
 * so everything the unwinder is asking about is in this image's own program
 * headers. Walk them once, cache the answer, and report it.
 */
#include <elf.h>
#include <link.h>
#include <stddef.h>

extern const Elf64_Ehdr __ehdr_start;

/* glibc's <dlfcn.h> layout (x86-64: no dbase, no count fields). */
struct mb_dl_find_object
{
	unsigned long long int dlfo_flags;
	void *dlfo_map_start;
	void *dlfo_map_end;
	void *dlfo_link_map;
	void *dlfo_eh_frame;
	unsigned long long int __dflo_reserved[7];
};

static struct
{
	int scanned;
	const char *map_start, *map_end;
	const void *eh_frame_hdr;
} mb_image;

static void mb_scan_phdrs(void)
{
	const Elf64_Phdr *ph = (const Elf64_Phdr *)((const char *)&__ehdr_start + __ehdr_start.e_phoff);
	const char *lo = (const char *)-1, *hi = 0;
	for (int i = 0; i < __ehdr_start.e_phnum; i++)
	{
		if (ph[i].p_type == PT_LOAD)
		{
			const char *start = (const char *)ph[i].p_vaddr;
			const char *end = start + ph[i].p_memsz;
			if (start < lo) lo = start;
			if (end > hi) hi = end;
		}
		else if (ph[i].p_type == PT_GNU_EH_FRAME)
		{
			mb_image.eh_frame_hdr = (const void *)ph[i].p_vaddr;
		}
	}
	mb_image.map_start = lo;
	mb_image.map_end = hi;
	mb_image.scanned = 1;
}

int _dl_find_object(void *address, struct mb_dl_find_object *result)
{
	if (!mb_image.scanned) mb_scan_phdrs();
	if (!mb_image.eh_frame_hdr) return -1;
	const char *pc = (const char *)address;
	if (pc < mb_image.map_start || pc >= mb_image.map_end) return -1;

	result->dlfo_flags = 0;
	result->dlfo_map_start = (void *)mb_image.map_start;
	result->dlfo_map_end = (void *)mb_image.map_end;
	result->dlfo_link_map = NULL;
	result->dlfo_eh_frame = (void *)mb_image.eh_frame_hdr;
	return 0;
}


/* Throwing across the guest also needs the unwinder to FIND the exception tables, and on a static
 * musl binary dl_iterate_phdr reads them out of the auxv - which the miniBox loader does not build,
 * because it is not an OS program loader and the guest is entered directly at its entry point. With
 * no auxv, musl's version reports zero program headers, libgcc finds no FDE for the throwing frame,
 * and any throw ends as std::terminate -> abort() -> a dead sandbox, however carefully the caller
 * wrapped it in try/catch.
 *
 * The guest is static and non-PIE at a fixed base, so the program headers are simply where the ELF
 * header says they are, and there is exactly one object to report. That makes this a three-line
 * answer to a question musl can only answer with help from a loader.
 */
int dl_iterate_phdr(int (*callback)(struct dl_phdr_info *info, size_t size, void *data), void *data)
{
	struct dl_phdr_info info;
	__builtin_memset(&info, 0, sizeof info);
	info.dlpi_addr = 0; /* no relocation: the image is linked at the address it runs at */
	info.dlpi_name = "core.wbx";
	info.dlpi_phdr = (const Elf64_Phdr *)((const char *)&__ehdr_start + __ehdr_start.e_phoff);
	info.dlpi_phnum = __ehdr_start.e_phnum;
	return callback(&info, sizeof info, data);
}
