#include <functional>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "activity_actor_definitions.h"
#include "activity_handlers.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "character.h"
#include "coordinates.h"
#include "debug.h"
#include "iexamine.h"
#include "iexamine_firestarters.h"
#include "item.h"
#include "item_location.h"
#include "json.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "mapdata.h"
#include "messages.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "type_id.h"
#include "units.h"
#include "veh_utils.h"
#include "vehicle.h"
#include "vpart_position.h"

static const activity_id ACT_PICKAXE( "ACT_PICKAXE" );
static const activity_id ACT_REPAIR_ITEM( "ACT_REPAIR_ITEM" );

static const itype_id itype_backpack( "backpack" );
static const itype_id itype_butane( "butane" );
static const itype_id itype_crowbar( "crowbar" );
static const itype_id itype_leather( "leather" );
static const itype_id itype_lighter( "lighter" );
static const itype_id itype_pickaxe( "pickaxe" );
static const itype_id itype_picklocks( "picklocks" );
static const itype_id itype_steel_chunk( "steel_chunk" );
static const itype_id itype_test_baseball( "test_baseball" );
static const itype_id itype_test_mmerge_spool( "test_mmerge_spool" );
static const itype_id itype_test_mmerge_spool_kit( "test_mmerge_spool_kit" );
static const itype_id itype_test_mmerge_steel_plate( "test_mmerge_steel_plate" );
static const itype_id itype_test_mmerge_vehicle_welder( "test_mmerge_vehicle_welder" );
static const itype_id itype_test_mmerge_worn_pick( "test_mmerge_worn_pick" );
static const itype_id itype_thread( "thread" );

static const skill_id skill_fabrication( "fabrication" );
static const skill_id skill_tailor( "tailor" );

static const ter_str_id ter_t_door_locked( "t_door_locked" );
static const ter_str_id ter_t_rock( "t_rock" );

static const vpart_id vpart_frame( "frame" );
static const vpart_id vpart_small_storage_battery( "small_storage_battery" );
static const vpart_id vpart_test_mmerge_arcade( "test_mmerge_arcade" );
static const vpart_id vpart_test_mmerge_welding_bench( "test_mmerge_welding_bench" );

static const vproto_id vehicle_prototype_none( "none" );

static bool has_message_containing( const std::string &text )
{
    for( const std::pair<std::string, std::string> &msg :
         Messages::recent_messages( Messages::size() ) ) {
        if( msg.second.find( text ) != std::string::npos ) {
            return true;
        }
    }
    return false;
}

static int count_at( const tripoint_bub_ms &p, const itype_id &type )
{
    int found = 0;
    for( const item &it : get_map().i_at( p ) ) {
        if( it.typeId() == type ) {
            found++;
        }
    }
    return found;
}

static item charged_lighter()
{
    item lighter( itype_lighter );
    lighter.ammo_set( itype_butane, 100 );
    return lighter;
}

// A one-tile vehicle east of the avatar, made of a frame and the given parts.
static vehicle &vehicle_beside( map &here, const std::vector<vpart_id> &parts )
{
    const tripoint_bub_ms pos = get_avatar().pos_bub() + tripoint_rel_ms::east;
    vehicle *veh = here.add_vehicle( vehicle_prototype_none, pos, 0_degrees, 0,
                                     veh_spawn_status::UNDAMAGED );
    REQUIRE( veh != nullptr );
    REQUIRE( veh->install_part( here, point_rel_ms::zero, vpart_frame ) != -1 );
    for( const vpart_id &part : parts ) {
        REQUIRE( veh->install_part( here, point_rel_ms::zero, part ) != -1 );
    }
    veh->refresh();
    here.add_vehicle_to_cache( veh );
    return *veh;
}

// #89473 inverted both checks for the chosen tool in the mining zone job, so the job read a
// tool through an empty item_location (a crash) instead of mining with the pickaxe it had.
TEST_CASE( "sync-89473_mining_zone_job_mines_with_a_pickaxe", "[activity][zones]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms rock = u.pos_bub() + tripoint_rel_ms::east;
    here.ter_set( rock, ter_t_rock );
    REQUIRE( here.has_flag( ter_furn_flag::TFLAG_MINEABLE, rock ) );
    u.set_wielded_item( item( itype_pickaxe ) );

    multi_mine_activity_actor zone_job;
    const activity_reason_info can_do = zone_job.multi_activity_can_do( u, rock );
    REQUIRE( can_do.can_do );
    // false: the job handed the character a mining activity
    CHECK_FALSE( zone_job.multi_activity_do( u, can_do, here.get_abs( rock ), rock ) );
    CHECK( u.activity.id() == ACT_PICKAXE );
}

