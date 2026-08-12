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

/* glibc 2.35+ gives the unwinder a fast path for finding FDEs; musl has no
 * _dl_find_object, so libgcc_eh's reference would go unresolved. Returning
 * non-zero ("not found") makes libgcc fall back to dl_iterate_phdr, which musl
 * does provide - the path every pre-2.35 system used anyway.
 */
int _dl_find_object(void *address, void *result)
{
	(void)address;
	(void)result;
	return -1;
}
