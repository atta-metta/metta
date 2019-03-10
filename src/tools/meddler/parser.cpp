//
// Part of Metta OS. Check https://atta-metta.net for latest version.
//
// Copyright 2007 - 2017, Stanislav Karchebnyy <berkus@atta-metta.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "parser.h"
#include <iostream>
#include <sstream>
#include "ast.h"
#include <llvm/ADT/Twine.h>
// for trim():
#include <boost/algorithm/string.hpp>
using namespace std;
using namespace boost::algorithm;
// @todo: since we already depend on boost, why not replace parsing with something like boost::wave::idl?

std::string token_to_name(token::kind tok)
{
#define TNAME(tk) \
    case token::tk: return #tk;
    switch (tok)
    {
        TNAME(none)
        TNAME(eof)
        TNAME(error)
        TNAME(equal)
        TNAME(comma)
        TNAME(reference)
        TNAME(dblarrow)
        TNAME(lsquare)
        TNAME(rsquare)
        TNAME(lbrace)
        TNAME(rbrace)
        TNAME(less)
        TNAME(greater)
        TNAME(lparen)
        TNAME(rparen)
        TNAME(semicolon)
        TNAME(backslash)
        TNAME(type)
        TNAME(_builtin_type)
        TNAME(_interface_type)
        TNAME(_exception_type)
        TNAME(kw_local)
        TNAME(kw_final)
        TNAME(kw_interface)
        TNAME(kw_exception)
        TNAME(kw_in)
        TNAME(kw_inout)
        TNAME(kw_out)
        TNAME(kw_idempotent)
        TNAME(kw_raises)
        TNAME(kw_extends)
        TNAME(kw_never)
        TNAME(kw_returns)
        TNAME(kw_type)
        TNAME(kw_sequence)
        TNAME(kw_set)
        TNAME(kw_range)
        TNAME(kw_record)
        TNAME(kw_choice)
        TNAME(kw_on)
        TNAME(kw_enum)
        TNAME(kw_array)
        TNAME(identifier)
        TNAME(dotdot)
        TNAME(cardinal)
        TNAME(autodoc)
    }
    return "UNKNOWN";
}

parser_t::parser_t(llvm::SourceMgr& sm, bool be_verbose)
    : is_local(false)
    , is_final(false)
    , is_idempotent(false)
    , lex(be_verbose)
    , parse_tree(nullptr)
    , source_mgr(sm)
    , verbose(be_verbose)
{
}

void parser_t::init(const llvm::MemoryBuffer *F)
{
    is_local = is_final = is_idempotent = false;
    delete parse_tree; parse_tree = nullptr; // todo: unique_ptr<>
    lex.init(F, &symbols);
    populate_symbol_table();
}

void parser_t::reportError(std::string const& msg)
{
    source_mgr.PrintMessage(lex.current_loc(), llvm::SourceMgr::DK_Error, llvm::Twine(msg));
}

#define PARSE_ERROR(x) do { \
        std::stringstream _s; \
        _s << (x); \
        reportError(_s.str()); \
    } while (false);

// Store various id types in a symbol table.
void parser_t::populate_symbol_table()
{
    symbols.clear();
    symbols.insert("..", token::dotdot);
    symbols.insert("local", token::kw_local);
    symbols.insert("final", token::kw_final);
    symbols.insert("interface", token::kw_interface);
    symbols.insert("exception", token::kw_exception);
    symbols.insert("in", token::kw_in);
    symbols.insert("inout", token::kw_inout);
    symbols.insert("out", token::kw_out);
    symbols.insert("idempotent", token::kw_idempotent);
    symbols.insert("raises", token::kw_raises);
    symbols.insert("extends", token::kw_extends);
    symbols.insert("never", token::kw_never);
    symbols.insert("returns", token::kw_returns);
    symbols.insert("type", token::kw_type);
    symbols.insert("sequence", token::kw_sequence);
    symbols.insert("set", token::kw_set);
    symbols.insert("range", token::kw_range);
    symbols.insert("record", token::kw_record);
    symbols.insert("choice", token::kw_choice);
    symbols.insert("on", token::kw_on);
    symbols.insert("enum", token::kw_enum);
    symbols.insert("array", token::kw_array);
    symbols.insert("int8", token::_builtin_type);
    symbols.insert("int16", token::_builtin_type);
    symbols.insert("int32", token::_builtin_type);
    symbols.insert("int64", token::_builtin_type);
    symbols.insert("octet", token::_builtin_type);
    symbols.insert("card16", token::_builtin_type);
    symbols.insert("card32", token::_builtin_type);
    symbols.insert("card64", token::_builtin_type);
    symbols.insert("float", token::_builtin_type);
    symbols.insert("double", token::_builtin_type);
    symbols.insert("boolean", token::_builtin_type);
    symbols.insert("string", token::_builtin_type);
    symbols.insert("opaque", token::_builtin_type);
}

