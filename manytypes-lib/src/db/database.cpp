#include "manytypes-lib/db/database.h"
#include "manytypes-lib/util/util.h"
#include "manytypes-lib/exceptions.h"

namespace mt
{

std::array<basic_type_t, 17> type_database_t::types = {
    basic_type_t{ "bool", sizeof( bool ) * 8 },
    basic_type_t{ "char", sizeof( char ) * 8 },
    basic_type_t{ "unsigned char", sizeof( unsigned char ) * 8 },
    basic_type_t{ "signed char", sizeof( signed char ) * 8 },
    basic_type_t{ "wchar_t", sizeof( wchar_t ) * 8 },
    basic_type_t{ "short", sizeof( short ) * 8 },
    basic_type_t{ "unsigned short", sizeof( unsigned short ) * 8 },
    basic_type_t{ "int", sizeof( int ) * 8 },
    basic_type_t{ "unsigned int", sizeof( unsigned int ) * 8 },
    basic_type_t{ "long", sizeof( long ) * 8 },
    basic_type_t{ "unsigned long", sizeof( unsigned long ) * 8 },
    basic_type_t{ "long long", sizeof( long long ) * 8 },
    basic_type_t{ "unsigned long long", sizeof( unsigned long long ) * 8 },
    basic_type_t{ "float", sizeof( float ) * 8 },
    basic_type_t{ "double", sizeof( double ) * 8 },
    basic_type_t{ "long double", sizeof( long double ) * 8 },
    basic_type_t{ "void", 0 }
};

type_database_t::type_database_t( const uint8_t byte_pointer_size )
    : bit_pointer_size( byte_pointer_size * 8 )
{
    curr_type_id = 1;

    for ( auto& t : types )
        insert_type( t );
}

type_id type_database_t::insert_type( const type_id_data& data, type_id semantic_parent )
{
    if ( !( semantic_parent == 0 || type_scopes.contains( semantic_parent ) ) )
    {
        throw InvalidSemanticParentException( "semantic parent must be valid type" );
    }

    type_info.insert( { curr_type_id, data } );
    if ( semantic_parent )
        type_scopes[curr_type_id] = semantic_parent;

    return curr_type_id++;
}

type_id type_database_t::insert_placeholder_type( const null_type_t& data, type_id semantic_parent )
{
    // todo this is a place holder for the insert place holder
    return insert_type( data, semantic_parent );
}

void type_database_t::insert_semantic_parent( type_id id, type_id parent )
{
    if ( !type_info.contains( id ) )
        throw TypeNotFoundException( "type must exist" );
    if ( !type_info.contains( parent ) )
        throw TypeNotFoundException( "parent type must exist" );
    if ( type_scopes.contains( id ) )
        throw TypeAlreadyExistsException( "type must not exist in scope" );

    type_scopes.insert( { id, parent } );
}

void type_database_t::update_type( const type_id id, const type_id_data& data )
{
    if ( !type_info.contains( id ) )
        throw TypeNotFoundException( "type with current id must exist" );

    type_info.at( id ) = data;
}

type_id_data& type_database_t::lookup_type( const type_id id )
{
    if ( !type_info.contains( id ) )
        throw TypeNotFoundException( "type info must contain id" );

    return type_info.at( id );
}

bool type_database_t::contains_type( const type_id id ) const
{
    return type_info.contains( id );
}

const std::unordered_map<type_id, type_id_data>& type_database_t::get_types() const
{
    return type_info;
}

std::string type_database_t::get_type_print(const type_id id)
{
    if (!type_info.contains(id))
        throw TypeNotFoundException("type info must contain id");

    auto& type = type_info.at(id);
    return std::visit(
        overloads{
            [&](const pointer_t& p)
            {
                return get_type_print(p.get_elem_type()) + "*";
            },
            [&](const typedef_type_t& s)
            {
                return s.alias;
            },
            [&](const structure_t& s)
            {
                return s.get_name();
            },
            [&](const enum_t& e)
            {
                return e.get_name();
            },
            [&](const auto&)
            {
                throw TypeNotPrintable("unknown type name printed " + std::to_string(id));
                return std::string{};
            }
        }, type);
}
} // namespace mt
