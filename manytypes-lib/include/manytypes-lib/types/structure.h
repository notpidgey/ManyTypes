#pragma once
#include <algorithm>
#include <utility>
#include <vector>

#include "manytypes-lib/types/models/field.h"
#include "manytypes-lib/types/models/named_sized.h"

struct structure_settings
{
    std::string name;

    uint32_t align;
    uint32_t size;

    bool is_union;
};

class structure_t final : public named_sized_type_t, public dependent_t
{
public:
    ~structure_t( ) override = default;

    explicit structure_t( structure_settings settings, const bool in_order_insertion )
        : settings( std::move( settings ) ), in_order_insert( in_order_insertion )
    {
    }

    bool add_field( const base_field_t& field )
    {
        const auto overlap = std::ranges::any_of(
            s_fields,
            [&field] ( auto& s_field )
            {
                if ( s_field.bit_offset >= field.bit_offset &&
                    s_field.bit_offset + s_field.bit_size < field.bit_offset )
                    return true;

                return false;
            } );

        if ( overlap )
            return false;

        s_fields.push_back( field );
        return true;
    }

    [[nodiscard]] const std::vector<base_field_t>& get_fields( ) const
    {
        return s_fields;
    }

    bool is_union( ) const
    {
        return settings.is_union;
    }

    size_t size_of( type_size_resolver& tr ) override
    {
        return settings.size;
    }

    std::string name_of( ) override
    {
        return settings.name;
    }

    std::vector<type_id> get_dependencies( ) override
    {
        std::vector<type_id> dependencies;
        dependencies.reserve( s_fields.size( ) );

        for ( auto& member : s_fields )
            dependencies.push_back( member.type_id );

        return dependencies;
    }

protected:
    structure_settings settings;

    std::vector<base_field_t> s_fields;
    bool in_order_insert;
};
