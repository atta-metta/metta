//
// Copyright 2007 - 2008, Stanislav Karchebnyy <berkus+metta@madfire.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "types.h"
#include "lockable.h"
#include "atomic_count.h"
#include "object_types.h"

namespace metta {
namespace kernel {

/// thread *target = object_cast<thread>(th);
// template <typename T>
// T *object_cast(object *ptr)
// {
//     return ptr->type() == T::type ? (T*)ptr : 0;
// }

/**
* All kernel entities are objects and have this common fields at their start.
*
* Common system part of kernel objects.
* Provides locking and allocation management.
**/
class object : public lockable
{
public:
    /**
    * Reference this object from the outside, increasing its reference count
    * and deferring its death.
    **/
    inline void ref() { refs++; }
    /**
    * Unreference this object, when reference count goes down to zero
    * object will rest in peace.
    **/
    inline void unref() { refs--; }

    inline bool active() { return active_; }

    hash_t hash();

private:
    /** Object type */
    object_type  type;
    /**
    * @c true if this object is currently active;
    * @c false if it has been destroyed and is merely waiting for all references to it to be dropped.
    **/
    bool         active_;
    /** Count of outstanding references to this object.  */
    atomic_count refs;
    /**
    * Mutex lock protecting this object.
    * This mutex covers all data in the basic part of the object,
    * and may also protect some or all of the type-specific data.
    **/
//     mutex        lock; // provided by lockable
    /** Physical address at which the user part of the object is located.  */
    address_t    user_pa;
    /** Hash value. Set at object creation time, exported to user via get_state.  */
    hash_t       user_hash;

    security_id_t     sid;

    char*             label;
};

}
}

// kate: indent-width 4; replace-tabs on;
// vi:set ts=4:set expandtab=on:
