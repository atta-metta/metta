//
// Part of Metta OS. Check https://atta-metta.net for latest version.
//
// Copyright 2007 - 2017, Stanislav Karchebnyy <berkus@atta-metta.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

/**
 * @brief Bochs Graphics Adapter related registers.
 */
namespace bga_card
{

/**
 * These values come directly from Bochs 2.4.5 iodev/vga.h
 */
enum {
    VBE_DISPI_TOTAL_VIDEO_MEMORY_MB  = 16,
    VBE_DISPI_4BPP_PLANE_SHIFT       = 22,
    VBE_DISPI_BANK_ADDRESS           = 0xA0000,
    VBE_DISPI_BANK_SIZE_KB           = 64,
    VBE_DISPI_MAX_XRES               = 2560,
    VBE_DISPI_MAX_YRES               = 1600,
    VBE_DISPI_MAX_BPP                = 32,
    VBE_DISPI_IOPORT_INDEX           = 0x01CE,
    VBE_DISPI_IOPORT_DATA            = 0x01CF,
    VBE_DISPI_INDEX_ID               = 0x0,
    VBE_DISPI_INDEX_XRES             = 0x1,
    VBE_DISPI_INDEX_YRES             = 0x2,
    VBE_DISPI_INDEX_BPP              = 0x3,
    VBE_DISPI_INDEX_ENABLE           = 0x4,
    VBE_DISPI_INDEX_BANK             = 0x5,
    VBE_DISPI_INDEX_VIRT_WIDTH       = 0x6,
    VBE_DISPI_INDEX_VIRT_HEIGHT      = 0x7,
    VBE_DISPI_INDEX_X_OFFSET         = 0x8,
    VBE_DISPI_INDEX_Y_OFFSET         = 0x9,
    VBE_DISPI_INDEX_VIDEO_MEMORY_64K = 0xa,
    VBE_DISPI_ID0                    = 0xB0C0,
    VBE_DISPI_ID1                    = 0xB0C1,
    VBE_DISPI_ID2                    = 0xB0C2,
    VBE_DISPI_ID3                    = 0xB0C3,
    VBE_DISPI_ID4                    = 0xB0C4,
    VBE_DISPI_ID5                    = 0xB0C5,
    VBE_DISPI_BPP_4                  = 0x04,
    VBE_DISPI_BPP_8                  = 0x08,
    VBE_DISPI_BPP_15                 = 0x0F,
    VBE_DISPI_BPP_16                 = 0x10,
    VBE_DISPI_BPP_24                 = 0x18,
    VBE_DISPI_BPP_32                 = 0x20,
    VBE_DISPI_DISABLED               = 0x00,
    VBE_DISPI_ENABLED                = 0x01,
    VBE_DISPI_GETCAPS                = 0x02,
    VBE_DISPI_8BIT_DAC               = 0x20,
    VBE_DISPI_LFB_ENABLED            = 0x40,
    VBE_DISPI_NOCLEARMEM             = 0x80,
    VBE_DISPI_LFB_PHYSICAL_ADDRESS   = 0xE0000000,
};

} // namespace bga_card
