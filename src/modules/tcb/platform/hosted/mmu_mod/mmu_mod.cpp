//
// Part of Metta OS. Check https://atta-metta.net for latest version.
//
// Copyright 2007 - 2017, Stanislav Karchebnyy <berkus@atta-metta.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
/**
 * Hosted mmu_mod is relatively more simple compared to other implementations,
 * it only needs to create some bookkeeping structures and maintain them in already
 * allocated memory.
 * The memory map abstraction is supported by the bootinfo page.
 */
#include "algorithm"
#include "default_console.h"
#include "bootinfo.h"
#include "infopage.h"
#include "mmu_module_v1_interface.h"
#include "mmu_module_v1_impl.h"
#include "mmu_v1_interface.h"
#include "mmu_v1_impl.h"
#include "ramtab_v1_interface.h"
#include "ramtab_v1_impl.h"
#include "system_frame_allocator_v1_interface.h"
#include "heap_v1_interface.h"
#include "stretch_allocator_v1_interface.h"
#include "nucleus.h"
#include "cpu.h"
#include "domain.h"
#include "stretch_v1_state.h"
#include "logger.h"

//======================================================================================================================
// mmu_v1 state
//======================================================================================================================

static const size_t N_L1_TABLES = 1024;
static const size_t N_L2_ENTRIES = 1024;

struct ramtab_entry_t
{
    address_t owner;        /* PHYSICAL address of owning domain's DCB   */
    uint16_t frame_width;   /* Logical width of the frame                */
    uint16_t state;         /* Misc bits, e.g. is_mapped, is_nailed, etc */
} PACKED;

struct pdom_st
{
    uint16_t               refcnt;  /* reference count on this pdom    */
    uint16_t               gen;     /* current generation of this pdom */
    stretch_v1::closure_t* stretch; /* handle on stretch (for destroy) */
};

struct pdom_t
{
    uint8_t rights[SID_MAX/2];
};

struct shadow_t
{
    sid_t sid;
    uint16_t flags;
} PACKED;

#define SHADOW(_va)  reinterpret_cast<shadow_t*>(reinterpret_cast<char*>(_va) + 4*KiB)

typedef uint8_t     l2_info;    /* free or used info for 1K L2 page tables */
#define L2FREE      (l2_info)0x12
#define L2USED      (l2_info)0x99

#define PDIDX(_pdid)   ((_pdid) & 0xffff)
#define PDIDX_MAX       0x80   /* Allow up to 128 protection domains */

struct mmu_v1::state_t
{
    shadow_t              l1_shadows[N_L1_TABLES]; /**< Level 1 shadows (4Mb pages) */

    mmu_v1::closure_t     mmu_closure;
    ramtab_v1::closure_t  ramtab_closure;

    uint32_t              next_pdidx;          /* Next free pdom idx (hint) */
    pdom_t*               pdom_tbl[PDIDX_MAX]; /* Map pdom idx to pdom_t's  */
    pdom_st               pdominfo[PDIDX_MAX]; /* Map pdom idx to pdom_st's */

    bool                  use_global_pages;    /* Set iff we can use PGE    */

    /*system_*/frame_allocator_v1::closure_t*  system_frame_allocator;
    heap_v1::closure_t*                        heap;
    stretch_allocator_v1::closure_t*           stretch_allocator;

//    uint32_t              n_frames;//FIXME: UNUSED!!?!

    address_t             l1_mapping_virt; /* virtual  address of l1 page table */
    address_t             l1_mapping_phys; /* physical address of l1 page table */

    address_t             l2_virt;   /* virtual  address of l2 array base */
    address_t             l2_phys;   /* physical address of l2 array base */

    address_t             l1_virt_virt; /* virtual address of l2 PtoV table  */

    ramtab_entry_t*       ramtab;      /* base of ram table            */
    size_t                ramtab_size; /* size of ram table            */

