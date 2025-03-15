#pragma once
#include <algorithm>
#include <utility>
#include <vector>

#include "manytypes-lib/struct/models/field.h"
#include "manytypes-lib/struct/models/named_sized.h"

struct structure_settings
{
    std::string name;

    uint32_t align;
    uint32_t size;

    bool is_union;
};

class structure_t final : public named_sized_type_t
{
public:
    virtual ~structure_t( ) = default;

    explicit structure_t( structure_settings settings, const bool in_order_insertion )
        : settings( std::move( settings ) ), in_order_insert( in_order_insertion )
    {
    }

    bool add_field( const base_field_t& field );
    [[nodiscard]] bool is_field_valid( const base_field_t& next_field );

    [[nodiscard]] const std::vector<base_field_t>& get_fields( ) const
    {
        return s_fields;
    }

    bool is_union() const
    {
        return settings.is_union;
    }

    size_t size_of( ) override;
    std::string name_of( ) override;

protected:
    structure_settings settings;

    std::vector<base_field_t> s_fields;
    bool in_order_insert;
};
