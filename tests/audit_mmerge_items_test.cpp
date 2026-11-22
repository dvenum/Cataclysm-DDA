#include <functional>
#include <string>

#include "activity_actor_definitions.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "coordinates.h"
#include "debug.h"
#include "flag.h"
#include "global_vars.h"
#include "iexamine.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "iuse.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "map_selector.h"
#include "math_parser_diag_value.h"
#include "npc.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "point.h"
#include "talker_npc.h"
#include "type_id.h"
#include "vehicle.h"
#include "vpart_position.h"

static const activity_id ACT_REPAIR_ITEM( "ACT_REPAIR_ITEM" );

static const itype_id itype_backpack( "backpack" );
static const itype_id itype_cash_card( "cash_card" );
static const itype_id itype_cash_card_test( "cash_card_test" );
static const itype_id itype_money( "money" );
static const itype_id itype_rock( "rock" );
static const itype_id itype_test_m24a_mending_kit( "test_m24a_mending_kit" );
static const itype_id itype_test_mmerge_items_cracker( "test_mmerge_items_cracker" );
static const itype_id itype_test_mmerge_items_pill( "test_mmerge_items_pill" );
static const itype_id itype_test_mmerge_items_vehicle_rune( "test_mmerge_items_vehicle_rune" );

static const vpart_id vpart_frame( "frame" );
static const vpart_id vpart_test_m24a_mending_bench( "test_m24a_mending_bench" );
static const vpart_id vpart_test_mmerge_items_rune_bench( "test_mmerge_items_rune_bench" );

static const vproto_id vehicle_prototype_none( "none" );

namespace
{

// clear of the avatar, which clear_avatar() puts at 60,60
constexpr tripoint_bub_ms veh_pos( 63, 60, 0 );

item_location carry_cash_card( avatar &u, const int cents )
{
    item card( itype_cash_card_test );
    card.ammo_set( card.ammo_default(), cents );
    item_location carried = u.i_add( card );
    REQUIRE( carried );
    return carried;
}

// A one-tile vehicle with the part, which provides a tool, at veh_pos
vehicle &place_tool_bench( map &here, const vpart_id &bench )
{
    vehicle *veh = here.add_vehicle( vehicle_prototype_none, veh_pos, 0_degrees, 0,
                                     veh_spawn_status::UNDAMAGED );
    REQUIRE( veh != nullptr );
    REQUIRE( veh->install_part( here, point_rel_ms::zero, vpart_frame ) != -1 );
    REQUIRE( veh->install_part( here, point_rel_ms::zero, bench ) != -1 );
    here.add_vehicle_to_cache( veh );
    return *veh;
}

void reset_world()
{
    clear_avatar();
    clear_map();
    clear_vehicles();
    get_globals().clear_global_values();
    set_time_to_day();
}

repair_item_activity_actor &repair_activity( avatar &u )
{
    REQUIRE( u.activity.id() == ACT_REPAIR_ITEM );
    repair_item_activity_actor *actor = dynamic_cast<repair_item_activity_actor *>
                                        ( u.activity.actor.get() );
    REQUIRE( actor != nullptr );
    return *actor;
}

// The tool the started repair works with, and whatever debug messages finding it logged
item *repair_tool( avatar &u, std::string &msg )
{
    repair_item_activity_actor &actor = repair_activity( u );
    item *tool = nullptr;
    msg = capture_debugmsg_during( [&]() {
        tool = actor.get_repair_tool( u.activity );
    } );
    return tool;
}

} // namespace

