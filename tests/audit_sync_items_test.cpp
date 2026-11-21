#include <list>
#include <optional>
#include <string>

#include "activity_actor_definitions.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "character.h"
#include "construction.h"
#include "coordinates.h"
#include "debug.h"
#include "dialogue.h"
#include "effect_on_condition.h"
#include "global_vars.h"
#include "item.h"
#include "item_action.h"
#include "item_location.h"
#include "itype.h"
#include "iuse.h"
#include "map.h"
#include "map_helpers.h"
#include "mapdata.h"
#include "math_parser_diag_value.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "ret_val.h"
#include "sleep.h"
#include "stomach.h"
#include "talker.h"
#include "trap.h"
#include "type_id.h"

static const construction_str_id construction_constr_deconstruct( "constr_deconstruct" );

static const effect_on_condition_id
effect_on_condition_EOC_audit_sync_items_activate_npc( "EOC_audit_sync_items_activate_npc" );

static const furn_str_id furn_f_chair( "f_chair" );
static const furn_str_id furn_f_table( "f_table" );

static const itype_id itype_audit_sync_items_bud( "audit_sync_items_bud" );
static const itype_id itype_audit_sync_items_rune( "audit_sync_items_rune" );
static const itype_id itype_audit_sync_items_timer( "audit_sync_items_timer" );
static const itype_id itype_audit_sync_items_tin( "audit_sync_items_tin" );
static const itype_id itype_backpack( "backpack" );
static const itype_id itype_cash_card( "cash_card" );
static const itype_id itype_oxycodone( "oxycodone" );
static const itype_id itype_stick_long( "stick_long" );

static const ter_str_id ter_t_dirt( "t_dirt" );
static const ter_str_id ter_t_window( "t_window" );

static const trait_id trait_OPIATEHYPER( "OPIATEHYPER" );

static const trap_str_id tr_rollmat( "tr_rollmat" );

static const vitamin_id vitamin_vit_mme( "vit_mme" );

// Guards item_location::form_loc: an item processed with no carrier that is not where it is
// processed (map::add_item processes temperature-tracked items before placing them) got its dummy
// location from a null Character, so its countdown action dereferenced nullptr. It now gets a
// location pointing at the item, quietly: that is a normal case, not a lost item.
TEST_CASE( "sync-89473_countdown_without_carrier_gets_a_map_location", "[item][iuse]" )
{
    clear_map();
    map &here = get_map();
    global_variables &globvars = get_globals();
    globvars.clear_global_values();
    const tripoint_bub_ms pos( 60, 60, 0 );

    item timer( itype_audit_sync_items_timer );
    timer.active = true;
    timer.countdown_point = calendar::turn;
    const std::string msg = capture_debugmsg_during( [&]() {
        timer.process( here, nullptr, pos );
    } );
    CHECK( msg.empty() );
    CHECK( globvars.get_global_value( "audit_sync_items_runs" ) == 1 );
}

// Guards item_location::try_form_loc on a map that isn't the main one: a tinymap that loads a
// submap processes its active items, and a map_cursor finds items on the main map only, so the
// countdown of an item outside the main map logged "Couldn't find item" (and before the
// null-carrier fix, crashed).
TEST_CASE( "sync-89473_countdown_on_a_tinymap_finds_its_item", "[item][iuse]" )
{
    clear_map();
    map &here = get_map();
    global_variables &globvars = get_globals();
    globvars.clear_global_values();
    const tripoint_abs_ms far_pos = here.get_abs( tripoint_bub_ms( 60, 60, 0 ) ) + tripoint::east * 200;
    REQUIRE_FALSE( here.inbounds( here.get_bub( far_pos ) ) );

    tinymap far_map;
    far_map.load( project_to<coords::omt>( far_pos ), false );
    const tripoint_omt_ms far_at = far_map.get_omt( far_pos );
    item &timer = far_map.add_item( far_at, item( itype_audit_sync_items_timer ) );
    const on_out_of_scope clean_up( [far_pos]() {
        tinymap m;
        m.load( project_to<coords::omt>( far_pos ), false );
        m.i_clear( m.get_omt( far_pos ) );
    } );
    timer.active = true;
    timer.countdown_point = calendar::turn;

    map &far = *far_map.cast_to_map();
    const tripoint_bub_ms at( far_at.raw() );
    const std::optional<item_location> loc = item_location::try_form_loc( nullptr, &far, at, timer );
    REQUIRE( loc.has_value() );
    CHECK( loc->get_item() == &timer );

    const std::string msg = capture_debugmsg_during( [&]() {
        timer.process( far, nullptr, at );
    } );
    CHECK( msg.empty() );
    CHECK( globvars.get_global_value( "audit_sync_items_runs" ) == 1 );
}

