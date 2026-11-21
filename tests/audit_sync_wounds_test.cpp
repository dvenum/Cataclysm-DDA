#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "activity_actor_definitions.h"
#include "activity_actor_eoc.h"
#include "activity_handlers.h"
#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "character.h"
#include "condition.h"
#include "construction.h"
#include "coordinates.h"
#include "creature.h"
#include "damage.h"
#include "debug.h"
#include "dialogue.h"
#include "effect_on_condition.h"
#include "enum_conversions.h"
#include "faction.h"
#include "flag.h"
#include "flexbuffer_json.h"
#include "game.h"
#include "global_vars.h"
#include "item.h"
#include "json_error.h"
#include "json_loader.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "monfaction.h"
#include "monster.h"
#include "npc.h"
#include "options_helpers.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "point.h"
#include "talker.h"
#include "type_id.h"
#include "weighted_list.h"
#include "wound.h"

static const activity_id ACT_BUILD( "ACT_BUILD" );
static const activity_id ACT_MULTIPLE_DISSECT( "ACT_MULTIPLE_DISSECT" );

static const construction_str_id construction_test_constr_eoc_hooks( "test_constr_eoc_hooks" );

static const damage_type_id damage_bash( "bash" );
static const damage_type_id damage_cut( "cut" );

static const efftype_id effect_bandaged( "bandaged" );
static const efftype_id effect_narcosis( "narcosis" );

static const effect_on_condition_id
effect_on_condition_EOC_TEST_MON_USE_SPECIAL( "EOC_TEST_MON_USE_SPECIAL" );

static const faction_id faction_audit_sync_monfaction_copy( "audit_sync_monfaction_copy" );
static const faction_id faction_audit_sync_monfaction_owner( "audit_sync_monfaction_owner" );
static const faction_id faction_free_merchants( "free_merchants" );
static const faction_id faction_robofac( "robofac" );

static const itype_id itype_scalpel( "scalpel" );

static const json_character_flag json_flag_LIMB_LOWER( "LIMB_LOWER" );
static const json_character_flag json_flag_LIMB_UPPER( "LIMB_UPPER" );

static const limb_score_id limb_score_lift( "lift" );

static const mfaction_str_id
monfaction_audit_sync_owned_monfaction( "audit_sync_owned_monfaction" );
static const mfaction_str_id monfaction_human( "human" );
static const mfaction_str_id monfaction_robofac( "robofac" );

static const mtype_id mon_dog( "mon_dog" );

static const ter_str_id ter_t_dirt( "t_dirt" );

static const trait_id trait_AUDIT_SYNC_DISTRIBUTED( "AUDIT_SYNC_DISTRIBUTED" );
static const trait_id trait_AUDIT_SYNC_NON_FLESH( "AUDIT_SYNC_NON_FLESH" );
static const trait_id trait_AUDIT_SYNC_PAIN_NUMB( "AUDIT_SYNC_PAIN_NUMB" );
static const trait_id trait_NOPAIN( "NOPAIN" );
static const trait_id trait_PAINRESIST( "PAINRESIST" );
static const trait_id trait_REGEN_LIZ( "REGEN_LIZ" );

static const wound_fix_id wound_fix_audit_sync_fix( "audit_sync_fix" );

static const wound_type_id wound_audit_sync_fix_result( "audit_sync_fix_result" );
static const wound_type_id wound_audit_sync_fix_target( "audit_sync_fix_target" );
static const wound_type_id wound_audit_sync_lift_mod_wound( "audit_sync_lift_mod_wound" );
static const wound_type_id wound_audit_sync_lift_wound( "audit_sync_lift_wound" );
static const wound_type_id wound_audit_sync_pain_wound( "audit_sync_pain_wound" );
static const wound_type_id wound_bleeding_cut_bad( "bleeding_cut_bad" );
static const wound_type_id wound_bleeding_cut_medium( "bleeding_cut_medium" );
static const wound_type_id wound_bone_broken_mend_all( "bone_broken_mend_all" );

static conditional_t::func load_condition( const std::string &json )
{
    const JsonObject jo = json_loader::from_string( json );
    conditional_t::func condition;
    read_condition( jo, "condition", condition, false );
    finalize_conditions();
    return condition;
}

static talk_effect_t load_effect( const std::string &json )
{
    const JsonObject jo = json_loader::from_string( json );
    jo.allow_omitted_members();
    talk_effect_t effect( jo, "effect", "audit_sync_wounds_test" );
    finalize_conditions();
    return effect;
}

