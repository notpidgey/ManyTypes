#include "manytypes-lib/manytypes.h"

#include <functional>
#include <ranges>

#define ALIGN_UP(value, alignment) (((value) + (alignment) - 1) & ~((alignment) - 1))

template < class... Ts >
struct overloads : Ts...
{
    using Ts::operator()...;
};

type_id unwind_complex_type( clang_context_t* client_data, const CXType& type )
{
    auto lower_type = type;
    while ( true )
    {
        // if we already have seen this type that means all the base types have been created
        if ( client_data->clang_db.is_type_defined( lower_type ) )
            break;

        if ( lower_type.kind == CXType_ConstantArray )
        {
            array_t arr( true, clang_getArraySize( lower_type ) );
            client_data->clang_db.save_type_id( lower_type, client_data->type_db.insert_type( arr ) );

            lower_type = clang_getArrayElementType( lower_type );
        }
        else if ( lower_type.kind == CXType_Pointer )
        {
            pointer_t ptr( clang_getArraySize( lower_type ) );
            client_data->clang_db.save_type_id( lower_type, client_data->type_db.insert_type( ptr ) );

            lower_type = clang_getPointeeType( lower_type );
        }
        else if ( lower_type.kind == CXType_Elaborated)
        {
            lower_type = clang_Type_getNamedType(type);
        }
        else
        {
            // we reached the base type, verify that it exists
            assert( client_data->clang_db.is_type_defined( lower_type ), "type id must exist" );
            break;
        };
    }

    return client_data->clang_db.get_type_id( type );
}

