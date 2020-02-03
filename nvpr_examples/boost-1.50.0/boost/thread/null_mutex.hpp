#ifndef BOOST_THREAD_PTHREAD_NULL_MUTEX_HPP
#define BOOST_THREAD_PTHREAD_NULL_MUTEX_HPP

//  (C) Copyright 2006-8 Anthony Williams
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy a
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/thread_time.hpp>

namespace boost
{
    class null_mutex
    {
    public:
        null_mutex()
        {
        }

        ~null_mutex()
        {
        }

        void lock_shared()
        {
        }

        bool try_lock_shared()
        {
            return true;
        }

        bool timed_lock_shared(system_time const& timeout)
        {
            return true;
        }

        template<typename TimeDuration>
        bool timed_lock_shared(TimeDuration const & relative_time)
        {
            return timed_lock_shared(get_system_time()+relative_time);
        }

        void unlock_shared()
        {
        }

        void lock()
        {
        }

        bool timed_lock(system_time const& timeout)
        {
            return true;
        }

        template<typename TimeDuration>
        bool timed_lock(TimeDuration const & relative_time)
        {
            return timed_lock(get_system_time()+relative_time);
        }

        bool try_lock()
        {
            return true;
        }

        void unlock()
        {
        }

        void lock_upgrade()
        {
        }

        bool timed_lock_upgrade(system_time const& timeout)
        {
            return true;
        }

        template<typename TimeDuration>
        bool timed_lock_upgrade(TimeDuration const & relative_time)
        {
            return timed_lock(get_system_time()+relative_time);
        }

        bool try_lock_upgrade()
        {
            return true;
        }

        void unlock_upgrade()
        {
        }

        void unlock_upgrade_and_lock()
        {
        }

        void unlock_and_lock_upgrade()
        {
        }

        void unlock_and_lock_shared()
        {
        }

        void unlock_upgrade_and_lock_shared()
        {
        }
    };
}

#endif