    uint32_t              l2_max;   /* index of the last available chunk   */
    uint32_t              l2_next;  /* index of first potential free chunk */
    l2_info               info[0];  /* free/used L2 info; actually l2_max entries   */
};

//======================================================================================================================
// helper methods
//======================================================================================================================

inline uint16_t alloc_pdidx(mmu_v1::state_t* state)
{
    uint32_t i = state->next_pdidx;
    kconsole << __FUNCTION__ << ": next_pdidx " << i << endl;
    do {
        if (state->pdom_tbl[i] == NULL)
        {
            state->next_pdidx = (i + 1) % PDIDX_MAX;
            kconsole << __FUNCTION__ << ": allocate next_pdidx " << i << endl;
            return i;
        }
        i = (i + 1) % PDIDX_MAX;
        kconsole << __FUNCTION__ << ": next_pdidx " << i << endl;
    } while(i != state->next_pdidx);

    kconsole << "alloc_pdidx: out of identifiers!" << endl;
    nucleus::debug_stop();
    return 0xdead;
}

//======================================================================================================================
// mmu_v1 methods
//======================================================================================================================

static void mmu_v1_start(mmu_v1::closure_t* self, protection_domain_v1::id root_domain)
{
    // nucleus::flush_tlb();
    // nucleus::wrpdom(base);
}

static void mmu_v1_add_range(mmu_v1::closure_t* self, stretch_v1::closure_t* str, memory_v1::virtmem_desc mem_range, stretch_v1::rights global_rights)
{
}

static void mmu_v1_add_mapped_range(mmu_v1::closure_t* self, stretch_v1::closure_t* str, memory_v1::virtmem_desc mem_range, memory_v1::physmem_desc pmem, stretch_v1::rights global_rights)
{
}

/**
 * Note: update cannot currently modify mappings, and expects that the virtual range contains valid PFNs already.
 */
static void mmu_v1_update_range(mmu_v1::closure_t* self, stretch_v1::closure_t* str, memory_v1::virtmem_desc mem_range, stretch_v1::rights global_rights)
{
}

static void mmu_v1_free_range(mmu_v1::closure_t* self, memory_v1::virtmem_desc mem_range)
{

}

static protection_domain_v1::id mmu_v1_create_domain(mmu_v1::closure_t* self)
{
    auto state = self->d_state;

    uint16_t idx = alloc_pdidx(state);

    state->pdominfo[idx].refcnt = 0;
    state->pdominfo[idx].stretch = state->stretch_allocator->create(sizeof(pdom_t), 0/*stretch_v1::rights_none*/);
    state->pdominfo[idx].gen++;

    memory_v1::size sz;
    pdom_t* base = reinterpret_cast<pdom_t*>(state->pdominfo[idx].stretch->info(&sz));
    memutils::clear_memory(base, sz);

    state->pdom_tbl[idx] = base;

    // Construct the pdid from the generation and the index.
    protection_domain_v1::id pdid = (uint32_t(state->pdominfo[idx].gen) << 16) | idx;
    kconsole << __FUNCTION__ << ": generated new pdid " << pdid << endl;
    return pdid;
}

static void mmu_v1_retain_domain(mmu_v1::closure_t* self, protection_domain_v1::id dom_id)
{
    auto state = self->d_state;
    uint16_t idx = PDIDX(dom_id);

    if ((idx >= PDIDX_MAX) || (state->pdom_tbl[idx] == NULL))
    {
        kconsole << __FUNCTION__ << ": bogus pdom id " << dom_id << endl;
        nucleus::debug_stop();
        return;
    }

    state->pdominfo[idx].refcnt++;
}

static void mmu_v1_release_domain(mmu_v1::closure_t* self, protection_domain_v1::id dom_id)
{
    auto state = self->d_state;
    uint16_t idx = PDIDX(dom_id);

    if ((idx >= PDIDX_MAX) || (state->pdom_tbl[idx] == NULL))
    {
        kconsole << __FUNCTION__ << ": bogus pdom id " << dom_id << endl;
        nucleus::debug_stop();
        return;
    }

    if (state->pdominfo[idx].refcnt)
        state->pdominfo[idx].refcnt--;

    if (state->pdominfo[idx].refcnt == 0)
    {
        state->stretch_allocator->destroy_stretch(state->pdominfo[idx].stretch);
        state->pdom_tbl[idx] = NULL;
    }
}

