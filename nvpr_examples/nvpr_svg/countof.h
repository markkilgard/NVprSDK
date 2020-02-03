
/* countof.hpp - robust macro to count array elements. */

#ifndef __countof_hpp__
#define __countof_hpp__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#if !defined(UNALIGNED)
# if defined(_MSC_VER)
#  if defined(_M_IA64) || defined(_M_AMD64)
#   define UNALIGNED __unaligned
#  else
#   define UNALIGNED
#  endif
# else
#  define UNALIGNED
# endif
#endif
#if !defined(countof)
# if !defined(__cplusplus)
#  define countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#  define icountof(_Array) ((int)countof(_Array))
#  define uicountof(_Array) ((unsigned int)countof(_Array))
# else
#  include <cstddef>  // for size_t in C++
// Safe C++ countof macro
extern "C++"
{
template <typename CountofType, size_t SizeOfArray>
char (*countof_helper(UNALIGNED CountofType (&Array)[SizeOfArray]))[SizeOfArray];
#  define countof(Array) sizeof(*countof_helper(Array))
#  define icountof(_Array) (static_cast<int>(countof(_Array)))
#  define uicountof(_Array) (static_cast<int>(countof(_Array)))
}
# endif
#endif

#endif /* __countof__ */
