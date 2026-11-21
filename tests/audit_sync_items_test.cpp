#include <list>
#include <optional>
#include <string>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
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
#include "math_parser_diag_value.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "ret_val.h"
#include "talker.h"
#include "type_id.h"

static const effect_on_condition_id
effect_on_condition_EOC_audit_sync_items_activate_npc( "EOC_audit_sync_items_activate_npc" );

static const itype_id itype_audit_sync_items_bud( "audit_sync_items_bud" );
static const itype_id itype_audit_sync_items_rune( "audit_sync_items_rune" );
static const itype_id itype_audit_sync_items_timer( "audit_sync_items_timer" );
static const itype_id itype_audit_sync_items_tin( "audit_sync_items_tin" );
static const itype_id itype_backpack( "backpack" );
static const itype_id itype_stick_long( "stick_long" );

// Guards the fix to item_location::form_loc: an item processed with no carrier that is not
// where it is processed (map::add_item processes new items before placing them) got its dummy
// location from a null Character, so its countdown action dereferenced nullptr.
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
    CAPTURE( msg );
    CHECK( msg.find( "Couldn't find item" ) != std::string::npos );
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
