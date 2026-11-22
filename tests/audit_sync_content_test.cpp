#include <algorithm>
#include <cstdlib>
#include <functional>
#include <initializer_list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "cata_path.h"
#include "construction.h"
#include "coordinates.h"
#include "creature.h"
#include "creature_tracker.h"
#include "dialogue.h"
#include "effect_on_condition.h"
#include "flexbuffer_json.h"
#include "game.h"
#include "item.h"
#include "item_group.h"
#include "json_loader.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "math_parser_diag_value.h"
#include "monattack.h"
#include "monster.h"
#include "mtype.h"
#include "npctalk.h"
#include "overmap.h"
#include "path_info.h"
#include "perf.h"
#include "player_helpers.h"
#include "point.h"
#include "recipe.h"
#include "requirements.h"
#include "ret_val.h"
#include "talker.h"
#include "type_id.h"
#include "units.h"

static const construction_str_id construction_constr_hesco_mil3( "constr_hesco_mil3" );
static const construction_str_id construction_constr_hesco_mil3_wall( "constr_hesco_mil3_wall" );

static const effect_on_condition_id
effect_on_condition_EOC_LS_Obelisk_fixbot_grant( "EOC_LS_Obelisk_fixbot_grant" );
static const effect_on_condition_id
effect_on_condition_eoc_wells_toxic_grenade( "eoc_wells_toxic_grenade" );
static const effect_on_condition_id effect_on_condition_warlord_setup( "warlord_setup" );

static const furn_str_id furn_f_hesco_barrier( "f_hesco_barrier" );
static const furn_str_id furn_f_hesco_wall( "f_hesco_wall" );

static const itype_id itype_air_pistol_beretta_9mm( "air_pistol_beretta_9mm" );
static const itype_id itype_air_pistol_beretta_9mm_mag( "air_pistol_beretta_9mm_mag" );
static const itype_id itype_budget_steel_lump( "budget_steel_lump" );
static const itype_id itype_ch_steel_lump( "ch_steel_lump" );
static const itype_id itype_coal_lump( "coal_lump" );
static const itype_id itype_lc_steel_lump( "lc_steel_lump" );
static const itype_id itype_material_soil( "material_soil" );
static const itype_id itype_mc_steel_lump( "mc_steel_lump" );
static const itype_id itype_qt_steel_lump( "qt_steel_lump" );
static const itype_id itype_warlord_gasbomb_act( "warlord_gasbomb_act" );

static const mtype_id mon_LS_fixbot_haywire( "mon_LS_fixbot_haywire" );
static const mtype_id mon_LS_sparkgenerator_haywire( "mon_LS_sparkgenerator_haywire" );
static const mtype_id mon_wells_machine( "mon_wells_machine" );
static const mtype_id mon_wells_machine_leg( "mon_wells_machine_leg" );

static const recipe_id recipe_lc_steel_lump_from_ch_steel( "lc_steel_lump_from_ch_steel" );
static const recipe_id recipe_mc_steel_lump( "mc_steel_lump" );
static const recipe_id recipe_mc_steel_lump_from_qt_steel( "mc_steel_lump_from_qt_steel" );
static const recipe_id recipe_nested_steel_lump( "nested_steel_lump" );

static const trait_id trait_LS_Passcode_1( "LS_Passcode_1" );
static const trait_id trait_LS_Passcode_fixbot( "LS_Passcode_fixbot" );

