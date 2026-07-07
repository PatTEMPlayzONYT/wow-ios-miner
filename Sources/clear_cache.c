// RandomX's AArch64 JIT (jit_compiler_a64.cpp) references the __clear_cache
// compiler builtin. On iOS that symbol isn't provided by the toolchain, so
// the link fails even though we run RandomX in interpreter mode and never
// actually use the JIT. Provide it via Apple's instruction-cache flush so
// the link resolves (and it's correct if JIT is ever enabled later).
#include <libkern/OSCacheControl.h>
#include <stddef.h>

void __clear_cache(void *begin, void *end) {
    sys_icache_invalidate(begin, (char *)end - (char *)begin);
}