static int total_hp_damage( const bodypart &bp )
{
    int total = 0;
    for( const wound &wd : bp.get_wounds() ) {
        total += wd.get_hp_damage();
    }
    return total;
}

static int raw_pain_of( const bodypart &bp, const wound_type_id &type )
{
    for( const wound &wd : bp.get_wounds() ) {
        if( wd.type == type ) {
            return wd.get_pain_raw();
        }
    }
    return -1;
}

// sync-89557: a hit's wounds hold back the HP they cost. With DISTRIBUTED_DAMAGE the struck
// part loses only its share of the hit, and a part with less HP than the hit loses only what
// it had, but the wound held back the whole hit.
TEST_CASE( "sync-89557_wounds_hold_back_only_the_hp_the_part_lost", "[wounds][health][audit]" )
{
    clear_map();
    clear_avatar();
    avatar &u = get_avatar();
    on_out_of_scope restore( []() {
        clear_avatar();
    } );
    const bodypart_id arm = body_part_arm_l;
    bodypart &part = *u.get_part( arm );
    part.get_wounds().clear();

    const auto hit_arm = [&]( const int damage ) {
        const int before = u.get_part_hp_cur( arm );
        u.deal_damage( nullptr, arm, damage_instance( damage_cut, damage ) );
        return before - u.get_part_hp_cur( arm );
    };

    SECTION( "an ordinary hit holds back all of it" ) {
        const int lost = hit_arm( 25 );
        REQUIRE( lost > 0 );
        REQUIRE( part.has_wounds() );
        CHECK( total_hp_damage( part ) == lost );
    }
    SECTION( "a spread hit holds back only the struck part's share" ) {
        u.set_mutation( trait_AUDIT_SYNC_DISTRIBUTED );
        const int lost = hit_arm( 25 );
        REQUIRE( lost > 0 );
        REQUIRE( lost < 25 );
        REQUIRE( part.has_wounds() );
        CHECK( total_hp_damage( part ) == lost );
    }
    SECTION( "a hit bigger than the part's HP holds back what the part had" ) {
        u.set_part_hp_cur( arm, 10 );
        const int lost = hit_arm( 25 );
        REQUIRE( lost == 10 );
        REQUIRE( part.has_wounds() );
        CHECK( total_hp_damage( part ) == 10 );
    }
}

// sync-89557: with WOUND_CHANCE strictly between 0 and 1, the potential wounds were rolled a
// second time and the second, possibly empty, list's pick() was dereferenced.
TEST_CASE( "sync-89557_partial_wound_chance_never_picks_from_an_empty_list",
           "[wounds][health][audit]" )
{
    clear_avatar();
    avatar &u = get_avatar();
    override_option chance( "WOUND_CHANCE", "0.5" );
    bodypart &part = *u.get_part( body_part_arm_l );
    int wounded = 0;
    for( int i = 0; i < 200; i++ ) {
        part.get_wounds().clear();
        dealt_damage_instance hit;
        hit.set_damage( damage_cut, 5 );
        hit.bp_hit = body_part_arm_l;
        u.apply_random_wound_from_damage( body_part_arm_l, hit, 5 );
        if( part.has_wounds() ) {
            wounded++;
        }
    }
    CHECK( wounded > 0 );
    CHECK( wounded < 200 );
    part.get_wounds().clear();
    clear_avatar();
}

// sync-89557: above the cap the wounds leave, medicine healing went negative and took HP away
// (and bandage and disinfectant progress back).
TEST_CASE( "sync-89557_medicine_never_heals_a_negative_amount", "[wounds][health][audit]" )
{
    clear_avatar();
    avatar &u = get_avatar();
    const bodypart_id arm = body_part_arm_l;
    bodypart &part = *u.get_part( arm );
    part.get_wounds().clear();
    part.add_wound( wound_audit_sync_fix_target, u, 30 );
    u.add_effect( effect_bandaged, 6_hours, arm, false, 3 );
    const int full = u.get_part_hp_max( arm );
    REQUIRE( u.get_part_hp_cur( arm ) == full );
    REQUIRE( full - total_hp_damage( part ) < full );

    u.regen( 1 );
    CHECK( u.get_part_hp_cur( arm ) == full );
    CHECK( u.get_part_damage_bandaged( arm ) >= 0 );
    part.get_wounds().clear();
    clear_avatar();
}