// #89473 made the firestarter search take only items with both the FIRESTARTER and FIRE
// flags, and then skip every one that had a firestarter action. Lighting a kiln, smoker or
// other process furniture from its examine menu never found the lighter the avatar carried.
TEST_CASE( "sync-89473_carried_firestarter_can_light_process_furniture", "[iexamine][fire]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    const tripoint_bub_ms kiln = u.pos_bub() + tripoint_rel_ms::east;
    u.i_add( charged_lighter() );

    const std::multimap<int, item_location> found = iexamine::find_firestarters( u, kiln, false );
    REQUIRE( found.size() == 1 );
    CHECK( found.begin()->second->typeId() == itype_lighter );
}

// #89473 made the fireplace look for firestarters on its own tile once for every tile around
// it, so a lighter lying beside it was never found and one inside it was.
TEST_CASE( "sync-89473_fireplace_finds_firestarters_on_the_tiles_around_it", "[iexamine][fire]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms fireplace = u.pos_bub() + tripoint_rel_ms::east;
    const tripoint_bub_ms beside = u.pos_bub() + tripoint_rel_ms::west;

    SECTION( "a lighter on a nearby tile is found" ) {
        here.add_item( beside, charged_lighter() );
        CHECK( iexamine::find_firestarters( u, fireplace, false ).empty() );
        const std::multimap<int, item_location> found = iexamine::find_firestarters( u, fireplace,
                true );
        REQUIRE( found.size() == 1 );
        CHECK( found.begin()->second.pos_bub( here ) == beside );
    }
    SECTION( "a lighter in the fireplace is not" ) {
        here.add_item( fireplace, charged_lighter() );
        CHECK( iexamine::find_firestarters( u, fireplace, true ).empty() );
    }
}

// #89473 inverted the worn-item check when picking a lock, so only worn items (hairpins) were
// offered and a lockpick in hand or in a pocket never was. The query is answered no in tests;
// whether anything was offered shows in the "nothing to pick it with" message.
TEST_CASE( "sync-89473_held_lockpicks_are_offered_and_worn_ones_are_not", "[iexamine][lockpick]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms door = u.pos_bub() + tripoint_rel_ms::east;
    here.ter_set( door, ter_t_door_locked );
    const std::string nothing_to_pick_with = "If only you had something to pick its lock with";
    Messages::clear_messages();

    SECTION( "a held lockpick" ) {
        u.set_wielded_item( item( itype_picklocks ) );
        iexamine::locked_object_pickable( u, door );
        CHECK_FALSE( has_message_containing( nothing_to_pick_with ) );
    }
    SECTION( "a worn one" ) {
        REQUIRE( u.wear_item( item( itype_test_mmerge_worn_pick ), false ).has_value() );
        iexamine::locked_object_pickable( u, door );
        CHECK( has_message_containing( nothing_to_pick_with ) );
    }
    Messages::clear_messages();
}

// #89473's arcade machine entry captured its item_location by reference, so the callback,
// which runs after the menu is built, read a location that was gone (found by ASan). Here the
// board has no power, so the game stops at the message that names it.
TEST_CASE( "sync-89473_arcade_machine_entry_keeps_its_item", "[vehicle][iuse]" )
{
    clear_avatar();
    clear_map();
    clear_vehicles();
    map &here = get_map();
    vehicle &veh = vehicle_beside( here, { vpart_test_mmerge_arcade } );
    for( const vpart_reference &vp : veh.get_any_parts( "ARCADE" ) ) {
        vp.part().enabled = true;
    }
    Messages::clear_messages();

    veh_menu menu( veh, "test" );
    veh.build_electronics_menu( here, menu );
    bool played = false;
    for( const veh_menu_item &entry : menu.get_items() ) {
        if( entry._text == "Play arcade machine" ) {
            entry._on_submit();
            played = true;
        }
    }
    REQUIRE( played );
    CHECK( has_message_containing( "test dead game board" ) );
    CHECK( has_message_containing( "batteries are dead" ) );
    Messages::clear_messages();
    clear_vehicles();
}

