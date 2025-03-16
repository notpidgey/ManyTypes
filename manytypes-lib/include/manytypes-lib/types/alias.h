#pragma once
#include "manytypes-lib/types/models/named_sized.h"

class alias_type_t final : public named_sized_type_t
{
public:
    alias_type_t( std::string alias, const type_id type )
        : alias( std::move( alias ) ), type( type )
    {
    }

    std::string name_of( ) const override
    {
        return alias;
    }

    size_t size_of( type_size_resolver& tr ) const override
    {
        return tr( type );
    }

    std::string alias;
    type_id type;
};