namespace
{

int count_monsters( const mtype_id &type )
{
    int count = 0;
    for( const monster &critter : g->all_monsters() ) {
        if( critter.type->id == type ) {
            count++;
        }
    }
    return count;
}

bool takes_component( const recipe &rec, const itype_id &component )
{
    for( const std::vector<item_comp> &alternatives :
         rec.simple_requirements().get_components() ) {
        for( const item_comp &comp : alternatives ) {
            if( comp.type == component ) {
                return true;
            }
        }
    }
    return false;
}

int soil_put_in( const construction_str_id &con )
{
    for( const std::vector<item_comp> &alternatives : con->requirements->get_components() ) {
        for( const item_comp &comp : alternatives ) {
            if( comp.type == itype_material_soil ) {
                return comp.count;
            }
        }
    }
    return 0;
}

// the most soil an item group gave in 50 tries
int most_soil_from( const item_group_id &group )
{
    int most = 0;
    for( int i = 0; i < 50; ++i ) {
        int soil = 0;
        for( const item &it : item_group::items_from( group, calendar::turn ) ) {
            if( it.typeId() == itype_material_soil ) {
                soil += it.count();
            }
        }
        most = std::max( most, soil );
    }
    return most;
}

JsonArray json_file( const std::string &relative_path )
{
    return json_loader::from_path( PATH_INFO::datadir_path() / relative_path ).get_array();
}

// every talk topic named in a JSON value: response topics (also a trial's success and
// failure), npc_first_topic effects and an NPC's chat
void collect_topic_references( const JsonValue &jv, std::set<std::string> &refs )
{
    if( jv.test_object() ) {
        JsonObject jo = jv.get_object();
        jo.allow_omitted_members();
        for( const JsonMember member : jo ) {
            const std::string name = member.name();
            if( ( name == "topic" || name == "npc_first_topic" || name == "chat" ) &&
                member.test_string() ) {
                refs.insert( member.get_string() );
            } else {
                collect_topic_references( member, refs );
            }
        }
    } else if( jv.test_array() ) {
        for( const JsonValue entry : jv.get_array() ) {
            collect_topic_references( entry, refs );
        }
    }
}

} // namespace

// The haywire Bot Printer counted the normal bots, but every bot turns haywire when the
// printer dies, so its cap of 50 never applied.
TEST_CASE( "sync-89409_haywire_printer_stops_at_50_haywire_bots", "[monster][sync]" )
{
    clear_map();
    clear_avatar();
    const tripoint_bub_ms printer_pos( 30, 30, 0 );
    monster &printer = spawn_test_monster( mon_LS_sparkgenerator_haywire.str(), printer_pos );
    for( int i = 0; i < 49; ++i ) {
        spawn_test_monster( mon_LS_fixbot_haywire.str(),
                            printer_pos + tripoint_rel_ms( 5 + i % 10, 5 + i / 10, 0 ) );
    }
    REQUIRE( count_monsters( mon_LS_fixbot_haywire ) == 49 );

    CHECK( mattack::fixbot_spawner_haywire( &printer ) );
    CHECK( count_monsters( mon_LS_fixbot_haywire ) == 50 );
    CHECK( mattack::fixbot_spawner_haywire( &printer ) );
    CHECK( count_monsters( mon_LS_fixbot_haywire ) == 50 );
}

// The fixbot obelisk checked for the wrong passcode before granting its own, so a player
// who already had it got the grant again.
TEST_CASE( "sync-89409_fixbot_obelisk_grants_its_passcode_once", "[eoc][sync]" )
{
    clear_avatar();
    avatar &u = get_avatar();
    u.set_value( "ls_had_a_passcode", "yes" );
    dialogue d( get_talker_for( u ), nullptr );

    u.set_mutation( trait_LS_Passcode_1 );
    CHECK( effect_on_condition_EOC_LS_Obelisk_fixbot_grant->test_condition( d ) );

    u.set_mutation( trait_LS_Passcode_fixbot );
    CHECK_FALSE( effect_on_condition_EOC_LS_Obelisk_fixbot_grant->test_condition( d ) );
    u.remove_value( "ls_had_a_passcode" );
    clear_avatar();
}