// sync-89557: treating several wounds at once gave every new wound the HP the first one
// held back.
TEST_CASE( "sync-89557_each_treated_wound_keeps_its_own_hp_damage", "[wounds][health][audit]" )
{
    clear_map();
    clear_avatar();
    npc &guy = spawn_npc( ( get_avatar().pos_bub() + tripoint::east ).xy(), "test_talker" );
    clear_character( guy, true );
    on_out_of_scope restore( []() {
        clear_npcs();
        clear_avatar();
    } );
    bodypart &part = *guy.get_part( body_part_arm_l );
    part.get_wounds().clear();
    part.add_wound( wound_audit_sync_fix_target, guy, 10 );
    part.add_wound( wound_audit_sync_fix_target, guy, 30 );

    player_activity act( fix_wound_activity_actor( 1_seconds, body_part_arm_l,
                         wound_audit_sync_fix_target, wound_fix_audit_sync_fix, 2 ) );
    act.actor->finish( act, guy );

    std::vector<int> held_back;
    for( const wound &wd : part.get_wounds() ) {
        CHECK( wd.type == wound_audit_sync_fix_result );
        held_back.push_back( wd.get_hp_damage() );
    }
    std::sort( held_back.begin(), held_back.end() );
    CHECK( held_back == std::vector<int> { 10, 30 } );
}

// sync-89557: u_add_wound read wound_id as optional (an empty id), and with may_worsen and no
// damage_type it built a hit of damage type "".
TEST_CASE( "sync-89557_add_wound_needs_its_ids", "[wounds][eoc][audit]" )
{
    clear_avatar();
    avatar &u = get_avatar();
    bodypart &part = *u.get_part( body_part_arm_l );
    part.get_wounds().clear();

    CHECK_THROWS_AS( load_effect( R"({ "effect": { "u_add_wound": "arm_l" } })" ), JsonError );
    CHECK_THROWS_AS( load_effect(
                         R"({ "effect": { "u_add_wound": "arm_l", "wound_id": "audit_sync_fix_target", "may_worsen": true } })" ),
                     JsonError );

    dialogue d( get_talker_for( u ), nullptr );
    load_effect( R"({ "effect": { "u_add_wound": "arm_l", "wound_id": "audit_sync_fix_target",
                   "may_worsen": true, "damage_type": "cut", "damage_amount": 7 } })" ).apply( d );
    REQUIRE( part.get_wounds().size() == 1 );
    CHECK( part.get_wounds().front().get_hp_damage() == 7 );

    part.get_wounds().clear();
    d.set_value( "dt", std::string( "not_a_damage_type" ) );
    CHECK_FALSE( capture_debugmsg_during( [&d]() {
        load_effect( R"({ "effect": { "u_add_wound": "arm_l", "wound_id": "audit_sync_fix_target",
                       "may_worsen": true, "damage_type": { "context_val": "dt" } } })" ).apply(
                         d );
    } ).empty() );
    CHECK( part.get_wounds().empty() );
    clear_avatar();
}

// sync-89651: above 75% HP, the HP factor (up to 1.33) soaked up the wound penalties, so a
// healthy limb showed none. The factor now stops at 1.
TEST_CASE( "sync-89651_wound_penalties_apply_at_full_hp", "[wounds][health][audit]" )
{
    clear_avatar();
    avatar &u = get_avatar();
    const bodypart_id arm = body_part_arm_l;
    bodypart &part = *u.get_part( arm );
    part.get_wounds().clear();
    const float base = arm->get_limb_score( limb_score_lift );
    REQUIRE( base == Approx( 0.5f ) );
    const auto lift = [&]() {
        return part.get_limb_score( u, limb_score_lift, -1, 0, 1 );
    };
    CHECK( lift() == Approx( base ) );

    SECTION( "a flat penalty" ) {
        part.add_wound( wound_audit_sync_lift_wound, u, 0 );
        CHECK( lift() == Approx( base - 0.1f ) );
        u.set_part_hp_cur( arm, u.get_part_hp_max( arm ) / 2 );
        const float percent = static_cast<float>( u.get_part_hp_cur( arm ) ) /
                              u.get_part_hp_max( arm );
        CHECK( lift() == Approx( base * percent / 0.75f - 0.1f ) );
    }
    SECTION( "a mod" ) {
        part.add_wound( wound_audit_sync_lift_mod_wound, u, 0 );
        CHECK( lift() == Approx( base * 0.5f ) );
    }
    part.get_wounds().clear();
    clear_avatar();
}