CXChildVisitResult visit_cursor( CXCursor cursor, CXCursor parent, CXClientData data )
{
    clang_context_t* client_data = static_cast<clang_context_t*>(data);

    const auto cursor_kind = clang_getCursorKind( cursor );
    switch ( cursor_kind )
    {
    case CXCursor_ClassDecl:
    case CXCursor_StructDecl:
    case CXCursor_UnionDecl:
        {
            // todo: in ctor check that none of the arguments are negative other than size = -1
            const auto cursor_type = clang_getCursorType( cursor );
            const auto type_name = clang_spelling_str( cursor );

            auto decl_type_byte_size = clang_Type_getSizeOf( cursor_type );
            assert(
                decl_type_byte_size != CXTypeLayoutError_Invalid &&
                decl_type_byte_size != CXTypeLayoutError_Incomplete &&
                decl_type_byte_size != CXTypeLayoutError_Dependent,
                "structure is not properly sized"
            );

            auto decl_type_byte_align = clang_Type_getAlignOf( cursor_type );
            assert( decl_type_byte_align > 0, "structure is not properly aligned" );

            uint32_t decl_type_size = decl_type_byte_size * 8;
            uint32_t decl_type_align = decl_type_byte_align * 8;

            structure_t sm{
                structure_settings{
                    .name = type_name,
                    .align = decl_type_align,
                    .size = decl_type_size,
                    .is_union = cursor_kind == CXCursor_UnionDecl,
                },
                true
            };

            type_id decl_type_id;
            if ( client_data->clang_db.is_type_defined( cursor_type ) )
            {
                // type already has been declared,
                decl_type_id = client_data->clang_db.get_type_id( cursor_type );
                auto prev_decl = std::get<structure_t>( client_data->type_db.lookup_type( decl_type_id ) );

                // todo this is terrible design. this should be fixed
                type_size_resolver resolver = [] ( type_id id ) { return static_cast<size_t>(0); };
                auto prev_size = prev_decl.size_of( resolver );
                auto prev_fields_count = prev_decl.get_fields( ).size( );

                // todo if current size warning for redefinition
                if ( prev_size != 0 && !prev_fields_count )
                    return CXChildVisit_Continue; // skip

                client_data->type_db.update_type( decl_type_id, sm );
            }
            else
            {
                decl_type_id = client_data->type_db.insert_type( sm );
                client_data->clang_db.save_type_id( cursor_type, decl_type_id );
            }

            const auto parent_cursor = clang_getCursorSemanticParent( cursor );
            if ( cursor_kind == CXCursor_UnionDecl && !clang_Cursor_isNull( parent_cursor ) &&
                clang_Cursor_isAnonymous( cursor ) )
            {
                // check if anonymous union because it must mean
                auto parent_type = clang_getCursorType( parent_cursor );
                std::visit(
                    overloads
                    {
                        [&] ( structure_t& s )
                        {
                            const auto fields = s.get_fields( );

                            uint32_t target_bit_offset = 0;
                            if ( !s.is_union( ) && !fields.empty( ) )
                            {
                                auto& back_field = fields.back( );
                                const auto prev_end_offset = back_field.bit_offset + ( back_field.bit_size + 7 ) / 8;
                                const auto union_align = clang_Type_getAlignOf( parent_type );

                                target_bit_offset = ALIGN_UP( prev_end_offset, union_align );
                            }

                            s.add_field(
                                base_field_t{
                                    .bit_offset = target_bit_offset,
                                    .bit_size = decl_type_size,
                                    .is_bit_field = false,
                                    .name = "",
                                    .type_id = decl_type_id,
                                } );
                        },
                        [] ( auto&& )
                        {
                            assert( true, "unexpected exception occurred" );
                        }
                    }, client_data->type_db.lookup_type( client_data->clang_db.get_type_id( parent_type ) ) );
            }

            return cursor_kind == CXCursor_ClassDecl ? CXChildVisit_Continue : CXChildVisit_Recurse;
        }
    case CXCursor_EnumDecl:
        {
            const auto type_name = clang_spelling_str( cursor );

            // todo add check that underlying type exists
            enum_t em{ type_name, client_data->clang_db.get_type_id( clang_getEnumDeclIntegerType( cursor ) ) };
            client_data->clang_db.save_type_id( clang_getCursorType( cursor ), client_data->type_db.insert_type( em ) );

            break;
        }
    }

    switch ( cursor_kind )
    {
    case CXCursor_EnumConstantDecl:
        {
            const CXCursorKind parent_kind = clang_getCursorKind( parent );
            assert( parent_kind == CXCursor_EnumDecl, "parent is not enum declaration" );

            const CXType parent_type = clang_getCursorType( parent );
            auto em = std::get<enum_t>(
                client_data->type_db.lookup_type(
                    client_data->clang_db.get_type_id( parent_type ) ) );

            const auto type_name = clang_spelling_str( cursor );
            em.insert_member( clang_getEnumConstantDeclValue( cursor ), type_name );
            break;
        }
    case CXCursor_FieldDecl:
        {
            const CXCursorKind parent_kind = clang_getCursorKind( parent );
            assert(
                parent_kind == CXCursor_ClassDecl || parent_kind == CXCursor_StructDecl,
                "parent is not valid structure declaration" );

            auto parent_type = clang_getCursorType( parent );
            auto underlying_type = clang_getCursorType( cursor );

            std::visit(
                overloads
                {
                    [&] ( structure_t& s )
                    {
                        const auto bit_width = clang_getFieldDeclBitWidth( cursor );
                        assert( bit_width != -1, "bit width must not be value dependent" );

                        const auto bit_offset = clang_Cursor_getOffsetOfField( cursor );
                        assert(
                            bit_offset != CXTypeLayoutError_Invalid &&
                            bit_offset != CXTypeLayoutError_Incomplete &&
                            bit_offset != CXTypeLayoutError_Dependent &&
                            bit_offset != CXTypeLayoutError_InvalidFieldName,
                            "field offset is invalid" );

                        const type_id field_type_id = unwind_complex_type( client_data, underlying_type );

                        bool revised_field = false;

                        auto fields = s.get_fields( );
                        if ( !fields.empty( ) )
                        {
                            auto& back = fields.back( );
                            if ( back.bit_offset == bit_offset && back.type_id == field_type_id )
                            {
                                back.name = clang_spelling_str( cursor );
                                revised_field = true;
                            }
                        }

                        if ( !revised_field )
                        {
                            s.add_field(
                                base_field_t{
                                    .bit_offset = static_cast<uint32_t>(bit_offset),
                                    .bit_size = static_cast<uint32_t>(bit_width),
                                    .is_bit_field = clang_Cursor_isBitField( cursor ) != 0,
                                    .name = "",
                                    .type_id = field_type_id,
                                } );
                        }
                    },
                    [&] ( auto&& )
                    {
                    },
                }, client_data->type_db.lookup_type( client_data->clang_db.get_type_id( parent_type ) ) );
            break;
        }
    case CXCursor_TypedefDecl:
        {
            if ( client_data->clang_db.is_type_defined( clang_getCursorType( cursor ) ) ) // skip repeating typedefs
                return CXChildVisit_Recurse;

            CXType underlying_type = clang_getTypedefDeclUnderlyingType( cursor );
            switch ( underlying_type.kind )
            {
            case CXType_FunctionProto:
                {
                    call_conv conv = call_conv::unk;
                    switch ( clang_getFunctionTypeCallingConv( underlying_type ) )
                    {
                    case CXCallingConv_C: conv = call_conv::cc_cdecl;
                        break;
                    case CXCallingConv_X86StdCall: conv = call_conv::cc_stdcall;
                        break;
                    case CXCallingConv_X86FastCall: conv = call_conv::cc_fastcall;
                        break;
                    case CXCallingConv_X86ThisCall: conv = call_conv::cc_thiscall;
                        break;
                    }

                    function_t fun_proto{
                        clang_spelling_str( cursor ),
                        conv,
                        client_data->clang_db.get_type_id( clang_getResultType( underlying_type ) ),
                        [&]( )-> std::vector<type_id>
                        {
                            std::vector<type_id> types;
                            for ( auto i = 0; i < clang_getNumArgTypes( underlying_type ); i++ )
                                types.push_back(
                                    client_data->clang_db.get_type_id( clang_getArgType( underlying_type, i ) ) );

                            return types;
                        }( )
                    };

                    client_data->clang_db.save_type_id(
                        clang_getCursorType( cursor ), client_data->type_db.insert_type( fun_proto ) );
                    break;
                }
            default:
                {
                    const type_id id = unwind_complex_type( client_data, underlying_type );
                    if ( !client_data->clang_db.is_type_defined( underlying_type ) )
                    {
                        client_data->failed = true;
                        return CXChildVisit_Break;
                    }

                    const auto type_name = clang_spelling_str( cursor );
                    type_id typedef_id = client_data->type_db.insert_type(
                        alias_type_t(
                            type_name, id
                        ) );

                    client_data->clang_db.save_type_id( clang_getCursorType( cursor ), typedef_id );
                    break;
                }
            }
        }
    }

    return CXChildVisit_Recurse;
}