// The rotated roofs of the old lakeside cabins migrated to the ground-floor cabin, so an
// old save generated a second cabin a level up.
TEST_CASE( "sync-89596_lakeside_cabin_roofs_migrate_to_roofs", "[overmap][sync]" )
{
    std::unique_ptr<overmap> om = std::make_unique<overmap>( point_abs_om::zero );
    for( int variant = 1; variant <= 5; ++variant ) {
        for( const std::string dir : {
                 "north", "east", "south", "west"
             } ) {
            const std::string old_cabin = "cabin_lakeside" + std::to_string( variant ) + "_" + dir;
            const std::string old_roof = "cabin_lakeside" + std::to_string( variant ) + "_roof_" + dir;
            CAPTURE( old_roof );
            REQUIRE( overmap::is_oter_id_obsolete( old_roof ) );
            CHECK( om->get_or_migrate_oter( old_cabin ) == oter_id( "cabin_lakeside_" + dir ) );
            CHECK( om->get_or_migrate_oter( old_roof ) == oter_id( "cabin_lakeside_roof_" + dir ) );
        }
    }
}

// The annealing recipes replaced the old way to make medium carbon steel: carburizing
// budget or low carbon steel with coal. Both ways are in the steel lump menu.
TEST_CASE( "sync-89404_medium_carbon_steel_by_carburizing_or_annealing", "[recipe][sync]" )
{
    const recipe &carburize = recipe_mc_steel_lump.obj();
    CHECK( carburize.result() == itype_mc_steel_lump );
    CHECK( takes_component( carburize, itype_budget_steel_lump ) );
    CHECK( takes_component( carburize, itype_lc_steel_lump ) );
    CHECK( takes_component( carburize, itype_coal_lump ) );

    const recipe &anneal_qt = recipe_mc_steel_lump_from_qt_steel.obj();
    CHECK( anneal_qt.result() == itype_mc_steel_lump );
    CHECK( takes_component( anneal_qt, itype_qt_steel_lump ) );

    const recipe &anneal_ch = recipe_lc_steel_lump_from_ch_steel.obj();
    CHECK( anneal_ch.result() == itype_lc_steel_lump );
    CHECK( takes_component( anneal_ch, itype_ch_steel_lump ) );

    const std::set<recipe_id> &menu = recipe_nested_steel_lump->nested_category_data;
    CHECK( menu.count( recipe_mc_steel_lump ) == 1 );
    CHECK( menu.count( recipe_mc_steel_lump_from_qt_steel ) == 1 );
    CHECK( menu.count( recipe_lc_steel_lump_from_ch_steel ) == 1 );
}

// The air pistol was listed at 2400 g and 85 mm, shorter than its own magazine.
TEST_CASE( "sync-89369_air_pistol_holds_its_magazine", "[item][sync]" )
{
    item gun( itype_air_pistol_beretta_9mm );
    const item mag( itype_air_pistol_beretta_9mm_mag );
    CHECK( gun.length() >= mag.length() );
    CHECK( gun.weight() < 1500_gram );
    CHECK( gun.can_contain( mag ).success() );
}

// Generic Guns: the air pistol magazine migration had a trailing space in its id, so it never
// applied, and the mod's air pistol only took the vanilla magazine, not the mod's own.
TEST_CASE( "sync-89369_generic_guns_air_pistol_uses_the_mod_magazine", "[item][mods][sync]" )
{
    for( const std::string file : {
             "mods/Generic_Guns/magazines/gg_magazines_migration.json",
             "mods/Generic_Guns/firearms/gg_firearms_migration.json"
         } ) {
        CAPTURE( file );
        for( JsonObject jo : json_file( file ) ) {
            jo.allow_omitted_members();
            for( const std::string &id : jo.get_as_string_array( "id" ) ) {
                CAPTURE( id );
                CHECK( id.find( ' ' ) == std::string::npos );
                CHECK( id.find( '\t' ) == std::string::npos );
            }
        }
    }

    bool found = false;
    for( JsonObject jo : json_file( "mods/Generic_Guns/firearms/pneumatic.json" ) ) {
        jo.allow_omitted_members();
        if( jo.get_string( "id" ) != "air_pistol" ) {
            continue;
        }
        for( JsonObject pocket : jo.get_array( "pocket_data" ) ) {
            pocket.allow_omitted_members();
            if( pocket.get_string( "id", "" ) == "ammo" ) {
                found = true;
                const std::vector<std::string> mags = pocket.get_as_string_array( "item_restriction" );
                CHECK( std::find( mags.begin(), mags.end(), "air_pistol_mag" ) != mags.end() );
            }
        }
    }
    CHECK( found );
}