bool parser_t::run()
{
    lex.lex(); // prime the parser
    bool ret = parse_top_level_entities();
    if (verbose)
    {
        symbols.dump();
        if (ret)
            std::cout << "** PARSE SUCCESS" << std::endl;
        else
            std::cout << "** PARSE FAILURE" << std::endl;
    }
    return ret;
}

std::string parser_t::parent_interface()
{
    if (!parse_tree)
        return "<not known yet>"; // trick: return non-null string to make Meddler attempt a parse!
    return parse_tree->base;
}

void parser_t::append_autodoc(std::string docline)
{
    // Strip spaces at start and end.
    trim(docline);

    if (docline.empty())
        autodoc_buffer += "\n";
    else {
        if (autodoc_buffer.empty())
            autodoc_buffer = docline;
        else
            if (*autodoc_buffer.rbegin() == '\n')
                autodoc_buffer += docline;
            else
                autodoc_buffer += " " + docline;
    }
}

void parser_t::assign_autodoc(AST::node_t* node)
{
    if (verbose) std::cout << __FUNCTION__ << ": " << autodoc_buffer << std::endl;
    if (!autodoc_buffer.empty())
        node->set_autodoc(autodoc_buffer);
    autodoc_buffer = "";
}

#define D() if(verbose) std::cout << __FUNCTION__ << ": " << token_to_name(lex.token_kind()) << ": " << lex.current_token() << std::endl

//! module ::= full_interface_decl
//! full_interface_decl ::= local_interface_decl | final_interface_decl | interface_decl
bool parser_t::parse_top_level_entities()
{
    is_local = false;
    is_final = false;
    while (true) {
        D();
        switch (lex.token_kind())
        {
            default:         return false;//error("expected top-level entity");
            case token::eof: return false;
            case token::kw_local:
            case token::kw_final:
            case token::kw_interface:
                if (parse_interface())
                    return true;
                else
                    return false;
                break;
            case token::autodoc:
                append_autodoc(lex.current_token());
                lex.lex();
                break;
        }
    }
}

//! local_interface_decl ::= 'local' (final_interface_decl | interface_decl)
//! final_interface_decl ::= 'final' (local_interface_decl | interface_decl)
//! interface_decl ::= 'interface' id ['extends' id] '{' interface_body '}'
bool parser_t::parse_interface()
{
    D();
    if (lex.match(token::kw_local))
    {
        is_local = true;
        lex.lex();
        return parse_interface(); // expect final or interface
    }
    if (lex.match(token::kw_final))
    {
        is_final = true;
        lex.lex();
        return parse_interface(); // expect local or interface
    }
    if (lex.match(token::kw_interface))
    {
        if (!lex.expect(token::identifier))
            return false;

        AST::interface_t* node = new AST::interface_t(lex.current_token(), is_local, is_final);
        parse_tree = node;

        assign_autodoc(node);

        local_scope_t new_scope(symbols, lex.current_token());

        if (!symbols.insert_checked(node->name(), token::_interface_type))
        {
            PARSE_ERROR("duplicate symbol");
            return false;
        }

        if (lex.maybe(token::kw_extends))
        {
            lex.lex();
            if (lex.token_kind() != token::identifier)
            {
                PARSE_ERROR("'extends' needs interface id");
                return false;
            }
            node->set_parent(lex.current_token());
        }

        if (!lex.expect(token::lbrace))
        {
            PARSE_ERROR("{ expected");
            return false;
        }

        if (!parse_interface_body())
            return false;

        if (!lex.expect(token::rbrace))
        {
            PARSE_ERROR("} expected");
            return false;
        }

        if (verbose)
            node->dump("");

        return true;
    }
    PARSE_ERROR("unexpected");
    return false;
}