// Guards the vending machine, the ATM and the gas pump after #89166 made vanilla's cash cards
// obsolete: they knew a cash card only by the id cash_card, which vanilla no longer defines.  A
// cash card is now any item that stores money (Aftershock's, the test data's).
TEST_CASE( "sync-89166_cards_that_store_money_pay_and_take_deposits", "[iexamine][activity]" )
{
    clear_avatar();
    const on_out_of_scope cleanup( []() {
        clear_avatar();
    } );
    avatar &u = get_avatar();
    REQUIRE_FALSE( item::type_is_defined( itype_cash_card ) );

    CHECK( item( itype_cash_card_test ).is_cash_card() );
    CHECK_FALSE( item( itype_rock ).is_cash_card() );
    CHECK_FALSE( item( itype_money ).is_cash_card() );

    u.wear_item( item( itype_backpack ), false );
    item_location small = carry_cash_card( u, 300 );
    item_location large = carry_cash_card( u, 500 );
    REQUIRE( iexamine::cash_card_money( u ) == 800 );

    SECTION( "a vending machine bills both cards" ) {
        CHECK( iexamine::vending_pay( u, 600, false ) );
        CHECK( iexamine::cash_card_money( u ) == 200 );
        CHECK( small->ammo_remaining() + large->ammo_remaining() == 200 );
        CHECK_FALSE( iexamine::vending_pay( u, 201, false ) );
        CHECK( iexamine::cash_card_money( u ) == 200 );
    }
    SECTION( "an ATM takes a deposit from the cards" ) {
        u.cash = 1000;
        u.assign_activity( atm_activity_actor( deposit_money, 700 ) );
        process_activity( u );
        CHECK( iexamine::cash_card_money( u ) == 100 );
        CHECK( u.cash == 1700 );
    }
    SECTION( "an ATM refuses a deposit the cards can't cover" ) {
        u.cash = 1000;
        u.assign_activity( atm_activity_actor( deposit_money, 900 ) );
        process_activity( u );
        CHECK( iexamine::cash_card_money( u ) == 800 );
        CHECK( u.cash == 1000 );
    }
    SECTION( "an ATM pays a withdrawal onto the cards" ) {
        u.cash = 1000;
        u.assign_activity( atm_activity_actor( withdraw_money, 400 ) );
        process_activity( u );
        CHECK( iexamine::cash_card_money( u ) == 1200 );
        CHECK( u.cash == 600 );
    }
    SECTION( "an ATM puts all the money on one card" ) {
        u.assign_activity( atm_activity_actor( transfer_all_money ) );
        process_activity( u );
        CHECK( iexamine::cash_card_money( u ) == 800 );
        CHECK( ( small->ammo_remaining() == 0 || large->ammo_remaining() == 0 ) );
    }
}

// VINT-J09: a repair tool a vehicle provides is a PSEUDO stand-in that is gone once its use
// action returns.  repair_item_actor::use hands the repair the vehicle's tool for any PSEUDO
// tool on a vehicle tile (upstream listed six tool ids), so the repair doesn't keep a location
// to the stand-in.
TEST_CASE( "vint-j09_a_pseudo_tool_on_a_vehicle_tile_is_the_vehicles_tool",
           "[vehicle][iuse_actor]" )
{
    reset_world();
    const on_out_of_scope cleanup( []() {
        get_avatar().cancel_activity();
        clear_vehicles();
        clear_map();
        clear_avatar();
    } );
    map &here = get_map();
    avatar &u = get_avatar();
    place_tool_bench( here, vpart_test_m24a_mending_bench );
    REQUIRE( here.veh_at( veh_pos ).part_with_tool( here, itype_test_m24a_mending_kit ).has_value() );
    std::string msg;

    SECTION( "a PSEUDO tool on the vehicle's tile" ) {
        {
            item stand_in( itype_test_m24a_mending_kit );
            stand_in.set_flag( flag_PSEUDO );
            item_location stand_in_loc( map_cursor( &here, veh_pos ), &stand_in );
            const use_function *use = stand_in.type->get_use( "repair_metal" );
            REQUIRE( use != nullptr );
            use->call( &u, stand_in_loc, &here, veh_pos );
        }
        // the stand-in is gone: the repair finds the vehicle's own
        item *tool = repair_tool( u, msg );
        CAPTURE( msg );
        CHECK( msg.empty() );
        REQUIRE( tool != nullptr );
        CHECK( tool->typeId() == itype_test_m24a_mending_kit );
    }
    SECTION( "a PSEUDO tool off any vehicle is used where it is" ) {
        const tripoint_bub_ms off_vehicle = veh_pos + tripoint_rel_ms::south * 2;
        REQUIRE_FALSE( here.veh_at( off_vehicle ).has_value() );
        item stand_in( itype_test_m24a_mending_kit );
        stand_in.set_flag( flag_PSEUDO );
        item_location stand_in_loc( map_cursor( &here, off_vehicle ), &stand_in );
        stand_in.type->get_use( "repair_metal" )->call( &u, stand_in_loc, &here, off_vehicle );
        CHECK( repair_tool( u, msg ) == &stand_in );
        CHECK( msg.empty() );
    }
    SECTION( "a carried tool on the vehicle's tile is used where it is" ) {
        item_location kit = u.i_add( item( itype_test_m24a_mending_kit ) );
        REQUIRE( kit );
        kit->type->get_use( "repair_metal" )->call( &u, kit, &here, veh_pos );
        CHECK( repair_tool( u, msg ) == kit.get_item() );
        CHECK( msg.empty() );
    }
}

