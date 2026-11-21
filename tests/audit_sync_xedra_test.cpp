#include <algorithm>
#include <string>
#include <vector>

#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "character.h"
#include "creature.h"
#include "debug.h"
#include "dialogue.h"
#include "effect.h"
#include "effect_on_condition.h"
#include "magic.h"
#include "magic_enchantment.h"
#include "map.h"
#include "map_helpers.h"
#include "math_parser_diag_value.h"
#include "npc.h"
#include "options_helpers.h"
#include "pimpl.h"
#include "player_helpers.h"
#include "point.h"
#include "talker.h"
#include "type_id.h"
#include "wound.h"

static const effect_on_condition_id
effect_on_condition_EOC_PYROKIN_CAUTERIZE_SELECTOR( "EOC_PYROKIN_CAUTERIZE_SELECTOR" );
static const effect_on_condition_id
effect_on_condition_EOC_SALAMANDER_SPELL_EXPERIENCE_INCREASER( "EOC_SALAMANDER_SPELL_EXPERIENCE_INCREASER" );
static const effect_on_condition_id
effect_on_condition_EOC_SYLPH_SPELL_EXPERIENCE_INCREASER( "EOC_SYLPH_SPELL_EXPERIENCE_INCREASER" );

static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_effect_ewwww_de_zombie( "effect_ewwww_de_zombie" );

static const skill_id skill_gramarye( "gramarye" );

static const spell_id spell_create_light_salamander( "create_light_salamander" );
static const spell_id spell_ominous_wind( "ominous_wind" );

static const trait_id trait_CREATE_LIGHT( "CREATE_LIGHT" );
static const trait_id trait_OMINOUS_WIND( "OMINOUS_WIND" );
static const trait_id trait_PROF_MED( "PROF_MED" );
static const trait_id trait_SALAMANDER( "SALAMANDER" );
static const trait_id trait_SYLPH( "SYLPH" );
static const trait_id trait_TRAIT_EWWWW_DE_ZOMBIE( "TRAIT_EWWWW_DE_ZOMBIE" );

static const weather_type_id weather_rain( "rain" );

static const wound_type_id wound_bleeding_cut_bad( "bleeding_cut_bad" );
static const wound_type_id wound_bleeding_laceration_medium( "bleeding_laceration_medium" );
static const wound_type_id wound_laceration_medium( "laceration_medium" );

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

// #89431 changed Ewwww de Zombie's "not while bleeding" check from the bleed effect to bleeding
// wounds.  Monster attacks and techniques still cause the bleed effect, so either kind of bleeding
// has to end the disguise.  Needs the mod: run with --mods=bombastic_perks "[bombastic_perks]".
TEST_CASE( "sync-89431_ewww_de_zombie_fails_while_bleeding", "[.][bombastic_perks]" )
{
    clear_avatar();
    on_out_of_scope restore( []() {
        clear_avatar();
    } );
    avatar &u = get_avatar();
    const enchantment *disguise = nullptr;
    for( const enchantment_id &ench : effect_effect_ewwww_de_zombie->enchantments ) {
        const std::vector<trait_id> &muts = ench->get_mutations();
        if( std::find( muts.begin(), muts.end(), trait_TRAIT_EWWWW_DE_ZOMBIE ) != muts.end() ) {
            disguise = &ench.obj();
        }
    }
    REQUIRE( disguise != nullptr );
    REQUIRE( disguise->is_active( u, true ) );

    SECTION( "the bleed effect" ) {
        u.add_effect( effect_bleed, 10_minutes, body_part_torso );
        CHECK_FALSE( disguise->is_active( u, true ) );
    }
    SECTION( "a bleeding wound" ) {
        u.get_part( body_part_arm_l )->add_wound( wound_bleeding_cut_bad, u, 0 );
        CHECK_FALSE( disguise->is_active( u, true ) );
    }
}

// #89431 changed MindOverMatter's Cauterize to check bleeding wounds instead of the bleed effect,
// but a successful cauterization still only removed the effect: it said the bleeding stopped and
// the wound kept bleeding, and a part bleeding from the effect alone couldn't be cauterized at
// all.  It now stops both on the part picked.  An NPC picks the part at random; every part bleeds,
// so whichever it picks must stop.  Needs the mod: run with --mods=mindovermatter "[mindovermatter]".
TEST_CASE( "sync-89431_cauterize_stops_both_kinds_of_bleeding", "[.][mindovermatter]" )
{
    clear_map();
    clear_avatar();
    on_out_of_scope restore( []() {
        clear_map();
        clear_avatar();
    } );
    avatar &u = get_avatar();
    npc &guy = spawn_npc( u.pos_bub().xy() + point::east, "test_talker" );
    guy.set_mutation( trait_PROF_MED );
    const std::vector<bodypart_id> parts = guy.get_all_body_parts( get_body_part_flags::only_main );
    const auto picked_part = [&guy]() {
        const diag_value *picked = guy.maybe_get_value( "pyrokin_cauterize_picked_bodypart" );
        REQUIRE( picked != nullptr );
        return bodypart_id( picked->str() );
    };

    SECTION( "a bleeding wound" ) {
        for( const bodypart_id &bp : parts ) {
            guy.get_part( bp )->get_wounds().clear();
            guy.get_part( bp )->add_wound( wound_bleeding_laceration_medium, guy, 0 );
        }
        CHECK( runs_quietly( effect_on_condition_EOC_PYROKIN_CAUTERIZE_SELECTOR, guy ) );
        const bodypart_id bp = picked_part();
        CAPTURE( bp.id().str() );
        CHECK_FALSE( guy.has_bleeding_wound( bp.id() ) );
        CHECK( guy.has_wound( wound_laceration_medium, bp.id() ) );
    }
    SECTION( "the bleed effect" ) {
        for( const bodypart_id &bp : parts ) {
            guy.add_effect( effect_bleed, 10_minutes, bp );
        }
        CHECK( runs_quietly( effect_on_condition_EOC_PYROKIN_CAUTERIZE_SELECTOR, guy ) );
        const bodypart_id bp = picked_part();
        CAPTURE( bp.id().str() );
        CHECK_FALSE( guy.has_effect( effect_bleed, bp ) );
    }
}
