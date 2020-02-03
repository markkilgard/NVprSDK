//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2008-2008. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_PREPROCESSOR_HPP
#define BOOST_INTERPROCESS_DETAIL_PREPROCESSOR_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#ifdef BOOST_INTERPROCESS_PERFECT_FORWARDING
#error "This file is not needed when perfect forwarding is available"
#endif

#include <boost/preprocessor/iteration/local.hpp> 
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>

#define BOOST_INTERPROCESS_MAX_CONSTRUCTOR_PARAMETERS 10

//Note:
//We define template parameters as const references to
//be able to bind temporaries. After that we will un-const them.
//This cast is ugly but it is necessary until "perfect forwarding"
//is achieved in C++0x. Meanwhile, if we want to be able to
//bind rvalues with non-const references, we have to be ugly
#ifdef BOOST_INTERPROCESS_RVALUE_REFERENCE
   #define BOOST_INTERPROCESS_PP_PARAM_LIST(z, n, data) \
   BOOST_PP_CAT(P, n) && BOOST_PP_CAT(p, n) \
   //!
#else
   #define BOOST_INTERPROCESS_PP_PARAM_LIST(z, n, data) \
   const BOOST_PP_CAT(P, n) & BOOST_PP_CAT(p, n) \
   //!
#endif

#ifdef BOOST_INTERPROCESS_RVALUE_REFERENCE
   #define BOOST_INTERPROCESS_PARAM(U, u) \
   U && u \
   //!
#else
   #define BOOST_INTERPROCESS_PARAM(U, u) \
   const U & u \
   //!
#endif

#ifdef BOOST_INTERPROCESS_RVALUE_REFERENCE
#define BOOST_INTERPROCESS_AUX_PARAM_INIT(z, n, data) \
  BOOST_PP_CAT(m_p, n) (BOOST_PP_CAT(p, n))           \
//!
#else
#define BOOST_INTERPROCESS_AUX_PARAM_INIT(z, n, data) \
  BOOST_PP_CAT(m_p, n) (const_cast<BOOST_PP_CAT(P, n) &>(BOOST_PP_CAT(p, n))) \
//!
#endif

#define BOOST_INTERPROCESS_AUX_PARAM_INC(z, n, data)   \
  BOOST_PP_CAT(++m_p, n)                        \
//!

#ifdef BOOST_INTERPROCESS_RVALUE_REFERENCE
#define BOOST_INTERPROCESS_AUX_PARAM_DEFINE(z, n, data)  \
  BOOST_PP_CAT(P, n) && BOOST_PP_CAT(m_p, n);            \
//!
#else
#define BOOST_INTERPROCESS_AUX_PARAM_DEFINE(z, n, data)  \
  BOOST_PP_CAT(P, n) & BOOST_PP_CAT(m_p, n);             \
//!
#endif

#ifdef BOOST_INTERPROCESS_RVALUE_REFERENCE
   #define BOOST_INTERPROCESS_PP_PARAM_FORWARD(z, n, data) \
   detail::forward_impl< BOOST_PP_CAT(P, n) >( BOOST_PP_CAT(p, n) ) \
   //!
#else
   #define BOOST_INTERPROCESS_PP_PARAM_FORWARD(z, n, data) \
   BOOST_PP_CAT(p, n) \
   //!
#endif

#ifdef BOOST_INTERPROCESS_RVALUE_REFERENCE
   #define BOOST_INTERPROCESS_PP_MEMBER_FORWARD(z, n, data) \
   detail::forward_impl< BOOST_PP_CAT(P, n) >( BOOST_PP_CAT(m_p, n) ) \
   //!
#else
   #define BOOST_INTERPROCESS_PP_MEMBER_FORWARD(z, n, data) \
   BOOST_PP_CAT(m_p, n) \
   //!
#endif

#define BOOST_INTERPROCESS_PP_MEMBER_IT_FORWARD(z, n, data) \
BOOST_PP_CAT(*m_p, n) \
//!

#include <boost/interprocess/detail/config_end.hpp>

#else
#ifdef BOOST_INTERPROCESS_PERFECT_FORWARDING
#error "This file is not needed when perfect forwarding is available"
#endif
#endif //#ifndef BOOST_INTERPROCESS_DETAIL_PREPROCESSOR_HPP