//! interface_body ::= (exception | typealias | full_method_decl)*
bool parser_t::parse_interface_body()
{
    D();
    while (lex.lex() != token::eof)
    {
        switch (lex.token_kind())
        {
            case token::rbrace: // end of interface declaration
                lex.lexback();
                return true;
            // Exception
            case token::kw_exception:
                if (!parse_exception())
                {
//                     PARSE_ERROR("Exception parse failed.");
                    return false;
                }
                break;
            // Typealiases
            case token::kw_enum:
                if (!parse_enum_type_alias())
                {
//                     PARSE_ERROR("Enum type parse failed.");
                    return false;
                }
                break;
            case token::kw_array:
                if (!parse_array_type_alias())
                {
//                     PARSE_ERROR("Array type parse failed.");
                    return false;
                }
                break;
            case token::kw_range:
                if (!parse_range_type_alias())
                {
//                     PARSE_ERROR("Range type parse failed.");
                    return false;
                }
                break;
            case token::kw_sequence:
                if (!parse_sequence_type_alias())
                {
//                     PARSE_ERROR("Sequence type parse failed.");
                    return false;
                }
                break;
            case token::kw_set:
                if (!parse_set_type_alias())
                {
//                     PARSE_ERROR("Set type parse failed.");
                    return false;
                }
                break;
            case token::kw_record:
                if (!parse_record_type_alias())
                {
//                     PARSE_ERROR("Record type parse failed.");
                    return false;
                }
                break;
            case token::kw_choice:
                if (!parse_choice_type_alias())
                {
//                     PARSE_ERROR("Choice type parse failed.");
                    return false;
                }
                break;
            case token::kw_type:
                if (!parse_type_alias())
                {
//                     PARSE_ERROR("Type alias parse failed.");
                    return false;
                }
                break;
            // Method declaration
            case token::kw_idempotent:
            case token::identifier:
                if (!parse_method())
                {
//                     PARSE_ERROR("Method parse failed.");
                    return false;
                }
                break;
            case token::autodoc:
                append_autodoc(lex.current_token());
                break;
            default:
                PARSE_ERROR("Invalid token encountered.");
                return false;
        }
    }

    return false;
}

//! exception ::= 'exception' id '{' exception_body '}'
//! exception_body ::= field_list
bool parser_t::parse_exception()
{
    D();
    if (!lex.match(token::kw_exception))
        return false;

    if (!lex.expect(token::identifier))
    {
        PARSE_ERROR("exception ID expected");
        return false;
    }

    AST::exception_t* node = new AST::exception_t(parse_tree, lex.current_token());

    assign_autodoc(node);

    local_scope_t new_scope(symbols, lex.current_token());
    if (!lex.expect(token::lbrace))
    {
        PARSE_ERROR("{ expected");
        delete node;
        return false;
    }

    parse_field_list(node);

    if (!lex.expect(token::rbrace))
    {
        PARSE_ERROR("} expected");
        delete node;
        return false;
    }

    parse_tree->add_exception(node);
    if (!symbols.insert_checked(node->name(), token::_exception_type))
    {
        PARSE_ERROR("duplicate symbol");
        return false;
    }

    return true;
}