// sync-89453: wound pain was multiplied by the raw PAIN enchantment multiplier, which is 0
// without one, so wounds had no pain; Pain Resistant (-0.33) made it negative.
TEST_CASE( "sync-89453_wounds_have_pain", "[wounds][health][audit]" )
{
    clear_avatar();
    avatar &u = get_avatar();
    bodypart &part = *u.get_part( body_part_arm_l );
    part.get_wounds().clear();

    SECTION( "an ordinary character" ) {
        part.add_wound( wound_audit_sync_pain_wound, u, 0 );
        CHECK( raw_pain_of( part, wound_audit_sync_pain_wound ) == 10 );
        CHECK( u.get_pain() >= 10 );
    }
    SECTION( "Pain Resistant lowers it, but not below 0" ) {
        u.set_mutation( trait_PAINRESIST );
        u.recalculate_enchantment_cache();
        part.add_wound( wound_audit_sync_pain_wound, u, 0 );
        const int pain = raw_pain_of( part, wound_audit_sync_pain_wound );
        CHECK( pain > 0 );
        CHECK( pain < 10 );
    }
    SECTION( "a multiplier below -1 leaves no pain, not a negative one" ) {
        u.set_mutation( trait_AUDIT_SYNC_PAIN_NUMB );
        u.recalculate_enchantment_cache();
        part.add_wound( wound_audit_sync_pain_wound, u, 0 );
        CHECK( raw_pain_of( part, wound_audit_sync_pain_wound ) == 0 );
        CHECK( u.get_pain() >= 0 );
    }
    part.get_wounds().clear();
    clear_avatar();
}

// sync-89453: Character::get_pain checks PAIN_IMMUNE and narcosis once instead of per wound;
// the result must be what the wounds report one by one.
TEST_CASE( "sync-89453_character_pain_matches_its_wounds", "[wounds][health][audit]" )
{
    clear_avatar();
    avatar &u = get_avatar();
    bodypart &arm = *u.get_part( body_part_arm_l );
    bodypart &leg = *u.get_part( body_part_leg_r );
    arm.add_wound( wound_audit_sync_pain_wound, u, 0 );
    leg.add_wound( wound_audit_sync_pain_wound, u, 0 );
    const auto wound_by_wound = [&u]() {
        int sum = 0;
        for( const bodypart_id &bp : u.get_all_body_parts() ) {
            for( const wound &wd : u.get_part( bp )->get_wounds() ) {
                sum += wd.get_pain( &u );
            }
        }
        return sum + u.Creature::get_pain();
    };

    CHECK( u.get_pain() == wound_by_wound() );
    CHECK( u.get_pain() >= 20 );
    SECTION( "under narcosis" ) {
        u.add_effect( effect_narcosis, 1_hours );
        CHECK( u.get_pain() == wound_by_wound() );
        CHECK( u.get_pain() == u.Creature::get_pain() );
    }
    SECTION( "immune to pain" ) {
        u.set_mutation( trait_NOPAIN );
        CHECK( u.get_pain() == wound_by_wound() );
        CHECK( u.get_pain() == u.Creature::get_pain() );
    }
    clear_avatar();
}

// sync-89386: u_is_bleeding was registered as taking an array, so { "u_is_bleeding": "MAJOR" }
// fell through to an always-false condition.
TEST_CASE( "sync-89386_is_bleeding_takes_a_severity", "[wounds][eoc][audit]" )
{
    clear_avatar();
    avatar &u = get_avatar();
    bodypart &part = *u.get_part( body_part_arm_l );
    part.get_wounds().clear();
    REQUIRE( wound_bleeding_cut_medium->severity == wound_severity::MAJOR );
    REQUIRE( wound_bleeding_cut_medium->is_bleeding_wound() );
    const dialogue d( get_talker_for( u ), nullptr );
    const conditional_t::func any = load_condition( R"({ "condition": "u_is_bleeding" })" );
    const conditional_t::func major = load_condition(
                                          R"({ "condition": { "u_is_bleeding": "MAJOR" } })" );
    const conditional_t::func critical = load_condition(
            R"({ "condition": { "u_is_bleeding": "CRITICAL" } })" );

    CHECK_FALSE( any( d ) );
    CHECK_FALSE( major( d ) );
    part.add_wound( wound_bleeding_cut_medium, u, 0 );
    CHECK( any( d ) );
    CHECK( major( d ) );
    CHECK_FALSE( critical( d ) );
    REQUIRE( wound_bleeding_cut_bad->severity == wound_severity::CRITICAL );
    part.add_wound( wound_bleeding_cut_bad, u, 0 );
    CHECK( critical( d ) );
    part.get_wounds().clear();
    clear_avatar();
}

