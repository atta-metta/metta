//
// Part of Metta OS. Check https://atta-metta.net for latest version.
//
// Copyright 2007 - 2017, Stanislav Karchebnyy <berkus@atta-metta.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Minimal operator new/delete implementation.
//
#include <new>
#include "macros.h"
#include "memory.h"
#include "memutils.h"
#include "default_console.h"
// #include "debugger.h"
#include "panic.h"

void* operator new[](size_t size)
{
    return operator new(size);
}

void* operator new(size_t size)
{
	// debugger_t::print_backtrace(0, 0, 0);
    PANIC("Default new called!");
}

void operator delete(void*) throw()
{
    PANIC("Default delete called!");
}

void operator delete[](void*) throw()
{
    PANIC("Default delete[] called!");
}