static void mmu_v1_set_rights(mmu_v1::closure_t* self, protection_domain_v1::id dom_id, stretch_v1::closure_t* str, stretch_v1::rights rights)
{
    auto state = self->d_state;
    uint16_t idx = PDIDX(dom_id);

    if ((idx >= PDIDX_MAX) || (state->pdom_tbl[idx] == NULL))
    {
        kconsole << __FUNCTION__ << ": bogus pdom id " << dom_id << endl;
        nucleus::debug_stop();
        return;
    }

    pdom_t* pdom = state->pdom_tbl[idx];
    sid_t sid = str->d_state->sid;

    kconsole << __FUNCTION__ << ": pdom " << pdom << ", sid " << sid << " " << rights << endl;

    uint8_t mask = sid & 1 ? 0xf0 : 0x0f;
    uint32_t val = rights;
    if (sid & 1) val <<= 4;
    pdom->rights[sid>>1] &= ~mask;
    pdom->rights[sid>>1] |= val;

    // Want to invalidate all non-global TB entries, but we can't
    // do that on Intel so just blow away the whole thing.
    // nucleus::flush_tlb();
}

static stretch_v1::rights mmu_v1_query_rights(mmu_v1::closure_t* self, protection_domain_v1::id dom_id, stretch_v1::closure_t* str)
{
    return 0;
}

// No ASN supported on x86.
static int32_t mmu_v1_query_asn(mmu_v1::closure_t* self, protection_domain_v1::id dom_id)
{
    return 0x666;
}

static stretch_v1::rights mmu_v1_query_global_rights(mmu_v1::closure_t* self, stretch_v1::closure_t* str)
{
    return 0;
}

static void mmu_v1_clone_rights(mmu_v1::closure_t* self, stretch_v1::closure_t* tmpl, stretch_v1::closure_t* str)
{

}

static const mmu_v1::ops_t mmu_v1_methods =
{
    mmu_v1_start,
    mmu_v1_add_range,
    mmu_v1_add_mapped_range,
    mmu_v1_update_range,
    mmu_v1_free_range,
    mmu_v1_create_domain,
    mmu_v1_retain_domain,
    mmu_v1_release_domain,
    mmu_v1_set_rights,
    mmu_v1_query_rights,
    mmu_v1_query_asn,
    mmu_v1_query_global_rights,
    mmu_v1_clone_rights
};

//======================================================================================================================
// ramtab_v1 methods
//======================================================================================================================

static memory_v1::size ramtab_v1_size(ramtab_v1::closure_t* self)
{
    mmu_v1::state_t* st = reinterpret_cast<mmu_v1::state_t*>(self->d_state);
    logger::trace() << __FUNCTION__ << ": ramtab state at " << st << ", returning size " << st->ramtab_size;
    return st->ramtab_size;
}

static memory_v1::address ramtab_v1_base(ramtab_v1::closure_t* self)
{
    mmu_v1::state_t* st = reinterpret_cast<mmu_v1::state_t*>(self->d_state);
    logger::trace() << __FUNCTION__ << ": ramtab state at " << st << ", returning base " << st->ramtab;
    return reinterpret_cast<memory_v1::address>(st->ramtab);
}

static void ramtab_v1_put(ramtab_v1::closure_t* self, uint32_t frame, uint32_t owner, uint32_t frame_width, ramtab_v1::state state)
{
    mmu_v1::state_t* st = reinterpret_cast<mmu_v1::state_t*>(self->d_state);
    logger::trace() << __FUNCTION__ << ": frame " << frame << " with owner " << owner << " and frame width " << int(frame_width) << " in state " << state;
    if (frame >= st->ramtab_size)
    {
        kconsole << __FUNCTION__ << ": out of range frame " << frame << ", max is " << st->ramtab_size << endl;
        nucleus::debug_stop();
        return;
    }

    st->ramtab[frame].owner = owner;
    st->ramtab[frame].frame_width = frame_width;
    st->ramtab[frame].state = state;
}

