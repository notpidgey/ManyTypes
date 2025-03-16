#include "plugin.h"

#include <filesystem>
#include <string>

#include "manytypes-lib/manytypes.h"

std::atomic<const char*> curr_image_name;
std::unordered_map<std::string, std::filesystem::file_time_type> last_save_time;

void plugin_run_loop( )
{
    const char* curr_image = curr_image_name;
    if ( curr_image == nullptr )
        return;

    std::string norm_image_name = curr_image;
    std::ranges::replace( norm_image_name, ' ', '-' );

    const auto run_root = std::filesystem::current_path( );

    const auto manytypes_root = run_root / "ManyTypes";
    const auto dbg_workspace = manytypes_root / norm_image_name;

    const auto dbg_workspace_root = dbg_workspace / "project.h";
    create_directories( dbg_workspace_root );

    bool must_refresh = false;
    for ( auto& dir_item : std::filesystem::directory_iterator( dbg_workspace ) )
    {
        if ( dir_item.is_regular_file( ) )
        {
            const auto& file = dir_item.path( );
            if ( file.extension( ) != ".h" && file.extension( ) != ".hpp" )
                continue;

            auto last_write = last_write_time( file );
            if ( last_save_time[ file.filename( ).string( ) ] != last_write )
                must_refresh = true;

            last_save_time[ file.filename( ).string( ) ] = last_write;
        }
    }

    if ( must_refresh )
    {
        // we must run clang parser and update x64dbg types
        const std::filesystem::path global = manytypes_root / "global.h";
        if ( !exists( global ) ) // global header must exist
            create_directory( global );

        // hidden source file
        const std::filesystem::path src_root = manytypes_root / "source.cpp";
        if ( !exists( src_root ) )
        {
            create_directory( src_root );
            SetFileAttributesA( src_root.string( ).c_str( ), FILE_ATTRIBUTE_HIDDEN );
        }

        // write dummy include
        bool abort_parse = false;
        {
            std::fstream src_file( src_root, std::ios::trunc );
            if ( src_file )
            {
                src_file << "#include \"global.h\"\n";
                src_file << "#include \"" << norm_image_name << "/project.h\"";
            }
            else abort_parse = true;
        }

        if ( !abort_parse )
        {
            auto opt_db = parse_root_source( src_root );
            if ( opt_db )
            {
                auto& db = *opt_db;
                db.
            }
            else
            {
                dprintf( "unable to parse source tree" );
            }
        }
        else
            dprintf( "aborting header parse, unable to write to source" );
    }
}

void set_workspace_target( const char* image_name )
{
    curr_image_name = image_name;
}

// Initialize your plugin data here.
bool plugin_init( PLUG_INITSTRUCT* initStruct )
{
    dprintf( "plugin_init(pluginHandle: %d)\n", pluginHandle );

    // Register the example command
    _plugin_registercommand( pluginHandle, PLUGIN_NAME, cbExampleCommand, true );

    return true;
}

// Deinitialize your plugin data here.
void plugin_stop( )
{
    dprintf( "plugin_stop(pluginHandle: %d)\n", pluginHandle );
}

// Do GUI/Menu related things here.
void plugin_setup( )
{
    dprintf( "plugin_setup(pluginHandle: %d)\n", pluginHandle );
}