bool parser_t::parse_method()
{
    D();
    if (lex.match(token::kw_idempotent))
    {
        is_idempotent = true;
        lex.lex();
        return parse_method(); // expect method name
    }
    if (lex.match(token::identifier))
    {
        std::string name = lex.current_token();

        if (!lex.expect(token::lparen))
        {
            PARSE_ERROR("( expected");
            return false;
        }

        AST::method_t* m = new AST::method_t(parse_tree, name, is_idempotent);

        assign_autodoc(m);

        is_idempotent = false;

        std::vector<AST::parameter_t*> params;
        if (!parse_argument_list(m, params, AST::parameter_t::in))
        {
            delete m;
            return false;
        }

        m->params = params;

        if (!lex.expect(token::rparen))
        {
            PARSE_ERROR(") expected");
            delete m;
            return false;
        }

        if (lex.maybe(token::kw_returns) || lex.maybe(token::kw_never))
        {
            if (!parse_method_returns(m))
            {
                delete m;
                return false;
            }
        }

        if (lex.maybe(token::kw_raises))
        {
            if (!parse_method_raises(m))
            {
                delete m;
                return false;
            }
        }

        if (lex.expect(token::semicolon))
        {
            parse_tree->add_method(m);
            return true;
        }

        delete m;
    }
    return false;
}

// TODO: return arglist here so we can stash it into right place
bool parser_t::parse_argument_list(AST::node_t* parent, std::vector<AST::parameter_t*>& args, AST::parameter_t::direction_e default_dir)
{
    D();
    while (lex.lex() != token::rparen)
    {
        lex.lexback();
        if (!parse_argument(parent, args, default_dir))
            return false;
    }
    lex.lexback();
    return true;
}

//! field_list ::= (var_decl ';')*
bool parser_t::parse_field_list(AST::node_t* parent)
{
    D();
    while (lex.lex() != token::rbrace)
    {
        lex.lexback();
        if (!parse_field(parent))
            return false;
    }
    lex.lexback();
    return true;
}

bool parser_t::parse_id_list(std::vector<std::string>& ids, token::kind delim)
{
    D();
    while (lex.lex() != token::rparen)
    {
        lex.lexback();
        if (!lex.expect(token::identifier))
        {
            // if (!lex.match(token::identifier))
            // {
                PARSE_ERROR("type ID expected");
                return false;
            // }
        }
        ids.push_back(lex.current_token());
        if (!lex.expect(token::comma))
        {
            if (lex.match(delim))
            {
                lex.lexback();
                return true;
            }
            PARSE_ERROR(", or delimiter expected");
            return false;
        }
    }
    lex.lexback();//??
    return true;
}

void parser_t::configure_type(AST::alias_t& to_get)
{
    D();
    // interface types:
    // - if not a builtin type
    // - if not a fully qualified type
    // - and not locally defined type
    // - then must be an interface type
    if (symbols.is_builtin_type(symbols.lookup(to_get.type())))
    {
        L(std::cout << "Builtin type " << to_get.type() << " found." << std::endl);
        to_get.set_builtin(true);
    }
    else
    {
        auto* parent = static_cast<AST::interface_t*>(parse_tree);
        if (symbols.is_qualified_type_name(to_get.type()))
        {
            // fully qualified type goes into the imported_types list
            L(std::cout << "Fully qualified import type " << to_get.type() << " found." << std::endl);
            if (!parent->imported_types_lookup(to_get))
                parent->add_imported_type(to_get);
        }
        else
        {
            // If type resolves - it's an interface-local type (which HAS to be defined before use), otherwise assume an interface reference.
            if (!parent->types_lookup(to_get))
            {
                L(std::cout << "Interface reference " << to_get.type() << " found." << std::endl);
                to_get.set_interface_reference(true); // must be ext interface ref
                if (!parent->imported_types_lookup(to_get))
                    parent->add_imported_type(to_get);
            }
            else
            {
                L(std::cout << "Interface local type " << to_get.type() << " found." << std::endl);
                to_get.set_local(true);
            }
        }
    }
}

bool parser_t::parse_type_decl(AST::alias_t& to_get)
{
    D();
    if (!lex.expect(token::type))
    {
        if (!lex.match(token::identifier))
        {
            PARSE_ERROR("field type ID expected");
            return false;
        }
    }
    to_get.set_type(lex.current_token());
    if (lex.maybe(token::reference))
        to_get.set_reference(true);
    // Scanned, now parse:
    configure_type(to_get);
    return true;
}

//! var_decl ::= typeid [reference] id
bool parser_t::parse_var_decl(AST::alias_t& to_get)
{
    D();
    // Scan all components:
    string autodoc;
    if (lex.maybe(token::autodoc))
        autodoc = lex.current_token();
    if (!parse_type_decl(to_get))
        return false;
    if (!lex.expect(token::identifier))
    {
        PARSE_ERROR("field name expected");
        return false;
    }
    to_get.set_name(lex.current_token());
    to_get.set_autodoc(autodoc);
    return true;
}