static uint32_t ramtab_v1_get(ramtab_v1::closure_t* self, uint32_t frame, uint32_t* frame_width, ramtab_v1::state* state)
{
    mmu_v1::state_t* st = reinterpret_cast<mmu_v1::state_t*>(self->d_state);
    if (frame >= st->ramtab_size)
    {
        kconsole << __FUNCTION__ << ": out of range frame " << frame << ", max is " << st->ramtab_size << endl;
        nucleus::debug_stop();
        return 0xdeadd00d;
    }

    *frame_width = st->ramtab[frame].frame_width;
    *state = ramtab_v1::state(st->ramtab[frame].state);
    logger::trace() << __FUNCTION__ << ": frame " << frame << " with owner " << st->ramtab[frame].owner << " and frame width " << int(*frame_width) << " in state " << *state;
    return st->ramtab[frame].owner;
}

static const ramtab_v1::ops_t ramtab_v1_methods =
{
    ramtab_v1_size,
    ramtab_v1_base,
    ramtab_v1_put,
    ramtab_v1_get
};

//======================================================================================================================
// mmu_module_v1 methods
//======================================================================================================================

/*
** Compute how much space is required initially for page tables:
** We do this currently by cycling through all initial mappings
** and setting a bit in a 1024-bit bitmap if a particular 4Mb
** chunk of the virtual address space requires a L2 page table.
** We then add a fixed number to this to allow for L2 allocation
** before we get frames and stretch allocators.
*/
static int bitmap_bit(address_t va)
{
    return (va >> 22) & 0x1f; // Each bit in each word represents 4Mb => 5 bits at 22 bit offset.
}

static int bitmap_index(address_t va)
{
    return va >> 27; // Each array word represents 32*4Mb = 128Mb => 27 bits.
}

#define N_EXTRA_L2S     32                     // We require an additional margin of L2 ptabs.

static size_t memory_required(bootinfo_t* bi, size_t& n_l2_tables)
{
    uint32_t bitmap[32] = { 0 };
    size_t nptabs = 0;

    std::for_each(bi->vmap_begin(), bi->vmap_end(), [&bitmap](const memory_v1::mapping* e)
    {
        kconsole << "Virtual mapping [" << e->virt << ", " << e->virt + (e->nframes << FRAME_WIDTH) << ") -> [" << e->phys << ", " << e->phys + (e->nframes << FRAME_WIDTH) << ")" << endl;
        for (size_t j = 0; j < e->nframes; ++j)
        {
	    	address_t va = e->virt + (j << FRAME_WIDTH);
	    	bitmap[bitmap_index(va)] |= 1 << bitmap_bit(va);
		}
    });

    /* Now scan through the bitmap to determine the number of L2s reqd */
    for (int i = 0; i < 32; ++i)
    {
		while (bitmap[i])
		{
	    	if (bitmap[i] & 1)
				nptabs++;
	    	bitmap[i] >>= 1;
		}
    }

    nptabs += N_EXTRA_L2S;
    n_l2_tables = nptabs;

    size_t res = sizeof(mmu_v1::state_t);   /* state includes the level 1 page table */

    // Account for L2 infos
    res += n_l2_tables * sizeof(l2_info);

    logger::debug() << "Got " << int(nptabs) << " nptabs";

    return res;
}

/*
** Compute how much space is required for the ram table; this is a
** system wide table which contains ownership information (and other
** misc. stuff) for each frame of 'real' physical memory.
*/
static size_t ramtab_required(bootinfo_t* bi, size_t& max_ramtab_entries)
{
	max_ramtab_entries = bi->find_usable_physical_memory_top() / PAGE_SIZE;
	return max_ramtab_entries * sizeof(ramtab_entry_t);
}

