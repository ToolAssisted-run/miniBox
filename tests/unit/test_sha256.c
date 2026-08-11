/* SHA-256 known-answer tests (FIPS 180-4 vectors). */
#include "minibox_internal.h"
#include "test_util.h"

static void hex(const uint8_t *d, char *out) {
	static const char *h = "0123456789abcdef";
	for (int i = 0; i < 32; i++) { out[i*2] = h[d[i]>>4]; out[i*2+1] = h[d[i]&15]; }
	out[64] = 0;
}

static void check_vec(const char *msg, size_t len, const char *want) {
	mb_sha256 c; uint8_t out[32]; char got[65];
	mb_sha256_init(&c);
	mb_sha256_update(&c, msg, len);
	mb_sha256_final(&c, out);
	hex(out, got);
	CHECK(strcmp(got, want) == 0);
	if (strcmp(got, want) != 0) fprintf(stderr, "    got %s\n    want %s\n", got, want);
}

static void test_empty(void) {
	check_vec("", 0, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}
static void test_abc(void) {
	check_vec("abc", 3, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}
static void test_long(void) {
	check_vec("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56,
		"248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
}
static void test_streaming(void) {
	/* one million 'a' fed in odd chunks must match the standard vector */
	mb_sha256 c; uint8_t out[32]; char got[65];
	mb_sha256_init(&c);
	char chunk[997]; memset(chunk, 'a', sizeof(chunk));
	size_t remaining = 1000000;
	while (remaining) { size_t take = remaining < sizeof(chunk) ? remaining : sizeof(chunk); mb_sha256_update(&c, chunk, take); remaining -= take; }
	mb_sha256_final(&c, out); hex(out, got);
	CHECK(strcmp(got, "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0") == 0);
}

static void run_all(void) {
	RUN(test_empty); RUN(test_abc); RUN(test_long); RUN(test_streaming);
}
TEST_MAIN()