bool parser_t::parse_field(AST::node_t* parent)
{
    D();
    auto* field = new AST::alias_t(parent);
    if (!parse_var_decl(*field))
    {
        delete field;
        return false;
    }
    if (!lex.expect(token::semicolon))
    {
        PARSE_ERROR("; expected");
        delete field;
        return false;
    }

    parent->add_field(field);
    return true;
}

bool parser_t::parse_argument(AST::node_t* parent, std::vector<AST::parameter_t*>& args, AST::parameter_t::direction_e default_dir)
{
    D();
    AST::parameter_t p(parent);
    p.direction = default_dir;
    if (lex.maybe(token::kw_in))
    {
        p.direction = p.in;
    }
    else if (lex.maybe(token::kw_out))
    {
        p.direction = p.out;
    }
    else if(lex.maybe(token::kw_inout))
    {
        p.direction = p.inout;
    }
    if (!parse_var_decl(p))
    {
        return false;
    }
    args.push_back(new AST::parameter_t(p));
    if (!lex.expect(token::comma))
    {
        if (lex.match(token::rparen))
        {
            lex.lexback();
            return true;
        }
        PARSE_ERROR(", or ) expected");
        return false;
    }
    return true;
}

//! typedef ::= 'type' id id ';'
bool parser_t::parse_type_alias()
{
    D();
    if (!lex.match(token::kw_type))
        return false;
    AST::type_alias_t t(parse_tree);
    if (!parse_type_decl(t))
    {
        PARSE_ERROR("type ID expected");
        return false;
    }
    if (!lex.expect(token::identifier))
    {
        PARSE_ERROR("type name expected");
        return false;
    }
    t.set_name(lex.current_token());

    if (!lex.expect(token::semicolon))
    {
        PARSE_ERROR("; expected");
        return false;
    }

    parse_tree->add_type(new AST::type_alias_t(t));
    if (!symbols.insert_checked(t.name(), token::type))
    {
        PARSE_ERROR("duplicate symbol");
        return false;
    }

    return true;
}

bool parser_t::parse_range_type_alias()
{
    D();
    if (!lex.match(token::kw_range))
        return false;

    if (!lex.expect(token::identifier))
    {
        PARSE_ERROR("range start ID expected");
        return false;
    }

    std::string range_start = lex.current_token();

    if (!lex.expect(token::dotdot))
    {
        PARSE_ERROR(".. expected");
        return false;
    }

    if (!lex.expect(token::identifier))
    {
        PARSE_ERROR("range end ID expected");
        return false;
    }

    std::string range_end = lex.current_token();

    if (!lex.expect(token::identifier))
    {
        PARSE_ERROR("range type ID expected");
        return false;
    }

    std::string range_id = lex.current_token();

    if (!lex.expect(token::semicolon))
    {
        PARSE_ERROR("; expected");
        return false;
    }

    AST::range_alias_t* node = new AST::range_alias_t(parse_tree, range_id, range_start, range_end);
    configure_type(*node);

    parse_tree->add_type(node);
    if (!symbols.insert_checked(node->name(), token::type))
    {
        PARSE_ERROR("duplicate symbol");
        return false;
    }

    return true;
}

bool parser_t::parse_sequence_type_alias()
{
    D();
    if (!lex.match(token::kw_sequence))
        return false;

    if (!lex.expect(token::less))
    {
        PARSE_ERROR("< expected");
        return false;
    }

    AST::sequence_alias_t node(parse_tree, /*type will be set by parse_type_decl*/"", "<unset>");

    if (!parse_type_decl(node))
    {
        PARSE_ERROR("invalid sequence base type ID");
        return false;
    }

    if (!lex.expect(token::greater))
    {
        PARSE_ERROR("> expected");
        return false;
    }

    if (!lex.expect(token::identifier))
    {
        PARSE_ERROR("sequence type ID expected");
        return false;
    }

    node.set_name(lex.current_token());

    if (!lex.expect(token::semicolon))
    {
        PARSE_ERROR("; expected");
        return false;
    }

    parse_tree->add_type(new AST::sequence_alias_t(node));
    if (!symbols.insert_checked(node.name(), token::type))
    {
        PARSE_ERROR("duplicate symbol");
        return false;
    }

    return true;
}

