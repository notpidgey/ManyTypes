#include "manytypes-lib/types/structure.h"
#include "manytypes-lib/db/database.h"

bool structure_t::add_field( const base_field_t& field )
{
    s_fields.push_back( field );
    return true;
}