// Every topic the Marloss evangelists' dialogue goes to exists, and is that evangelist's own:
// Sera's first topic had the wrong case, Amaides named three topics that don't exist, and
// Ameth was sent into Joth's.
TEST_CASE( "sync-89494_marloss_evangelist_topics_exist", "[npc][dialogue][sync]" )
{
    for( const std::string file : {
             "json/npcs/marloss/MARLOSS_1_Joth_Luteus.json",
             "json/npcs/marloss/MARLOSS_2_Sera_Nameko.json",
             "json/npcs/marloss/MARLOSS_3_Amaides.json",
             "json/npcs/marloss/MARLOSS_4_Ameth_Aria.json"
         } ) {
        CAPTURE( file );
        std::set<std::string> defined;
        std::set<std::string> referenced;
        for( const JsonValue entry : json_file( file ) ) {
            JsonObject jo = entry.get_object();
            jo.allow_omitted_members();
            if( jo.get_string( "type" ) == "talk_topic" ) {
                for( const std::string &id : jo.get_as_string_array( "id" ) ) {
                    defined.insert( id );
                }
            }
            collect_topic_references( entry, referenced );
        }
        REQUIRE( !referenced.empty() );
        for( const std::string &topic : referenced ) {
            CAPTURE( topic );
            CHECK( talk_topic_all::talk_topic_exists( topic ) );
            CHECK( ( topic == "TALK_DONE" || topic == "TALK_NONE" || defined.count( topic ) == 1 ) );
        }
    }
}

// The HESCO barrier took 40 soil to build but gave back 100 to 160 when taken apart (the wall
// 200 to 320 for 80), and 70 to 110 when smashed.
TEST_CASE( "sync-89443_hesco_barriers_give_back_no_more_soil_than_they_took",
           "[construction][sync]" )
{
    const int barrier_soil = soil_put_in( construction_constr_hesco_mil3 );
    const int wall_soil = barrier_soil + soil_put_in( construction_constr_hesco_mil3_wall );
    REQUIRE( barrier_soil > 0 );

    CHECK( most_soil_from( furn_f_hesco_barrier->deconstruct->drop_group ) <= barrier_soil );
    CHECK( most_soil_from( furn_f_hesco_wall->deconstruct->drop_group ) <= wall_soil );
    // smashing the wall only breaks its top cell
    CHECK( most_soil_from( furn_f_hesco_barrier->bash->drop_group ) <= barrier_soil );
    CHECK( most_soil_from( furn_f_hesco_wall->bash->drop_group ) <= wall_soil - barrier_soil );
}

// The warlord's gas grenade added its random offset to an unused variable, so it always
// landed right on the target.
TEST_CASE( "sync-89427_warlord_gas_grenade_lands_near_the_target", "[monster][eoc][sync]" )
{
    clear_map();
    clear_avatar();
    set_time_to_day();
    map &here = get_map();
    avatar &u = get_avatar();
    const tripoint_bub_ms target( 40, 40, 0 );
    u.setpos( here, target );
    monster &warlord = spawn_test_monster( mon_wells_machine.str(), target + tripoint_rel_ms( 6, 0,
                                           0 ) );
    REQUIRE( u.sees( here, warlord ) );
    // its first bomb comes as soon as it is set up
    warlord.set_value( "timer_smoke", 0.0 );

    int off_target = 0;
    int too_far = 0;
    for( int i = 0; i < 20; ++i ) {
        set_time( calendar::turn + 1_minutes );
        dialogue d( get_talker_for( warlord ), get_talker_for( u ) );
        REQUIRE( effect_on_condition_eoc_wells_toxic_grenade->activate( d ) );
        int bombs = 0;
        for( const tripoint_bub_ms &p : here.points_in_radius( target, 6 ) ) {
            map_stack items = here.i_at( p );
            for( auto it = items.begin(); it != items.end(); ) {
                if( it->typeId() == itype_warlord_gasbomb_act ) {
                    bombs++;
                    off_target += p != target ? 1 : 0;
                    too_far += std::abs( p.x() - target.x() ) > 4 || std::abs( p.y() - target.y() ) > 4;
                    it = items.erase( it );
                } else {
                    ++it;
                }
            }
        }
        REQUIRE( bombs == 1 );
    }
    CHECK( off_target > 0 );
    CHECK( too_far == 0 );
}

