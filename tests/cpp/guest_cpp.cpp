/* C++ (Tier 2) guest: proves the STL works inside the sandbox.
 *
 * Exercises the things a real C++ core needs and that a naive port gets wrong:
 * heap-allocating containers, std::string, an associative container, <algorithm>
 * with a lambda, virtual dispatch, and a function-local static (which needs the
 * __cxa_guard_* runtime). Everything must live in guest memory, so the state
 * survives a whole-machine savestate round-trip - that is what the host test
 * checks after calling Mutate().
 *
 * Compiled with -fno-exceptions -fno-rtti (see waterbox_guest_cxxflags).
 */
#include <emulibc.h>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

namespace
{
	struct Base
	{
		virtual ~Base() = default;
		virtual int id() const { return 1; }
	};

	struct Derived : Base
	{
		int id() const override { return 41; }
	};

	std::vector<int> g_v;
	std::map<std::string, int> g_m;

	// function-local static: needs __cxa_guard_acquire/release
	int &counter()
	{
		static int c = 0;
		return c;
	}
}

extern "C" ECL_EXPORT int Init(void)
{
	for (int i = 0; i < 100; i++) g_v.push_back(i);
	std::sort(g_v.begin(), g_v.end(), [](int a, int b) { return a > b; });  // descending
	g_m[std::string("answer")] = g_v[0];                                   // 99
	Derived d;
	Base *b = &d;
	return b->id() + g_m["answer"];                                        // 41 + 99 = 140
}

/* Grows the containers, so a savestate taken before this and reloaded after must
 * restore the smaller sizes (proving STL heap lives in guest memory). */
extern "C" ECL_EXPORT void Mutate(void)
{
	for (int i = 0; i < 50; i++) g_v.push_back(1000 + i);
	g_m[std::string("extra")] = ++counter();
}

extern "C" ECL_EXPORT int Sizes(void)
{
	return (int)g_v.size() * 1000 + (int)g_m.size();  // 100*1000+1 = 100001 initially
}

int main() { return 0; }
