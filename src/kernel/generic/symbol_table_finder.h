//
// Part of Metta OS. Check https://atta-metta.net for latest version.
//
// Copyright 2007 - 2017, Stanislav Karchebnyy <berkus@atta-metta.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <unordered_map>
#include "elf.h"
#include "panic.h"
#include "default_console.h"
#include "module_loader.h"

/**
 * Given only two ELF sections - a symbol table and a string table (plus a base for section offsets) find symbol by either name or value.
 */
class symbol_table_finder_t
{
    address_t base;
    elf32::section_header_t* symbol_table;
    elf32::section_header_t* string_table;

public:
    symbol_table_finder_t(address_t base_, elf32::section_header_t* symtab_, elf32::section_header_t* strtab_)
        : base(base_)
        , symbol_table(symtab_)
        , string_table(strtab_)
    {
        ASSERT(symbol_table);
        ASSERT(string_table);
        logger::debug() << "Symbol table finder starting: base = " << base << ", symtab = " << symbol_table << ", strtab = " << string_table;
    }

    symbol_table_finder_t(module_loader_t::module_entry& mod)
        : base(mod.load_base)
        , symbol_table(reinterpret_cast<elf32::section_header_t*>(mod.symtab_start))
        , string_table(reinterpret_cast<elf32::section_header_t*>(mod.strtab_start))
    {
        ASSERT(symbol_table);
        ASSERT(string_table);
        logger::debug() << "Symbol table finder starting: base = " << base << ", symtab = " << symbol_table << ", strtab = " << string_table;
    }

    // TODO: use debugging info if present
    cstring_t find_symbol(address_t addr, address_t* symbol_start)
    {
        address_t max = 0;
        elf32::symbol_t* fallback_symbol = 0;
        size_t n_entries = symbol_table->size / symbol_table->entsize;

        for (size_t i = 0; i < n_entries; i++)
        {
            elf32::symbol_t* symbol = reinterpret_cast<elf32::symbol_t*>(base + symbol_table->offset + i * symbol_table->entsize);

            if ((addr >= symbol->value) && (addr < symbol->value + symbol->size))
            {
                const char* c = reinterpret_cast<const char*>(base + string_table->offset + symbol->name);

                if (symbol_start)
                    *symbol_start = symbol->value;
                return c;
            }

            if (symbol->value > max && symbol->value <= addr)
            {
                max = symbol->value;
                fallback_symbol = symbol;
            }
        }

        // Search for symbol with size failed, now take a wild guess.
        // Use a biggest symbol value less than addr (if found).
        if (fallback_symbol)
        {
            const char* c = reinterpret_cast<const char*>(base + string_table->offset + fallback_symbol->name);

            if (symbol_start)
                *symbol_start = fallback_symbol->value;
            return c;
        }

        if (symbol_start)
            *symbol_start = 0;
        return NULL;
    }

    // Find symbol str in symbol table and return its absolute address.
    address_t find_symbol(cstring_t str)
    {
        size_t n_entries = symbol_table->size / symbol_table->entsize;
        logger::trace() << int(n_entries) << " symbols to consider.";
        logger::trace() << "Symbol table @ " << base + symbol_table->offset;
        logger::trace() << "String table @ " << base + string_table->offset;

        for (size_t i = 0; i < n_entries; i++)
        {
            elf32::symbol_t* symbol = reinterpret_cast<elf32::symbol_t*>(base + symbol_table->offset + i * symbol_table->entsize);
            const char* c = reinterpret_cast<const char*>(base + string_table->offset + symbol->name);

            logger::trace() << "Looking at symbol " << c << " @ " << symbol;
            if (str == c)
            {
                if (ELF32_ST_TYPE(symbol->info) == STT_SECTION)
                {
                    PANIC("FINDING SECTION NAMES UNSUPPORTED!");
                    return 0;
                    // return section_header(symbol->shndx)->vaddr; //offset + start();
                }
                else
                {
                    return symbol->value;
                }
            }
        }

        return 0;
    }

    /**
     * Return all symbols in a module with a given suffix.
     */
    module_symbols_t::symmap all_symbols(const char* suffix);
};
