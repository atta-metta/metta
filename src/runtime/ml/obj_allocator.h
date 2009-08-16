//
// Copyright 2007 - 2009, Renārs Lediņš <renars@madfire.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "minmax.h"
#include "obj_destructor.h"

template <typename Allocator>
struct obj_allocator
{
    typedef Allocator                       allocator;
    typedef typename allocator::value_type  value_type;
    typedef obj_destructor<value_type>      destructor;

    obj_allocator(const allocator& _allocator=allocator())
        : m_allocator(_allocator) {}

    value_type* allocate(value_type* old_mem, size_t old_size, size_t new_size)
    {
        value_type* ptr = m_allocator.allocate(0, 0, new_size);
        if (ptr && old_mem)
        {
            value_type* old_mem_ptr = old_mem,
                      * new_mem_ptr = ptr;
            size_t copy_size = min(old_size, new_size);
            while(copy_size--)
            {
                construct_inplace(new_mem_ptr++, *old_mem_ptr);
                destruct_inplace(old_mem_ptr++);
            }
            if (old_size > new_size)
            {
                size_t distance = old_size - new_size;
                while (distance--)
                    destruct_inplace(old_mem_ptr++);
            }
            m_allocator.deallocate(old_mem, old_size);
        }
        return ptr;
    }

    void deallocate(value_type* mem, size_t size)
    {
        destructor::destruct(mem, size);
        m_allocator.deallocate(mem, size);
    }

    allocator m_allocator;
};