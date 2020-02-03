/* 
 * Copyright 2005 by NVIDIA Corporation.  All rights reserved.  All
 * information contained herein is proprietary and confidential to NVIDIA
 * Corporation.  Any use, reproduction, or disclosure without the written
 * permission of NVIDIA Corporation is prohibited.
 */

#ifndef __Cg_distance_hpp__
#define __Cg_distance_hpp__

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
static inline __CGvector<typename __CGtype_trait<T>::numericType,1> distance(const __CGvector<T,N> & a, 
                                                                             const __CGvector<T,N> & b)
{
    typedef typename __CGtype_trait<T>::numericType numericType;
    typedef typename __CGtype_trait<T>::dotType dotType;
    dotType distsq = (dotType(a[0]) - b[0])*(dotType(a[0]) - b[0]);
    for (int i=1; i<N; i++) {
        distsq += (dotType(a[i]) - b[i])*(dotType(a[i]) - b[i]);
    }
    return __CGvector<numericType,1>(numericType(::sqrt(distsq)));
}
template <typename TA, typename TB, int N, typename TAstore, typename TBstore>
static inline __CGvector<typename __CGtype_trait<TA,TB>::numericType,1> distance(const __CGvector_usage<TA,N,TAstore> & a,
                                                                                 const __CGvector_usage<TB,N,TBstore> & b)
{
    typedef typename __CGtype_trait<TA,TB>::numericType numericType;
    typedef typename __CGtype_trait<TA,TB>::dotType dotType;
    dotType distsq = (dotType(a[0]) - b[0])*(dotType(a[0]) - b[0]);
    for (int i=1; i<N; i++) {
        distsq += (dotType(a[i]) - b[i])*(dotType(a[i]) - b[i]);
    }
    return __CGvector<numericType,1>(numericType(::sqrt(distsq)));
}

#if defined(_MSC_VER) && !defined(__EDG__)  // Visual C++ but not EDG fakery
#pragma warning(pop)
#endif

} // namespace Cg

#endif // __Cg_distance_hpp__
