#include <string>

#include "avatar.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "character.h"
#include "debug.h"
#include "dialogue.h"
#include "effect_on_condition.h"
#include "magic.h"
#include "map.h"
#include "map_helpers.h"
#include "options_helpers.h"
#include "pimpl.h"
#include "player_helpers.h"
#include "talker.h"
#include "type_id.h"

static const effect_on_condition_id
effect_on_condition_EOC_SALAMANDER_SPELL_EXPERIENCE_INCREASER( "EOC_SALAMANDER_SPELL_EXPERIENCE_INCREASER" );
static const effect_on_condition_id
effect_on_condition_EOC_SYLPH_SPELL_EXPERIENCE_INCREASER( "EOC_SYLPH_SPELL_EXPERIENCE_INCREASER" );

static const skill_id skill_gramarye( "gramarye" );

static const spell_id spell_create_light_salamander( "create_light_salamander" );
static const spell_id spell_ominous_wind( "ominous_wind" );

static const trait_id trait_CREATE_LIGHT( "CREATE_LIGHT" );
static const trait_id trait_OMINOUS_WIND( "OMINOUS_WIND" );
static const trait_id trait_SALAMANDER( "SALAMANDER" );
static const trait_id trait_SYLPH( "SYLPH" );

static const weather_type_id weather_rain( "rain" );

// Runs @p eoc on @p who; true when it logged nothing
static bool runs_quietly( const effect_on_condition_id &eoc, Character &who )
{
    REQUIRE( eoc.is_valid() );
    dialogue d( get_talker_for( who ), nullptr );
    const std::string messages = capture_debugmsg_during( [&]() {
        eoc->activate( d );
    } );
    CAPTURE( eoc.str(), messages );
    return messages.empty();
}

// #89544 made Salamander and Sylph passive spell XP gramarye times an xp_multiplier that the
// recurring caller passes to its selector, and #89607 added Sylph spells.  Run where it applies,
// each caller gives XP to a known spell.  The Salamander caller also looked for a
// SALAMANDER_CREATE_LIGHT trait that doesn't exist, so a Salamander whose only spell came from
// Create Light got none.  Needs the mod: run with --mods=no_npc_food,xedra_evolved "[xedra_evolved]".
TEST_CASE( "sync-89544_paraclesian_callers_give_passive_xp", "[.][xedra_evolved]" )
{
    clear_map();
    clear_avatar();
    on_out_of_scope restore( []() {
        clear_avatar();
    } );
    avatar &u = get_avatar();
    u.set_skill_level( skill_gramarye, 1 );
    const auto xp_gained = [&u]( const effect_on_condition_id & caller, const spell_id & sp ) {
        u.magic->learn_spell( sp, u, true );
        const int before = u.magic->get_spell( sp ).xp();
        CHECK( runs_quietly( caller, u ) );
        return u.magic->get_spell( sp ).xp() - before;
    };

    SECTION( "a Salamander near a fire, whose only spell is Create Light" ) {
        u.set_mutation( trait_SALAMANDER );
        u.set_mutation( trait_CREATE_LIGHT );
        u.set_value( "salamander_near_fire", 1 );
        CHECK( xp_gained( effect_on_condition_EOC_SALAMANDER_SPELL_EXPERIENCE_INCREASER,
                          spell_create_light_salamander ) > 0 );
    }
    SECTION( "a Sylph outdoors in the rain" ) {
        scoped_weather_override rain( weather_rain );
        get_map().build_map_cache( 0, true );
        u.set_mutation( trait_SYLPH );
        u.set_mutation( trait_OMINOUS_WIND );
        CHECK( xp_gained( effect_on_condition_EOC_SYLPH_SPELL_EXPERIENCE_INCREASER,
                          spell_ominous_wind ) > 0 );
    }
}
