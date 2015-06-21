// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_THREADING_HPP_INCLUDED
#define FOONATHAN_MEMORY_THREADING_HPP_INCLUDED

/// \file
/// \brief Mutexes and utilities to synchronize allocators.

#include <mutex>

#include "allocator_traits.hpp"
#include "config.hpp"

namespace foonathan { namespace memory
{
    /// \brief A dummy mutex class that does not lock anything.
    /// \details It serves the \c Mutex concept. Use it to disable locking for adapters.
    /// \ingroup memory
    struct dummy_mutex
    {
        void lock() FOONATHAN_NOEXCEPT {}
        bool try_lock() FOONATHAN_NOEXCEPT {return true;}
        void unlock() FOONATHAN_NOEXCEPT {}
    };

    /// \brief The default mutex used by \ref allocator_reference.
    /// \details It is \c std::mutex if \ref FOONATHAN_MEMORY_THREAD_SAFE_REFERENCE is \c true, \ref dummy_mutex otherwise.
    /// \ingroup memory
#if FOONATHAN_MEMORY_THREAD_SAFE_REFERENCE
    using default_mutex = std::mutex;
#else
    using default_mutex = dummy_mutex;
#endif

    namespace detail
    {
        // selects a mutex for an Allocator
        // stateless allocators don't need locking
        template <class RawAllocator, class Mutex>
        using mutex_for = typename std::conditional<allocator_traits<RawAllocator>::is_stateful::value,
                                                    Mutex, dummy_mutex>::type;

        // storage for mutexes to use EBO
        // it provides const lock/unlock function, inherit from it
        template <class Mutex>
        class mutex_storage
        {
        public:
            mutex_storage() FOONATHAN_NOEXCEPT = default;
            mutex_storage(const mutex_storage &) FOONATHAN_NOEXCEPT {}

            mutex_storage& operator=(const mutex_storage &) FOONATHAN_NOEXCEPT
            {
                return *this;
            }

            void lock() const
            {
                mutex_.lock();
            }

            void unlock() const FOONATHAN_NOEXCEPT
            {
                mutex_.unlock();
            }

        protected:
            ~mutex_storage() FOONATHAN_NOEXCEPT = default;
        private:
            mutable Mutex mutex_;
        };

        template <>
        class mutex_storage<dummy_mutex>
        {
        public:
            mutex_storage() FOONATHAN_NOEXCEPT = default;

            void lock() const FOONATHAN_NOEXCEPT {}
            void unlock() const FOONATHAN_NOEXCEPT {}
        protected:
            ~mutex_storage() FOONATHAN_NOEXCEPT = default;
        };

        // non changeable pointer to an Allocator that keeps a lock
        // I don't think EBO is necessary here...
        template <class Alloc, class Mutex>
        class locked_allocator
        {
        public:
            locked_allocator(Alloc &alloc, Mutex &m) FOONATHAN_NOEXCEPT
            : lock_(m), alloc_(&alloc) {}

            locked_allocator(locked_allocator &&other) FOONATHAN_NOEXCEPT
            : lock_(std::move(other.lock_)), alloc_(other.alloc_) {}

            Alloc& operator*() const FOONATHAN_NOEXCEPT
            {
                return *alloc_;
            }

            Alloc* operator->() const FOONATHAN_NOEXCEPT
            {
                return alloc_;
            }

        private:
            std::unique_lock<Mutex> lock_;
            Alloc *alloc_;
        };
    } // namespace detail
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_THREADING_HPP_INCLUDED