static inline bool is_non_cacheable(uint32_t type)
{
    return type == multiboot_t::mmap_entry_t::reserved
        || type == multiboot_t::mmap_entry_t::acpi_reclaimable
        || type == multiboot_t::mmap_entry_t::acpi_nvs
        || type == multiboot_t::mmap_entry_t::bad_memory;
}

static void enter_mappings(mmu_v1::state_t* state)
{
    bootinfo_t* bi = new(bootinfo_t::ADDRESS) bootinfo_t;
    std::for_each(bi->vmap_begin(), bi->vmap_end(), [bi, state](const memory_v1::mapping* e)
    {
        kconsole << "Virtual mapping [" << e->virt << ", " << e->virt + (e->nframes << FRAME_WIDTH) << ") -> [" << e->phys << ", " << e->phys + (e->nframes << FRAME_WIDTH) << ")" << endl;
        for (size_t j = 0; j < e->nframes; ++j)
        {
            address_t phys = e->phys + (j << FRAME_WIDTH);
            state->ramtab_closure.put(phys_frame_number(phys), OWNER_SYSTEM, FRAME_WIDTH, ramtab_v1::state_mapped);
        }
    });

    kconsole << " +-mmu_module_v1: enter_mappings required total of " << state->l2_next << " new l2 tables." << endl;
}