// Warlord ids came from rand(1200), so two warlords could share one and swap legs.
TEST_CASE( "sync-89427_warlords_get_their_own_ids", "[monster][eoc][sync]" )
{
    clear_map();
    clear_avatar();
    avatar &u = get_avatar();
    u.setpos( get_map(), tripoint_bub_ms( 10, 10, 0 ) );
    std::vector<monster *> warlords;
    warlords.reserve( 64 );
    for( int i = 0; i < 64; ++i ) {
        warlords.push_back( &spawn_test_monster( mon_wells_machine.str(),
                            tripoint_bub_ms( 20 + 8 * ( i % 8 ), 20 + 8 * ( i / 8 ), 0 ) ) );
    }
    std::set<int> ids;
    for( monster *warlord : warlords ) {
        dialogue d( get_talker_for( *warlord ), get_talker_for( u ) );
        effect_on_condition_warlord_setup->activate( d );
        const int id = static_cast<int>( warlord->get_value( "wm_uid" ).dbl() );
        CHECK( id != 0 );
        ids.insert( id );
    }
    CHECK( ids.size() == warlords.size() );
    // each warlord's three legs carry its id
    for( monster *warlord : warlords ) {
        int legs = 0;
        for( const tripoint_bub_ms &p : get_map().points_in_radius( warlord->pos_bub(), 1 ) ) {
            const monster *leg = get_creature_tracker().creature_at<monster>( p );
            if( leg != nullptr && leg->type->id == mon_wells_machine_leg &&
                leg->get_value( "wm_id" ) == warlord->get_value( "wm_uid" ) ) {
                legs++;
            }
        }
        CHECK( legs == 3 );
    }
}

// Printing the timer tree divided by a node's own time, which is zero when the node only
// waited on its children.
TEST_CASE( "sync-89274_timer_report_survives_a_node_that_only_waited", "[sync]" )
{
    cata_timer::timer_stats parent( "parent" );
    parent.duration = 10;
    parent.wait_duration = 10;
    parent.count = 1;
    cata_timer::timer_stats child( "child" );
    child.wait = true;
    child.wait_duration = 10;
    child.count = 1;
    parent.child_timers.emplace( "child", child );
    cata_timer::timer_stats never_finished( "never_finished" );
    parent.child_timers.emplace( "never_finished", never_finished );
    parent.print_stats_recursively();
    CHECK( parent.count == 1 );
}

// MONAI-B03: a flood fill of range 0 returns its origin. It used to return nothing, so
// absorb_meat never found the meat under the monster.
TEST_CASE( "monai-b03_flood_fill_of_range_0_keeps_its_origin", "[map][audit]" )
{
    clear_map();
    const map &here = get_map();
    const tripoint_bub_ms origin( 50, 50, 0 );
    CHECK( here.reachable_flood_steps( origin, 0 ) == std::vector<tripoint_bub_ms> { origin } );
    const std::vector<tripoint_bub_ms> one = here.reachable_flood_steps( origin, 1 );
    CHECK( one.size() == 9 );
    CHECK( std::count( one.begin(), one.end(), origin ) == 1 );
}