bool parser_t::parse_set_type_alias()
{
    D();
    if (!lex.match(token::kw_set))
        return false;

    if (!lex.expect(token::less))
    {
        PARSE_ERROR("< expected");
        return false;
    }

    if (!lex.expect(token::type))
    {
        PARSE_ERROR("set base type ID expected");
        return false;
    }

    std::string base_type = lex.current_token();

    if (!lex.expect(token::greater))
    {
        PARSE_ERROR("> expected");
        return false;
    }

    if (!lex.expect(token::identifier))
    {
        PARSE_ERROR("set type ID expected");
        return false;
    }

    std::string type = lex.current_token();

    if (!lex.expect(token::semicolon))
    {
        PARSE_ERROR("; expected");
        return false;
    }

    AST::set_alias_t* node = new AST::set_alias_t(parse_tree, type, base_type);
    configure_type(*node);

    parse_tree->add_type(node);
    if (!symbols.insert_checked(node->name(), token::type))
    {
        PARSE_ERROR("duplicate symbol");
        return false;
    }
    return true;
}

bool parser_t::parse_record_type_alias()
{
    D();
    if (!lex.match(token::kw_record))
        return false;

    if (!lex.expect(token::identifier))
    {
        PARSE_ERROR("record ID expected");
        return false;
    }

    AST::record_alias_t* node = new AST::record_alias_t(parse_tree, lex.current_token());
    local_scope_t new_scope(symbols, lex.current_token());

    if (!lex.expect(token::lbrace))
    {
        PARSE_ERROR("{ expected");
        return false;
    }

    parse_field_list(node);

    if (!lex.expect(token::rbrace))
    {
        PARSE_ERROR("} expected");
        return false;
    }

    parse_tree->add_type(node);
    if (!symbols.insert_checked(node->name(), token::type))
    {
        PARSE_ERROR("duplicate symbol");
        return false;
    }

    return true;
}

//! choice_decl ::= id '=>' type_decl
bool parser_t::parse_choice_decl(AST::node_t* parent)
{
    auto* alias = new AST::alias_t(parent);

    if (!lex.expect(token::identifier))
    {
        PARSE_ERROR("Choice from selector enum expected.");
        return false;
    }

    std::string name = lex.current_token();

    if (!lex.expect(token::dblarrow))
    {
        PARSE_ERROR("=> expected");
        return false;
    }

    if (!parse_type_decl(*alias))
        return false;

    alias->set_name(name);
    parent->add_field(alias);

    if (!lex.expect(token::comma))
    {
        if (lex.match(token::rbrace))
        {
            lex.lexback();
            return true;
        }
        PARSE_ERROR(", or } expected");
        return false;
    }
    return true;
}

//! choice_list ::= choice_decl (',' choice_decl)*
bool parser_t::parse_choice_list(AST::node_t* parent)
{
    D();
    while (lex.lex() != token::rbrace)
    {
        lex.lexback();
        if (!parse_choice_decl(parent))
            return false;
    }
    lex.lexback();
    return true;
}


bool parser_t::parse_choice_type_alias()
{
    D();
    if (!lex.match(token::kw_choice))
        return false;

    if (!lex.expect(token::identifier))
    {
        PARSE_ERROR("choice ID expected");
        return false;
    }

    AST::choice_alias_t* node = new AST::choice_alias_t(parse_tree, lex.current_token());
    local_scope_t new_scope(symbols, lex.current_token());

    if (!lex.expect(token::kw_on))
    {
        PARSE_ERROR("'on' keyword expected in choice declaration");
        return false;
    }

    if (!lex.expect(token::identifier))
    {
        PARSE_ERROR("choice selector ID expected");
        return false;
    }

    std::string selector = lex.current_token();
    // @todo: find selector in the list of defined enums and raise error if not found
    // or if specified selectors are not defined in the enum.
    node->set_selector(selector);

    if (!lex.expect(token::lbrace))
    {
        PARSE_ERROR("{ expected");
        return false;
    }

    parse_choice_list(node);

    if (!lex.expect(token::rbrace))
    {
        PARSE_ERROR("} expected");
        return false;
    }

    parse_tree->add_type(node);
    if (!symbols.insert_checked(node->name(), token::type))
    {
        PARSE_ERROR("duplicate symbol");
        return false;
    }

    return true;
}