// Guards vehicle::use_vehicle_tool: #89473 commented out its invoke_item, so a tool used from the
// vehicle menu did nothing.  The use action gets a stand-in for the vehicle's tool; nothing may
// keep the stand-in or use it up.
TEST_CASE( "sync-89473_vehicle_menu_tools_are_used_again", "[vehicle][iuse]" )
{
    reset_world();
    const on_out_of_scope cleanup( []() {
        get_avatar().cancel_activity();
        clear_vehicles();
        clear_map();
        clear_avatar();
        get_globals().clear_global_values();
    } );
    map &here = get_map();
    avatar &u = get_avatar();
    std::string msg;

    SECTION( "a repair tool starts a repair with the vehicle's tool" ) {
        vehicle &veh = place_tool_bench( here, vpart_test_m24a_mending_bench );
        msg = capture_debugmsg_during( [&]() {
            CHECK( vehicle::use_vehicle_tool( veh, &here, veh_pos, itype_test_m24a_mending_kit ) );
        } );
        CAPTURE( msg );
        CHECK( msg.empty() );
        item *tool = repair_tool( u, msg );
        CHECK( msg.empty() );
        REQUIRE( tool != nullptr );
        CHECK( tool->typeId() == itype_test_m24a_mending_kit );
    }
    SECTION( "a use that would use up a carried tool leaves the vehicle's" ) {
        vehicle &veh = place_tool_bench( here, vpart_test_mmerge_items_rune_bench );
        msg = capture_debugmsg_during( [&]() {
            CHECK( vehicle::use_vehicle_tool( veh, &here, veh_pos, itype_test_mmerge_items_vehicle_rune ) );
        } );
        CAPTURE( msg );
        CHECK( msg.empty() );
        CHECK( get_globals().get_global_value( "test_mmerge_items_rune_uses" ) == 1 );
        CHECK( here.veh_at( veh_pos ).part_with_tool( here,
                itype_test_mmerge_items_vehicle_rune ).has_value() );
    }
}

// Guards talker_npc's try_consume: #89473 made it work on a copy of the offered item, so food or
// medicine an NPC was given used up charges of the copy only, and a stack never shrank until its
// last charge.
TEST_CASE( "sync-89473_an_npc_uses_up_the_food_and_medicine_it_is_given", "[npc][talker]" )
{
    clear_avatar();
    clear_map();
    clear_npcs();
    const on_out_of_scope cleanup( []() {
        clear_npcs();
        clear_avatar();
    } );
    avatar &u = get_avatar();
    u.wear_item( item( itype_backpack ), false );
    npc &guy = spawn_npc( u.pos_bub().xy() + point_rel_ms( 1, 0 ), "thug" );
    talker_npc talker( &guy );

    SECTION( "medicine" ) {
        item_location pills = u.i_add( item( itype_test_mmerge_items_pill, calendar::turn, 3 ) );
        REQUIRE( pills );
        REQUIRE( pills->charges == 3 );
        talker.give_item_to( pills, true );
        REQUIRE( pills );
        CHECK( pills->charges == 2 );
        talker.give_item_to( pills, true );
        talker.give_item_to( pills, true );
        CHECK_FALSE( u.has_amount( itype_test_mmerge_items_pill, 1 ) );
    }
    SECTION( "food" ) {
        guy.set_stored_kcal( guy.get_healthy_kcal() / 2 );
        guy.set_hunger( 300 );
        item_location crackers = u.i_add( item( itype_test_mmerge_items_cracker, calendar::turn, 3 ) );
        REQUIRE( crackers );
        REQUIRE( crackers->charges == 3 );
        talker.give_item_to( crackers, true );
        REQUIRE( crackers );
        CHECK( crackers->charges == 2 );
    }
}
