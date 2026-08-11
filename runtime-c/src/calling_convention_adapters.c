/* win64 <-> sysv64 calling-convention trampolines, exported for the managed
 * layer to bridge a win64 host and the always-sysv64 guest on Windows. Faithful
 * C port of runtime/src/calling_convention_adapters.rs. The target function
 * pointer is passed as a hidden argument in rax.
 *
 * On a Linux host these are unused (host and guest are both sysv64) but they
 * compile fine and are harmless. CROSS-COMPILE-CHECKED with mingw; the Windows
 * ABI path is not runtime-validated here.
 *
 * GCC's function attributes give the two ABIs: ms_abi == win64, sysv_abi ==
 * System V. Reading rax with an empty asm mirrors the reference's `asm!("",
 * out("rax") fp)` - technically the callee could have clobbered rax, but in
 * practice the compiler has not emitted anything before this point. */
#include <stdint.h>

#define WIN64 __attribute__((ms_abi))
#define SYSV  __attribute__((sysv_abi))

/* depart<N>: win64 entry that calls a sysv64 target (in rax) with N args. */
#define DEPART(n, params, args) \
	WIN64 uintptr_t depart##n params { \
		uintptr_t fp; __asm__("" : "=a"(fp)); \
		return ((SYSV uintptr_t (*) params)fp) args; \
	}

/* arrive<N>: sysv64 entry that calls a win64 target (in rax) with N args. */
#define ARRIVE(n, params, args) \
	SYSV uintptr_t arrive##n params { \
		uintptr_t fp; __asm__("" : "=a"(fp)); \
		return ((WIN64 uintptr_t (*) params)fp) args; \
	}

DEPART(0, (void), ())
DEPART(1, (uintptr_t a), (a))
DEPART(2, (uintptr_t a, uintptr_t b), (a, b))
DEPART(3, (uintptr_t a, uintptr_t b, uintptr_t c), (a, b, c))
DEPART(4, (uintptr_t a, uintptr_t b, uintptr_t c, uintptr_t d), (a, b, c, d))
DEPART(5, (uintptr_t a, uintptr_t b, uintptr_t c, uintptr_t d, uintptr_t e), (a, b, c, d, e))
DEPART(6, (uintptr_t a, uintptr_t b, uintptr_t c, uintptr_t d, uintptr_t e, uintptr_t f), (a, b, c, d, e, f))

ARRIVE(0, (void), ())
ARRIVE(1, (uintptr_t a), (a))
ARRIVE(2, (uintptr_t a, uintptr_t b), (a, b))
ARRIVE(3, (uintptr_t a, uintptr_t b, uintptr_t c), (a, b, c))
ARRIVE(4, (uintptr_t a, uintptr_t b, uintptr_t c, uintptr_t d), (a, b, c, d))
ARRIVE(5, (uintptr_t a, uintptr_t b, uintptr_t c, uintptr_t d, uintptr_t e), (a, b, c, d, e))
ARRIVE(6, (uintptr_t a, uintptr_t b, uintptr_t c, uintptr_t d, uintptr_t e, uintptr_t f), (a, b, c, d, e, f))
