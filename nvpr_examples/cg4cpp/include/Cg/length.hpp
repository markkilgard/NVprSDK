/* 
 * Copyright 2005 by NVIDIA Corporation.  All rights reserved.  All
 * information contained herein is proprietary and confidential to NVIDIA
 * Corporation.  Any use, reproduction, or disclosure without the written
 * permission of NVIDIA Corporation is prohibited.
 */

#ifndef __Cg_length_hpp__
#define __Cg_length_hpp__

#ifdef __Cg_stdlib_hpp__
#pragma message("error: include this header file (" __FILE__ ") before <Cg/stdlib.hpp>")
#endif

#include <Cg/vector.hpp>

#include <math.h>  // for ::sqrt

namespace Cg {

#if defined(_MSC_VER) && !defined(__EDG__)  // Visual C++ but not EDG fakery
#pragma warning(push)
#pragma warning(disable:6294)  // Ill-defined for-loop:  initial condition does not satisfy test.  Loop body not executed.
#endif

template <typename T, int N>
static inline __CGvector<typename __CGtype_trait<T>::realType,1> length(const __CGvector<T,N> & v)
{
    typedef typename __CGtype_trait<T>::realType realType;
    typedef typename __CGtype_trait<T>::dotType dotType;
    dotType sum = dotType(v[0]) * v[0];
    for (int i=1; i<N; i++)
        sum += dotType(v[i]) * v[i];
    return __CGvector<realType,1>(realType(::sqrt(sum)));
}
template <typename T, int N, typename Tstore>
static inline __CGvector<typename __CGtype_trait<T>::realType,1> length(const __CGvector_usage<T,N,Tstore> & v)
{
    typedef typename __CGtype_trait<T>::realType realType;
    typedef typename __CGtype_trait<T>::dotType dotType;
    dotType sum = dotType(v[0]) * v[0];
    for (int i=1; i<N; i++)
        sum += dotType(v[i]) * v[i];
    return __CGvector<realType,1>(realType(::sqrt(sum)));
}

#if defined(_MSC_VER) && !defined(__EDG__)  // Visual C++ but not EDG fakery
#pragma warning(pop)
#endif

} // namespace Cg

#endif // __Cg_length_hpp__