std::optional<type_database_t> parse_root_source( const std::filesystem::path& src_path )
{
    const std::vector<std::string> clang_args =
    {
        "-x",
        "c++",
        "-fms-extensions",
        "-Xclang",
        "-ast-dump",
        "-fsyntax-only",
        "-fms-extensions",
        "-target x86_64-windows-msvc"
    };

    std::vector<const char*> c_args;
    for ( const auto& arg : clang_args )
        c_args.push_back( arg.c_str( ) );

    if ( const auto index = clang_createIndex( 0, 1 ) )
    {
        CXTranslationUnit tu = nullptr;
        const auto error = clang_parseTranslationUnit2(
            index,
            src_path.string( ).c_str( ),
            c_args.data( ),
            static_cast<int>(c_args.size( )),
            nullptr,
            0,
            CXTranslationUnit_DetailedPreprocessingRecord |
            CXTranslationUnit_PrecompiledPreamble |
            CXTranslationUnit_SkipFunctionBodies |
            CXTranslationUnit_ForSerialization,
            &tu );

        if ( error == CXError_Success )
        {
            clang_context_t ctx = { };

            const CXCursor cursor = clang_getTranslationUnitCursor( tu );
            clang_visitChildren( cursor, visit_cursor, &ctx );

            if ( !ctx.failed ) return ctx.type_db;
        }
        else printf( "CXError: %d\n", error );

        clang_disposeTranslationUnit( tu );
    }

    return std::nullopt;
}

std::vector<type_id> order_database_nodes( const type_database_t& db )
{
    auto& types = db.get_types( );

    std::unordered_set<type_id> visited;
    std::unordered_set<type_id> rec_stack;
    std::vector<type_id> sorted;

    std::function<void( type_id )> dfs_type;
    dfs_type = [&] ( const type_id id )
    {
        if ( rec_stack.contains( id ) )
            throw std::runtime_error( "Cycle detected in type dependencies" );

        if ( visited.contains( id ) )
            return;

        visited.insert( id );
        rec_stack.insert( id );

        const auto it = types.find( id );
        if ( it != types.end( ) )
        {
            //for ( auto dep : getDependencies( it->second ) )
            //{
            //    dfs_type( dep );
            //}
        }

        rec_stack.erase( id );
        sorted.push_back( id );
    };

    for ( const auto& id : types | std::views::keys )
    {
        if ( !visited.contains( id ) )
            dfs_type( id );
    }

    return sorted;
}