// #89451 passed a carried repair tool's position to consume_tool_uses as the vehicle tool's
// tile, an empty optional. The spool the tool uses drops out where the tool is once it is empty,
// so it must land at the repairer's feet.
TEST_CASE( "sync-89451_carried_repair_tool_uses_its_own_position", "[activity][repair]" )
{
    restore_on_out_of_scope restore_turn( calendar::turn );
    clear_avatar();
    clear_map_without_vision();
    set_time_to_day();
    avatar &u = get_avatar();
    map &here = get_map();
    REQUIRE( static_cast<int>( here.light_at( u.pos_bub() ) ) > 2 );
    u.set_skill_level( skill_tailor, 10 );

    REQUIRE( u.wear_item( item( itype_backpack ), false ).has_value() );
    item spool( itype_test_mmerge_spool );
    spool.ammo_set( itype_thread, 1 );
    item kit( itype_test_mmerge_spool_kit );
    REQUIRE( kit.put_in( spool, pocket_type::MAGAZINE_WELL ).success() );
    item_location kit_loc = u.i_add( kit );
    REQUIRE( kit_loc );
    REQUIRE( kit_loc->ammo_sufficient( &u ) );
    item leather( itype_leather );
    leather.charges = 10;
    u.i_add_or_drop( leather, 10 );
    item ball( itype_test_baseball );
    ball.set_damage( 1000 );
    REQUIRE( u.wield( ball ) );

    u.assign_activity( repair_item_activity_actor( u.get_wielded_item(), kit_loc, std::nullopt,
                       "repair_fabric", repair_stage::ONCE, true ) );
    REQUIRE( u.activity.id() == ACT_REPAIR_ITEM );
    u.process_activity_until_invalid();

    CHECK( count_at( u.pos_bub(), itype_test_mmerge_spool ) == 1 );
}

// #89451 went on after "type descriptor and actual type mismatch" with a null repair action,
// and crashed reading it. The activity must just end.
TEST_CASE( "sync-89451_repair_with_another_use_action_ends", "[activity][repair]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    REQUIRE( u.wear_item( item( itype_backpack ), false ).has_value() );
    item_location crowbar = u.i_add( item( itype_crowbar ) );
    REQUIRE( crowbar );
    item ball( itype_test_baseball );
    ball.set_damage( 1000 );
    REQUIRE( u.wield( ball ) );

    u.assign_activity( repair_item_activity_actor( u.get_wielded_item(), crowbar, std::nullopt,
                       "CROWBAR", repair_stage::ONCE, true ) );
    REQUIRE( u.activity.id() == ACT_REPAIR_ITEM );
    const std::string error = capture_debugmsg_during( [&u]() {
        u.process_activity_until_invalid();
    } );
    CHECK( error.find( "actual type mismatch" ) != std::string::npos );
    CHECK( u.activity.is_null() );
}

// VINT-J09: a repair tool a vehicle provides is lent the vehicle's power. Starting the repair
// from the vehicle menu uses none of it, and each repair attempt bills the vehicle once for
// what that attempt used: the tool's charges_per_use, 5.
TEST_CASE( "vint-j09_vehicle_repair_tool_bills_the_vehicle_once_per_use", "[vehicle][repair]" )
{
    restore_on_out_of_scope restore_turn( calendar::turn );
    clear_avatar();
    clear_map_without_vision();
    clear_vehicles();
    set_time_to_day();
    avatar &u = get_avatar();
    map &here = get_map();
    REQUIRE( static_cast<int>( here.light_at( u.pos_bub() ) ) > 2 );
    u.set_skill_level( skill_fabrication, 10 );
    vehicle &veh = vehicle_beside( here, { vpart_small_storage_battery, vpart_test_mmerge_welding_bench } );
    veh.charge_battery( here, 1000 );
    const tripoint_bub_ms bench = u.pos_bub() + tripoint_rel_ms::east;
    REQUIRE( here.veh_at( bench ).part_with_tool( here,
             itype_test_mmerge_vehicle_welder ).has_value() );
    const int battery_before = static_cast<int>( veh.battery_left( here ) );
    REQUIRE( battery_before >= 1000 );

    SECTION( "the vehicle menu starts the repair without billing" ) {
        REQUIRE( vehicle::use_vehicle_tool( veh, &here, bench, itype_test_mmerge_vehicle_welder,
                                            false ) );
        CHECK( u.activity.id() == ACT_REPAIR_ITEM );
        CHECK( static_cast<int>( veh.battery_left( here ) ) == battery_before );
        // the repair works with the vehicle's tool, which it bills as it goes
        std::ostringstream saved;
        JsonOut jsout( saved );
        u.activity.serialize( jsout );
        CHECK( saved.str().find( R"("vp_tool")" ) != std::string::npos );
        CHECK( saved.str().find( itype_test_mmerge_vehicle_welder.str() ) != std::string::npos );
        u.cancel_activity();
    }
    SECTION( "one repair attempt bills one use" ) {
        item chunks( itype_steel_chunk );
        u.i_add_or_drop( chunks, 10 );
        item plate( itype_test_mmerge_steel_plate );
        plate.set_damage( 2000 );
        REQUIRE( u.wield( plate ) );
        u.assign_activity( repair_item_activity_actor( u.get_wielded_item(), item_location::nowhere,
                           std::make_pair( itype_test_mmerge_vehicle_welder, bench ), "repair_metal",
                           repair_stage::ONCE, true ) );
        REQUIRE( u.activity.id() == ACT_REPAIR_ITEM );
        u.process_activity_until_invalid();
        CHECK( static_cast<int>( veh.battery_left( here ) ) == battery_before - 5 );
    }
    clear_vehicles();
}
