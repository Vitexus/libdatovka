#include "../config.h"

/*
 * _hidden macro marks library private symbols.
 * GCC can exclude them from the global symbols table.
 */
#if defined(__GNUC__) && (__GNUC__ >= 4) && !defined(_WIN32)
#define _hidden __attribute__((visibility("hidden")))
#else
#define _hidden
#endif

#if (defined HAVE_BUILTIN_EXPECT) && HAVE_BUILTIN_EXPECT
#  define LIKELY(x) __builtin_expect((x), 1)
#  define UNLIKELY(x) __builtin_expect((x), 0)
#else /* !HAVE_BUILTIN_EXPECT */
#  define LIKELY(x) (x)
#  define UNLIKELY(x) (x)
#endif /* HAVE_BUILTIN_EXPECT */
