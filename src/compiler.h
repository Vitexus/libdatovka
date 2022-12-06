#include "../config.h"

#if (defined HAVE_BUILTIN_EXPECT) && HAVE_BUILTIN_EXPECT
#  define LIKELY(x) __builtin_expect((x), 1)
#  define UNLIKELY(x) __builtin_expect((x), 0)
#else /* !HAVE_BUILTIN_EXPECT */
#  define LIKELY(x) (x)
#  define UNLIKELY(x) (x)
#endif /* HAVE_BUILTIN_EXPECT */