// Guards Character::invoke_item: when the method belongs to an item inside the one invoked
// (u_activate on a container), the use action got the container's location, and a single-use
// item removed the container instead of itself.
TEST_CASE( "sync-89473_use_action_on_a_contained_item_gets_its_own_location",
           "[item][iuse][eoc]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    global_variables &globvars = get_globals();
    globvars.clear_global_values();
    const std::optional<std::list<item>::iterator> worn = u.wear_item( item( itype_backpack ),
            false );
    REQUIRE( worn.has_value() );
    item_location backpack( u, & **worn );
    item_location rune = u.i_add( item( itype_audit_sync_items_rune ) );
    REQUIRE( rune );
    REQUIRE( rune.parent_item() == backpack );

    dialogue d( get_talker_for( u ), get_talker_for( backpack ) );
    effect_on_condition_EOC_audit_sync_items_activate_npc->activate( d );

    CHECK( globvars.get_global_value( "audit_sync_items_runs" ) == 1 );
    CHECK( globvars.get_global_value( "audit_sync_items_used_id" ) == "audit_sync_items_rune" );
    CHECK( u.is_worn( *backpack ) );
    CHECK_FALSE( u.has_amount( itype_audit_sync_items_rune, 1 ) );
}

// Guards the same fix in item_action_generator::map_actions_to_items: it skipped every item
// with the action inside it, and never found an action on an item itself, so the item action
// menu offered nothing.
TEST_CASE( "sync-89473_item_action_menu_finds_carried_and_contained_items", "[item][iuse]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    REQUIRE( u.wear_item( item( itype_backpack ), false ).has_value() );
    item_location rune = u.i_add( item( itype_audit_sync_items_rune ) );
    REQUIRE( rune );
    u.set_wielded_item( item( itype_stick_long ) );

    const item_action_map actions = item_action_generator::generator().map_actions_to_items( u );
    const auto stick = actions.find( "BREAK_STICK" );
    REQUIRE( stick != actions.end() );
    CHECK( stick->second.get_item() == u.get_wielded_item().get_item() );
    const auto use_rune = actions.find( "effect_on_conditions" );
    REQUIRE( use_rune != actions.end() );
    CHECK( use_rune->second.get_item() == rune.get_item() );
}

// USE-P08: iuse_transform::can_use looked the item up again in the whole inventory to see
// whether the transformed item still fits its container.  It now uses the location it is
// given, and looks the item up only for a location made straight on the character.
TEST_CASE( "use-p08_transform_parent_pocket_check_uses_the_given_location",
           "[iuse_actor][item]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    u.set_wielded_item( item( itype_audit_sync_items_tin ) );
    item_location tin = u.get_wielded_item();
    REQUIRE( tin->put_in( item( itype_audit_sync_items_bud ), pocket_type::CONTAINER ).success() );
    const std::list<item *> inside = tin->all_items_top( pocket_type::CONTAINER );
    REQUIRE( inside.size() == 1 );
    item &bud = *inside.front();
    const use_function *use = bud.type->get_use( "transform" );
    REQUIRE( use != nullptr );
    const iuse_actor *actor = use->get_actor_ptr();
    REQUIRE( actor != nullptr );

    SECTION( "in the tin, the bloom would not fit" ) {
        const item_location in_tin( tin, &bud );
        const ret_val<void> ret = actor->can_use( u, in_tin, &here, u.pos_bub() );
        CHECK_FALSE( ret.success() );
        CHECK( ret.str().find( "too big for a parent pocket" ) != std::string::npos );
    }
    SECTION( "a location made straight on the character gives the same answer" ) {
        const item_location flat( u, &bud );
        CHECK_FALSE( actor->can_use( u, flat, &here, u.pos_bub() ).success() );
    }
    SECTION( "held in the hand, it transforms" ) {
        u.set_wielded_item( item( itype_audit_sync_items_bud ) );
        const item_location held = u.get_wielded_item();
        CHECK( actor->can_use( u, held, &here, u.pos_bub() ).success() );
    }
}