static mmu_v1::closure_t* mmu_module_v1_create(mmu_module_v1::closure_t* self, uint32_t initial_reservation, ramtab_v1::closure_t** ramtab, memory_v1::address* free)
{
    kconsole << " +-mmu_module_v1.create" << endl;
    bootinfo_t* bi = new(bootinfo_t::ADDRESS) bootinfo_t;

	size_t mmu_memory_needed_bytes = 0;

    // Calculate how much space is needed for the MMU structures.
    //    mmu_state,
    //    pagetables
    //    and ramtab
	size_t n_l2_tables = 0;
	size_t max_ramtab_entries = 0;
    size_t i;

	mmu_memory_needed_bytes = memory_required(bi, n_l2_tables);
	mmu_memory_needed_bytes = page_align_up(mmu_memory_needed_bytes);

    address_t ramtab_offset = mmu_memory_needed_bytes;

	mmu_memory_needed_bytes += ramtab_required(bi, max_ramtab_entries);
	mmu_memory_needed_bytes = page_align_up(mmu_memory_needed_bytes);

    address_t l2_tables_offset = mmu_memory_needed_bytes;

    //mmu_memory_needed_bytes += n_l2_tables * L2SIZE; // page-aligned by definition

	mmu_memory_needed_bytes += initial_reservation;
	mmu_memory_needed_bytes = page_align_up(mmu_memory_needed_bytes);

	address_t first_range = bi->find_highmem_range_of_at_least(mmu_memory_needed_bytes); // FIXME: PORT THIS

    // Find proper location to start "allocating" from.
	first_range = page_align_up(first_range);

	if (!bi->use_memory(first_range, mmu_memory_needed_bytes))
	{
        PANIC("Unable to use memory for initial MMU setup!");
	}

	kconsole << " +-mmu_module_v1: state allocated at " << first_range << endl;

	mmu_v1::state_t *state = reinterpret_cast<mmu_v1::state_t*>(first_range);
	mmu_v1::closure_t *cl = &state->mmu_closure;
	closure_init(cl, &mmu_v1_methods, state);

    //state->l1_mapping_virt = state->l1_mapping_phys = first_range;
    //state->l1_virt_virt = reinterpret_cast<address_t>(&state->l1_virt);

    kconsole << " +-mmu_module_v1: L1 phys table at va=" << state->l1_mapping_virt << ", pa=" << state->l1_mapping_phys << ", virt table at va=" << state->l1_virt_virt << endl;

    // Initialise the physical mapping to fault everything, & virtual to 'no trans'.
    for(i = 0; i < N_L1_TABLES; i++)
    {
	    state->l1_shadows[i].sid = SID_NULL;
	    state->l1_shadows[i].flags = 0;
    }

    // Initialise the ram table; it follows the state record immediately.
    state->ramtab_size = max_ramtab_entries;
    state->ramtab = reinterpret_cast<ramtab_entry_t*>(first_range + ramtab_offset);
    memutils::clear_memory(state->ramtab, state->ramtab_size * sizeof(ramtab_entry_t));

    closure_init(&state->ramtab_closure, &ramtab_v1_methods, reinterpret_cast<ramtab_v1::state_t*>(first_range));
    *ramtab = &state->ramtab_closure;

    kconsole << " +-mmu_module_v1: ramtab at " << state->ramtab << " with " << state->ramtab_size << " entries." << endl;

    // Initialise the protection domain tables
    state->next_pdidx = 0;
    for(i = 0; i < PDIDX_MAX; i++)
    {
	    state->pdom_tbl[i] = NULL;
	    state->pdominfo[i].refcnt  = 0;
	    state->pdominfo[i].gen     = 0;
	    state->pdominfo[i].stretch = NULL;
    }

    // And store a pointer to the pdom_tbl in the info page.
    INFO_PAGE.protection_domains = &(state->pdom_tbl);

    state->use_global_pages = (INFO_PAGE.cpu_features & X86_32_FEAT_PGE) != 0;

    // Intialise our closures, etc to NULL for now  // will be fixed by $Done later
    state->system_frame_allocator = NULL;
    state->heap = NULL;
    state->stretch_allocator = NULL;

    state->l2_virt  = page_align_up(first_range + l2_tables_offset);
    state->l2_phys  = page_align_up(state->l1_mapping_phys + l2_tables_offset);
    state->l2_max  = n_l2_tables;

    kconsole << " +-mmu_module_v1: " << static_cast<int>(state->l2_max) << " L2 tables at va=" << state->l2_virt << ", pa=" << state->l2_phys << endl;

    state->l2_next = 0;
    for(i = 0; i < state->l2_max; i++)
        state->info[i] = L2FREE;

    // Enter mappings for all the existing translations.
    enter_mappings(state); // this call uses mappings in bootinfo_page, so we need to set them up sooner or call enter_mappings() later, maybe in Done or Engage?

    // Swap over to our new page table!
    kconsole << " +-mmu_module_v1: setting pagetable to " << state->l1_mapping_virt << ", " << state->l1_mapping_phys << endl;
    nucleus::write_pdbr(state->l1_mapping_virt, state->l1_mapping_phys);
    kconsole << " +-mmu_module_v1: wrote new pdbr using syscall!" << endl;

    // And store some useful pointers in the PIP for user-level translation.
//    INFO_PAGE.l1_va  = st->va_l1;
//    INFO_PAGE.l2tab  = st->vtab_va;
    INFO_PAGE.mmu_ok = true;

    /* Sort out pointer to free space for caller */
    *free = first_range;// +  l2_tables_offset + n_l2_tables * L2SIZE;

    return cl;
}

static void mmu_module_v1_finish_init(mmu_module_v1::closure_t* self, mmu_v1::closure_t* mmu, frame_allocator_v1::closure_t* frames, heap_v1::closure_t* heap, stretch_allocator_v1::closure_t* sysalloc)
{
    /* We don't require much here; just squirrel away the closures */
    mmu->d_state->system_frame_allocator = frames;
    mmu->d_state->heap = heap;
    mmu->d_state->stretch_allocator = sysalloc;
}

static const mmu_module_v1::ops_t mmu_module_v1_methods =
{
    mmu_module_v1_create,
    mmu_module_v1_finish_init
};

static const mmu_module_v1::closure_t clos =
{
    &mmu_module_v1_methods,
    NULL // no state
};

EXPORT_CLOSURE_TO_ROOTDOM(mmu_module, v1, clos);