// sync-89386: the severity was read on every check, so a typo was an error each time the
// sidebar was drawn. A literal one is checked as the condition loads.
TEST_CASE( "sync-89386_severity_typo_is_a_load_error", "[wounds][eoc][audit]" )
{
    CHECK_THROWS_AS( load_condition( R"({ "condition": { "u_is_bleeding": "MAJR" } })" ),
                     JsonError );
    CHECK_THROWS_AS( load_condition(
                         R"({ "condition": { "u_bodypart_is_bleeding": "arm_l", "severity": "CRITICL" } })" ),
                     JsonError );
    CHECK_NOTHROW( load_condition(
                       R"({ "condition": { "u_bodypart_is_bleeding": "arm_l", "severity": "CRITICAL" } })" ) );
}

// sync-89386/89439/89467 (EOC-B13): the new wound conditions gave no message for a talker
// that has no body, unlike the others.
TEST_CASE( "sync-89386_wound_conditions_with_the_wrong_talker_are_false", "[wounds][eoc][audit]" )
{
    clear_map();
    clear_avatar();
    avatar &u = get_avatar();
    monster &zed = spawn_test_monster( "mon_zombie", u.pos_bub() + point( 3, 0 ) );
    on_out_of_scope restore( []() {
        clear_creatures();
        clear_avatar();
    } );
    const auto false_with_message = []( const std::string & json, const_dialogue const & d ) {
        const conditional_t::func cond = load_condition( R"({ "condition": )" + json + " }" );
        bool result = true;
        const std::string messages = capture_debugmsg_during( [&]() {
            result = cond( d );
        } );
        CAPTURE( json, messages );
        return !result && !messages.empty();
    };
    const dialogue monster_first( get_talker_for( zed ), get_talker_for( u ) );
    CHECK( false_with_message( R"({ "u_is_bleeding": "MAJOR" })", monster_first ) );
    CHECK( false_with_message( R"({ "u_bodypart_is_wounded": "arm_l" })", monster_first ) );
    CHECK( false_with_message( R"({ "u_bodypart_is_wounded_and_fixable": "arm_l" })",
                               monster_first ) );
}

// sync-89549: Character::is_bleeding (fixed for HLTH-B09) and the new has_bleeding_wound
// answered the same question twice; is_bleeding now asks the body part.
TEST_CASE( "sync-89549_is_bleeding_asks_the_body_part", "[wounds][health][audit]" )
{
    clear_avatar();
    avatar &u = get_avatar();
    bodypart &arm = *u.get_part( body_part_arm_l );
    arm.get_wounds().clear();
    arm.add_wound( wound_audit_sync_pain_wound, u, 0 );
    CHECK_FALSE( u.is_bleeding( body_part_arm_l ) );
    arm.add_wound( wound_bleeding_cut_medium, u, 0 );
    CHECK( u.is_bleeding( body_part_arm_l ) );
    CHECK( arm.has_bleeding_wound() );
    CHECK_FALSE( u.is_bleeding( body_part_arm_r ) );
    arm.get_wounds().clear();
    clear_avatar();
}

// sync-89489: the lower vine limbs copied the upper one's flags, so they were LIMB_UPPER
// (small monsters couldn't reach them) instead of LIMB_LOWER.
TEST_CASE( "sync-89489_lower_vine_limbs_are_lower_limbs", "[wounds][audit]" )
{
    for( const char *side : {
             "r", "l"
         } ) {
        const bodypart_str_id upper( std::string( "vine_limb_upper_" ) + side );
        const bodypart_str_id lower( std::string( "vine_limb_lower_" ) + side );
        CAPTURE( side );
        CHECK( upper->has_flag( json_flag_LIMB_UPPER ) );
        CHECK_FALSE( upper->has_flag( json_flag_LIMB_LOWER ) );
        CHECK( lower->has_flag( json_flag_LIMB_LOWER ) );
        CHECK_FALSE( lower->has_flag( json_flag_LIMB_UPPER ) );
    }
}

