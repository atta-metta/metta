//
// Part of Metta OS. Check https://atta-metta.net for latest version.
//
// Copyright 2007 - 2017, Stanislav Karchebnyy <berkus@atta-metta.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "lexer.h"
#include <cstring>
#include "token.h"

lexer_t::lexer_t(bool be_verbose)
    : verbose(be_verbose)
{
}

lexer_t::lexer_t(const llvm::MemoryBuffer *StartBuf, symbol_table_t* sym, bool be_verbose)
    : verbose(be_verbose)
{
    init(StartBuf, sym);
}

void lexer_t::init(const llvm::MemoryBuffer *StartBuf, symbol_table_t* sym)
{
    cur_buf = StartBuf;
    cur_ptr = cur_buf->getBufferStart();
    symbols = sym;
    cur_kind = next_kind = token::none;
    token_val = 0;
}

int lexer_t::get_next_char()
{
    char cur_char = *cur_ptr++;
    switch (cur_char)
    {
        default: return (unsigned char)cur_char;
        case 0:
            // A nul character in the stream is either the end of the current buffer or
            // a random nul in the file.  Disambiguate that here.
            if (cur_ptr - 1 != cur_buf->getBufferEnd())
                return 0;  // Just whitespace.

            // Otherwise, return end of file.
            --cur_ptr;  // Another call to lex will return EOF again.
            return EOF;
    }
}

void lexer_t::skip_line_comment()
{
    while (true)
    {
        if (*cur_ptr == '\n' || *cur_ptr == '\r' || get_next_char() == EOF)
            return;
    }
}

void lexer_t::get_autodoc_line()
{
    token_start = cur_ptr; // Skip ##
    skip_line_comment();
}

token::kind lexer_t::get_token()
{
    if (next_kind != token::none)
    {
        token::kind t = next_kind;
        next_kind = token::none;
        return t;
    }

    token_start = cur_ptr;

    int cur_char = get_next_char();
    switch (cur_char)
    {
        case EOF: return token::eof;
        case 0: case ' ': case '\t': case '\n': case '\r': // Ignore whitespace.
            return get_token();
        case '#':
            if (get_next_char() == '#')
            {
                get_autodoc_line();
                return token::autodoc;
            }
            else
            {
                --cur_ptr;
                skip_line_comment();
            }
            return get_token();
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            return get_cardinal();
        case '=':
            if (get_next_char() == '>')
            {
                return token::dblarrow;
            }
            --cur_ptr;
            return token::equal;
        // @todo Does this make things nicer or only confuses?
        // i.e. method()->(int value);
        // case '-':
        // if (get_next_char() == '>')
        // {
        //     return token::kw_returns;
        // }
        // --cur_ptr; --cur_ptr;
        // return get_identifier();
        case ',': return token::comma;
        case '&': return token::reference;
        case '[': return token::lsquare;
        case ']': return token::rsquare;
        case '{': return token::lbrace;
        case '}': return token::rbrace;
        case '<': return token::less;
        case '>': return token::greater;
        case '(': return token::lparen;
        case ')': return token::rparen;
        case ';': return token::semicolon;
        case '\\': return token::backslash;
        default:
            return get_identifier();
    }
}

/// is_label_char - Return true for [a-zA-Z._0-9].
static bool is_label_char(char c)
{
    return isalnum(c) || c == '.' || c == '_';//TODO: add special handling for dotdot
}

/// is_number_char - Return true for [xX0-9].
static bool is_number_char(char c)
{
    return isdigit(c) || c == 'x' || c == 'X';
}

/// get_identifier: Handle several related productions:
///    keyword         interface, idempotent, ...
///    integer         [0-9]+
///    hex integer     0x[0-9A-Fa-f]+
token::kind lexer_t::get_identifier()
{
    const char *start_ptr = cur_ptr;

    for (; is_label_char(*cur_ptr); ++cur_ptr)
    {
    }

    --start_ptr;
    unsigned int len = cur_ptr - start_ptr;

    std::string symbol(start_ptr, len);
    symbol_table_t::iterator idx = symbols->lookup(symbol);
    if (idx == symbols->end())
        return token::identifier;
        // Parser will do insertions, as it has more information about types.
//         idx = symbols->insert(symbol, token::identifier); //FIXME: will put in types as identifiers too

    return symbols->kind(idx);
}

token::kind lexer_t::get_cardinal()
{
    const char *start_ptr = cur_ptr;

    for (; is_number_char(*cur_ptr) || isalnum(*cur_ptr) /* for other base numbers */; ++cur_ptr)
    {
    }

    --start_ptr;
    unsigned int len = cur_ptr - start_ptr;

    if (is_label_char(*cur_ptr))
        return token::error;

    std::string symbol(start_ptr, len);

    char* end;
    token_val = strtol(symbol.c_str(), &end, 0);

    if (*end == '\0')
        return token::cardinal;
    else
        return token::error;
}
