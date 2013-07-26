#ifndef CGI_NEW_H
#define CGI_NEW_H

#include <cstdlib>
#include <cstddef>

/* CGI_INLINE is for forcing the inlining of inline functions */

#ifndef CGI_INLINE
# if defined(__GNUC__) && __GNUC__>=4
#  define CGI_INLINE __attribute__ ((always_inline)) inline
# else
#  ifdef _MSC_VER
#   define CGI_INLINE __forceinline
#  else
#   define CGI_INLINE inline
#  endif
# endif
#endif

#if !defined(CG_STATIC_LIBRARY)

// Provide our own operator new and delete so that there are no dependencies
// on the C++ stdlib.  These are forced inline to avoid inheriting visibility
// (shared library exports) from the C++ stdlib.  We've seen problems on Linux
// with libCg.so and libCgGL.so using each other's new and delete.

CGI_INLINE void *
operator new(std::size_t size)
{
  return std::malloc(size);
}

CGI_INLINE void
operator delete(void *ptr)
{
  std::free(ptr);
}

CGI_INLINE void *
operator new[](std::size_t size)
{
  return std::malloc(size);
}

CGI_INLINE void
operator delete[](void *ptr)
{
  std::free(ptr);
}

#endif
#endif
