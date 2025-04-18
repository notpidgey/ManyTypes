#pragma once
#include "manytypes-lib/db/database.h"
#include "manytypes-lib/util/clang-utils.h"

#include <manytypes-lib/exceptions.h>

namespace mt
{
class clang_database_t
{
public:
    explicit clang_database_t( type_database_t& type_db )
        : type_db( &type_db )
    {
        const std::pair<CXTypeKind, const char*> type_map[] = {
            { CXType_Void, "void" },
            { CXType_Bool, "bool" },
            { CXType_Char_U, "unsigned char" },
            { CXType_Char_S, "char" },
            { CXType_UChar, "unsigned char" },
            { CXType_SChar, "signed char" },
            { CXType_WChar, "wchar_t" },
            { CXType_Short, "short" },
            { CXType_UShort, "unsigned short" },
            { CXType_Int, "int" },
            { CXType_UInt, "unsigned int" },
            { CXType_Long, "long" },
            { CXType_ULong, "unsigned long" },
            { CXType_LongLong, "long long" },
            { CXType_ULongLong, "unsigned long long" },
            { CXType_Float, "float" },
            { CXType_Double, "double" },
            { CXType_LongDouble, "long double" }
        };

        for ( auto& [cx_type, str] : type_map )
        {
            bool found = false;
            for ( int i = 0; i < type_database_t::types.size() && !found; i++ )
            {
                if ( type_database_t::types[i].name == str )
                {
                    base_kinds[cx_type] = i + 1;
                    found = true;
                }
            }

            // some error occ
            if ( !found )
                __debugbreak();
        }
    }

    void save_type_id( const CXType& type, const type_id id )
    {
        type_map.insert( { type, id } );
    }

    type_id get_type_id( const CXType& type ) const
    {
        if ( !type_map.contains( type ) && !base_kinds.contains( type.kind ) )
            throw TypeNotFoundException( "Type not found in the database" );

        if ( base_kinds.contains( type.kind ) )
            return base_kinds.at( type.kind );

        return type_map.at( type );
    }

    bool is_type_defined( const CXType& type ) const
    {
        if ( base_kinds.contains( type.kind ) )
            return true;

        return type_map.contains( type );
    }

private:
    std::unordered_map<CXTypeKind, type_id> base_kinds;

    std::unordered_map<CXType, type_id, cx_type_hash, cx_type_equal> type_map;
    type_database_t* type_db;
};
} // namespace mt