// FIXME: should enum create a scope?
bool parser_t::parse_enum_type_alias()
{
    D();
    if (!lex.match(token::kw_enum))
        return false;

    if (!lex.expect(token::identifier))
    {
        PARSE_ERROR("enum ID expected");
        return false;
    }

    AST::enum_alias_t* node = new AST::enum_alias_t(parse_tree);
    node->set_name(lex.current_token());

    if (!lex.expect(token::lbrace))
    {
        PARSE_ERROR("{ expected");
        return false;
    }

    std::vector<std::string> ids;
    if (!parse_id_list(ids, token::rbrace))
    {
        // PARSE_ERROR("enum list parse failed");
        return false;
    }

    node->fields = ids;

    if (!lex.expect(token::rbrace))
    {
        PARSE_ERROR("} expected");
        return false;
    }

    parse_tree->add_type(node);
    if (!symbols.insert_checked(node->name(), token::type))
    {
        PARSE_ERROR("duplicate symbol");
        return false;
    }

    return true;
}

bool parser_t::parse_array_type_alias()
{
    D();
    if (!lex.match(token::kw_array))
        return false;

    if (!lex.expect(token::type))
    {
        PARSE_ERROR("array base type ID expected");
        return false;
    }

    std::string base_type = lex.current_token();

    if (!lex.expect(token::lsquare))
    {
        PARSE_ERROR("[ expected");
        return false;
    }

    if (!lex.expect(token::cardinal))
    {
        PARSE_ERROR("number of repetitions expected");
        return false;
    }

    int count = lex.current_value();

    if (!lex.expect(token::rsquare))
    {
        PARSE_ERROR("] expected");
        return false;
    }

    if (!lex.expect(token::identifier))
    {
        PARSE_ERROR("array ID expected");
        return false;
    }

    std::string type = lex.current_token();

    if (!lex.expect(token::semicolon))
    {
        PARSE_ERROR("; expected");
        return false;
    }

    AST::array_alias_t* node = new AST::array_alias_t(parse_tree, type, base_type, count);
    configure_type(*node);

    parse_tree->add_type(node);
    if (!symbols.insert_checked(node->name(), token::type))
    {
        PARSE_ERROR("duplicate symbol");
        return false;
    }

    return true;
}

bool parser_t::parse_method_returns(AST::method_t* m)
{
    D();

    if (lex.match(token::kw_never))
    {
        if (!lex.expect(token::kw_returns))
        {
            PARSE_ERROR("'returns' expected after 'never'");
            return false;
        }
        m->returns.clear();
        m->never_returns = true;
        return true;
    }

    if (!lex.match(token::kw_returns))
        return false;

    if (!lex.expect(token::lparen))
    {
        PARSE_ERROR("( expected");
        return false;
    }

    std::vector<AST::parameter_t*> returns;
    if (!parse_argument_list(m, returns, AST::parameter_t::out))
        return false;

    // TODO: check that all ids are types
    m->returns = returns;

    if (!lex.expect(token::rparen))
    {
        PARSE_ERROR(") expected");
        return false;
    }

    return true;
}

bool parser_t::parse_method_raises(AST::method_t* m)
{
    D();
    if (!lex.match(token::kw_raises))
        return false;

    if (!lex.expect(token::lparen))
    {
        PARSE_ERROR("( expected");
        return false;
    }

    std::vector<std::string> exc_ids;
    if (!parse_id_list(exc_ids, token::rparen))
        return false;

    // TODO: check that all ids are _exception_types
    m->raises_ids = exc_ids;

    if (!lex.expect(token::rparen))
    {
        PARSE_ERROR(") expected");
        return false;
    }

    return true;
}