// sync-89489: wound descriptions aren't formatted, so the bark and vine wounds showed a
// literal "%s".  sync-89351: the stab wounds listed "block" twice, so it counted double.
TEST_CASE( "sync-89489_wound_data_is_consistent", "[wounds][audit]" )
{
    for( const wound_type &wd : wound_type::get_all() ) {
        CAPTURE( wd.id.str() );
        CHECK( wd.get_description().find( '%' ) == std::string::npos );
        std::set<limb_score_id> scores;
        for( const wound_limb_score &s : wd.get_limb_scores() ) {
            CAPTURE( s.score.str() );
            CHECK( scores.insert( s.score ).second );
        }
    }
}

// sync-89489: the mend-all bone wounds blacklisted the character flag NON_FLESH_LIMB, which
// any non-flesh limb gives, so a MEND_ALL character with one got no broken bones at all.
TEST_CASE( "sync-89489_mend_all_bones_break_beside_a_non_flesh_limb", "[wounds][audit]" )
{
    clear_avatar();
    avatar &u = get_avatar();
    u.set_mutation( trait_REGEN_LIZ );
    u.set_mutation( trait_AUDIT_SYNC_NON_FLESH );
    const bodypart_id arm = body_part_arm_l;
    dealt_damage_instance hit;
    hit.set_damage( damage_bash, 70 );
    hit.bp_hit = arm;
    bool found = false;
    for( const auto &[pt_wd, weight] : arm->potential_wounds ) {
        if( pt_wd.id == wound_bone_broken_mend_all ) {
            found = true;
            CHECK( pt_wd.id->can_be_applied( &u, pt_wd, arm, hit ) );
        }
    }
    CHECK( found );
    clear_avatar();
}

// sync-89139: the reason for a corpse to dissect had no text, and enum_to_string's default
// is cata_fatal: a follower on the dissection job without a fine cutting tool crashed.
TEST_CASE( "sync-89139_dissecting_reason_has_a_text", "[activity][audit]" )
{
    CHECK( io::enum_to_string( do_activity_reason::NEEDS_DISSECTING ) ==
           "There is a corpse here to dissect." );
}

// sync-89139 (ACT-J02): u_assign_activity and NPC goals couldn't start the new dissection job.
TEST_CASE( "sync-89139_dissection_job_starts_from_eocs", "[activity][audit]" )
{
    CHECK( activity_actors::can_start_from_eoc( ACT_MULTIPLE_DISSECT ) );
}

// sync-89139: the dissection job took quartered and badly field dressed corpses, which
// set_up_butchery refuses, so it went back to the same corpse forever.
TEST_CASE( "sync-89139_dissection_job_leaves_corpses_it_cannot_dissect", "[activity][audit]" )
{
    clear_map();
    clear_avatar();
    avatar &u = get_avatar();
    u.i_add( item( itype_scalpel ) );
    map &here = get_map();
    multi_dissection_activity_actor job;

    const tripoint_bub_ms whole = u.pos_bub() + tripoint::east;
    here.add_item( whole, item::make_corpse( mon_dog ) );
    CHECK( job.multi_activity_can_do( u, whole ).reason == do_activity_reason::NEEDS_DISSECTING );
    for( const flag_id &spoiled : {
             flag_QUARTERED, flag_FIELD_DRESS_FAILED
         } ) {
        CAPTURE( spoiled.str() );
        const tripoint_bub_ms spot = u.pos_bub() + ( spoiled == flag_QUARTERED ? tripoint::west :
                                     tripoint::north );
        item corpse = item::make_corpse( mon_dog );
        corpse.set_flag( spoiled );
        here.add_item( spot, corpse );
        CHECK( job.multi_activity_can_do( u, spot ).reason == do_activity_reason::NO_COMPONENTS );
    }
    clear_avatar();
}

