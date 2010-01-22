#include "dwarf_lines.h"
#include "local_panic.h"

bool lineprogram_regs_t::execute(address_t from, size_t& offset)
{
    uint8_t opcode = *reinterpret_cast<uint8_t*>(from + offset);
    ++offset;

    if (reset_to_initial)
        reset();

    if (reset_fields)
    {
        basic_block = false;
        prologue_end = false;
        epilogue_begin = false;
        reset_fields = false;
    }

    // Decode opcode into standard, extended or special and execute updating current information.
    if (opcode < header.opcode_base) // std opcode
    {
        switch (opcode)
        {
            case DW_LNS_copy:
                reset_fields = true;
                break;
            case DW_LNS_advance_pc:
            {
                uleb128_t operand;
                operand.decode(from, offset);
                address += operand * header.minimum_instruction_length;
                break;
            }
            case DW_LNS_advance_line:
            {
                sleb128_t operand;
                operand.decode(from, offset);
                line += operand;
                break;
            }
            case DW_LNS_set_file:
            {
                uleb128_t operand;
                operand.decode(from, offset);
                file = operand;
                break;
            }
            case DW_LNS_set_column:
            {
                uleb128_t operand;
                operand.decode(from, offset);
                column = operand;
                break;
            }
            case DW_LNS_negate_stmt:
                is_stmt = !is_stmt;
                break;
            case DW_LNS_set_basic_block:
                basic_block = true;
                break;
            case DW_LNS_const_add_pc:
                address += address_increment(255) * header.minimum_instruction_length;
                break;
            case DW_LNS_fixed_advance_pc:
            {
                uint16_t operand = *reinterpret_cast<uint16_t*>(from + offset);
                offset += sizeof(uint16_t);
                address += operand;
                break;
            }
            case DW_LNS_set_prologue_end:
                prologue_end = true;
                break;
            case DW_LNS_set_epilogue_begin:
                epilogue_begin = true;
                break;
            case DW_LNS_set_isa:
            {
                uleb128_t operand;
                operand.decode(from, offset);
                isa = operand;
                break;
            }
            case DW_LNS_extended_op:
            {
                uleb128_t ext_area_length;
                ext_area_length.decode(from, offset);
                size_t prev_offset = offset;
                uint8_t sub_opcode = *reinterpret_cast<uint8_t*>(from + offset);
                ++offset;
                switch (sub_opcode)
                {
                    case DW_LNE_end_sequence:
                        end_sequence = true;
                        reset_to_initial = true;
                        break;
                    case DW_LNE_set_address:
                    {
                        uint32_t operand = *reinterpret_cast<uint32_t*>(from + offset);
                        offset += sizeof(uint32_t);
                        address = operand;
                        break;
                    }
                    case DW_LNE_define_file:
                    {
                        lnp_filename_t fname;
                        fname.decode(from, offset);
                        header.file_names.push_back(fname);
                        break;
                    }
                    default:
                        printf("UNKNOWN EXTENDED OPCODE %x\n", sub_opcode);
                        return false;
                }
                if (offset != prev_offset + ext_area_length)
                {
                    printf("STREAM OUT OF SYNC! offset %x should be %x\n", offset, prev_offset + ext_area_length);
                    return false;
                }
            }
            default:
                printf("UNKNOWN OPCODE %x\n", opcode);
                return false;
        }
    }
    else // special opcode
    {
        opcode -= header.opcode_base;
        address += address_increment(opcode);
        line += line_increment(opcode);
    }

    return true;
}

void lnp_header_t::decode(address_t from, size_t& offset)
{
    unit_length = *reinterpret_cast<uint32_t*>(from + offset);
    if (unit_length == 0xffffffff)
        PANIC("DWARF64 is not supported!");
    offset += sizeof(uint32_t);
    version = *reinterpret_cast<uint16_t*>(from + offset);
    offset += sizeof(uint16_t);
    header_length = *reinterpret_cast<uint32_t*>(from + offset);
    offset += sizeof(uint32_t);
    line_program_start = from + offset + header_length;

    minimum_instruction_length = *reinterpret_cast<uint8_t*>(from + offset);
    offset += sizeof(uint8_t);
    default_is_stmt = *reinterpret_cast<uint8_t*>(from + offset);
    offset += sizeof(uint8_t);
    line_base = *reinterpret_cast<int8_t*>(from + offset);
    offset += sizeof(int8_t);
    line_range = *reinterpret_cast<uint8_t*>(from + offset);
    offset += sizeof(uint8_t);
    opcode_base = *reinterpret_cast<uint8_t*>(from + offset);
    offset += sizeof(uint8_t);

    for (int i = 1; i < opcode_base; i++)
    {
        uleb128_t size;
        size.decode(from, offset);
        standard_opcode_lengths.push_back(size);
    }

    char* p = reinterpret_cast<char*>(from + offset);
    while (*p)
    {
        char* str = p;
        while (*p++)
            ++offset;
        include_directories.push_back(std::string(str));
        ++p;
        ++offset;
    }
    ++offset;

    lnp_filename_t fname;
    while (fname.decode(from, offset))
        file_names.push_back(fname);
}

bool lnp_filename_t::decode(address_t from, size_t& offset)
{
    char* s = reinterpret_cast<char*>(from + offset);
    if (!*s)
    {
        ++offset;
        return false;
    }

    char* str = s;
    while (*s++)
        ++offset;
    ++offset;

    filename = std::string(str);
    directory_index.decode(from, offset);
    file_timestamp.decode(from, offset);
    file_bytes.decode(from, offset);

    return true;
}

dwarf_debug_lines_t::dwarf_debug_lines_t(address_t st, size_t sz)
    : start(st)
    , size(sz)
    , current_regs(header)
{
    size_t offset = 0;
    header.decode(start, offset);
    current_regs.reset();

    while (offset < size)
    {
        if (current_regs.execute(start, offset))
            state_matrix.push_back(current_regs);
    }
}

std::string dwarf_debug_lines_t::file_name(int /*index*/)
{
    return std::string("");
}

int dwarf_debug_lines_t::line_number(address_t /*address*/)
{
    return 0;
}