// Guards construct::check_deconstruct: since #89499 the terrain branch also passed when
// the (absent) furniture lacked NO_DECONSTRUCT, so "deconstruct" was offered on every tile
// without furniture, and failed after 20 minutes.
TEST_CASE( "sync-89499_deconstruct_needs_something_to_take_apart", "[construction]" )
{
    clear_avatar();
    clear_map();
    map &here = get_map();
    const construction &con = construction_constr_deconstruct.obj();
    const tripoint_bub_ms site = get_avatar().pos_bub() + tripoint::east;
    here.furn_set( site, furn_str_id::NULL_ID() );

    SECTION( "bare ground" ) {
        here.ter_set( site, ter_t_dirt );
        REQUIRE_FALSE( here.ter( site )->deconstruct.has_value() );
        CHECK_FALSE( can_construct( con, site ) );
    }
    SECTION( "terrain that comes apart" ) {
        here.ter_set( site, ter_t_window );
        REQUIRE( here.ter( site )->deconstruct.has_value() );
        CHECK( can_construct( con, site ) );
    }
    SECTION( "furniture that comes apart" ) {
        here.ter_set( site, ter_t_dirt );
        here.furn_set( site, furn_f_table );
        REQUIRE( here.furn( site )->deconstruct.has_value() );
        CHECK( can_construct( con, site ) );
    }
}

// Guards the ATM after #89166 removed cash cards: buying one built the undefined cash_card
// (a debugmsg and an undefined item) and charged $10.  The menu no longer offers it, and a
// purchase saved before the update is refused.
TEST_CASE( "sync-89166_atm_sells_no_removed_cash_cards", "[activity][iexamine]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    REQUIRE_FALSE( item::type_is_defined( itype_cash_card ) );
    u.cash = 2000;

    u.assign_activity( atm_activity_actor( purchase_card ) );
    const std::string msg = capture_debugmsg_during( [&]() {
        process_activity( u );
    } );
    CAPTURE( msg );
    CHECK( msg.empty() );
    CHECK( u.cash == 2000 );
    CHECK_FALSE( u.has_item_with( []( const item & it ) {
        return it.typeId() == itype_cash_card;
    } ) );
}

// Guards Opiate Intolerance (#89621): its vitamins_absorb_multi was keyed on the vitamin
// vit_mme, not a material, so the twelvefold dose never applied.
TEST_CASE( "sync-89621_opiate_intolerance_multiplies_opiate_doses", "[mutation][vitamin]" )
{
    clear_avatar();
    avatar &u = get_avatar();
    const item pill( itype_oxycodone );
    const int usual = u.compute_effective_nutrients( pill ).get_vitamin( vitamin_vit_mme );
    REQUIRE( usual > 0 );

    u.set_mutation( trait_OPIATEHYPER );
    REQUIRE( u.has_trait( trait_OPIATEHYPER ) );
    CHECK( u.compute_effective_nutrients( pill ).get_vitamin( vitamin_vit_mme ) == usual * 12 );
}

// Guards sleep comfort after #89346: it took the best of furniture and trap (so a rollmat on
// furniture counts), but started from 0, so bare floor and rough ground lost their discomfort.
TEST_CASE( "sync-89346_sleeping_on_the_ground_stays_uncomfortable", "[sleep]" )
{
    clear_map();
    map &here = get_map();
    const tripoint_bub_ms spot( 60, 60, 0 );
    here.ter_set( spot, ter_t_dirt );
    here.furn_set( spot, furn_str_id::NULL_ID() );
    const on_out_of_scope clear_trap( [&here, &spot]() {
        here.remove_trap( spot );
    } );

    SECTION( "bare ground" ) {
        CHECK( comfort_data::human_comfort_at( spot ) == here.ter( spot )->comfort -
               here.move_cost( spot ) );
        const int neutral = comfort_data::COMFORT_NEUTRAL;
        CHECK( comfort_data::human_comfort_at( spot ) < neutral );
    }
    SECTION( "a rollmat on a chair counts" ) {
        here.furn_set( spot, furn_f_chair );
        here.trap_set( spot, tr_rollmat );
        REQUIRE( tr_rollmat->comfort > furn_f_chair->comfort );
        CHECK( comfort_data::human_comfort_at( spot ) == tr_rollmat->comfort );
    }
}