// sync-89385: when another builder finished the construction, the activity ended with no
// end hooks at all. The construction is done, so its completion hooks run. A construction
// finished by the builder itself ran none either.
// EOC_character_finished_activity_event_test copies the event into key1 and key2.
TEST_CASE( "sync-89385_finished_construction_runs_completion_hooks", "[activity][audit]" )
{
    clear_map();
    clear_avatar();
    avatar &u = get_avatar();
    map &here = get_map();
    global_variables &globvars = get_globals();
    const tripoint_bub_ms site = u.pos_bub() + tripoint::east;
    REQUIRE( here.ter( site ) != ter_t_dirt );
    partial_con pc;
    pc.id = construction_test_constr_eoc_hooks.id();
    here.partial_con_set( site, pc );
    u.assign_activity( build_construction_activity_actor( here.get_abs( site ) ) );
    REQUIRE( u.activity.id() == ACT_BUILD );
    globvars.clear_global_values();

    SECTION( "another builder finished it" ) {
        here.partial_con_remove( site );
        here.ter_set( site, ter_t_dirt );
        u.set_moves( 100 );
        u.activity.do_turn( u );
        CHECK( !u.activity );
    }
    SECTION( "the builder finished it" ) {
        REQUIRE( process_activity_bounded( u, 1000, 2000 ) );
        CHECK( here.ter( site ) == ter_t_dirt );
    }
    CHECK( globvars.get_global_value( "key1" ) == "ACT_BUILD" );
    CHECK( globvars.get_global_value( "key2" ) == 0 );
    clear_avatar();
}

// sync-88359: an NPC faction without a mon_faction left its NPCs with the empty monster
// faction instead of "human", since the empty id counts as valid.
TEST_CASE( "sync-88359_npcs_without_a_monster_faction_are_human", "[npc][audit]" )
{
    clear_map();
    clear_avatar();
    npc &guy = spawn_npc( ( get_avatar().pos_bub() + tripoint::east ).xy(), "test_talker" );
    on_out_of_scope restore( []() {
        clear_npcs();
        clear_avatar();
    } );
    guy.set_fac( faction_free_merchants );
    REQUIRE( guy.get_faction()->mon_faction.is_null() );
    CHECK( guy.get_monster_faction() == monfaction_human.id() );
    guy.set_fac( faction_robofac );
    REQUIRE( guy.get_faction()->mon_faction == monfaction_robofac );
    CHECK( guy.get_monster_faction() == monfaction_robofac.id() );
}

// sync-88359: a faction made with copy-from inherits its parent's mon_faction, and the new
// ownership pass logged that the monster faction had two owners. The parent owns it.
TEST_CASE( "sync-88359_faction_copy_shares_but_does_not_own_its_monfaction", "[npc][audit]" )
{
    const faction *owner = g->faction_manager_ptr->get( faction_audit_sync_monfaction_owner );
    const faction *copy = g->faction_manager_ptr->get( faction_audit_sync_monfaction_copy );
    REQUIRE( owner != nullptr );
    REQUIRE( copy != nullptr );
    CHECK( copy->mon_faction == monfaction_audit_sync_owned_monfaction );
    CHECK( copy->mon_faction_inherited );
    CHECK_FALSE( owner->mon_faction_inherited );
    CHECK( monfaction_audit_sync_owned_monfaction->associated_faction ==
           faction_audit_sync_monfaction_owner );
}

// sync-88359 (MONAI-J01): attack_target() now needs a HOSTILE target, so an EOC could no
// longer make a monster use a special attack on a creature it is only neutral to.
TEST_CASE( "sync-88359_eoc_special_attacks_reach_neutral_targets", "[monster][eoc][audit]" )
{
    clear_map_without_vision();
    clear_creatures();
    clear_avatar();
    get_globals().clear_global_values();
    map &here = get_map();
    avatar &u = get_avatar();
    const tripoint_bub_ms mon_pos( 65, 65, 0 );
    u.setpos( here, mon_pos + tripoint_rel_ms::east );
    monster &mon = spawn_test_monster( "mon_test_eoc_attacker", mon_pos );
    on_out_of_scope restore( []() {
        clear_creatures();
        clear_avatar();
    } );
    mon.set_moves( 100 );
    mon.aggro_character = false;
    REQUIRE( mon.attitude_to( u ) == Creature::Attitude::NEUTRAL );

    // Left to itself, a monster still attacks only what it is hostile to
    mon.set_dest( u.pos_abs() );
    CHECK( mon.attack_target() == nullptr );

    dialogue d( get_talker_for( mon ), get_talker_for( u ) );
    effect_on_condition_EOC_TEST_MON_USE_SPECIAL->activate( d );
    const diag_value *hit = get_globals().maybe_get_global_value( "test_mon_special_hit_avatar" );
    REQUIRE( hit != nullptr );
    CHECK( hit->dbl() == 1 );
    CHECK_FALSE( mon.eoc_attack );
}
