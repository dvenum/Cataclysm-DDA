#include <algorithm>
#include <string>
#include <utility>

#include "activity_tracker.h"
#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "character_attire.h"
#include "character_martial_arts.h"
#include "creature.h"
#include "damage.h"
#include "display.h"
#include "flexbuffer_json.h"
#include "game_constants.h"
#include "grapple.h"
#include "item.h"
#include "json_loader.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "mattack_common.h"
#include "math_parser_diag_value.h"
#include "melee.h"
#include "messages.h"
#include "mod_setting.h"
#include "monattack.h"
#include "monster.h"
#include "mtype.h"
#include "mutation.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"
#include "widget.h"

// Regression tests for the combat part of the 2026-11-21 upstream sync
// (Audit/upstream/sync-2026-11-21/B_combat.md).

static const damage_type_id damage_bash( "bash" );

static const efftype_id effect_downed( "downed" );
static const efftype_id effect_grabbed( "grabbed" );
static const efftype_id effect_incorporeal( "incorporeal" );
static const efftype_id effect_test_no_grab( "test_no_grab" );

static const flag_id json_flag_GRAB( "GRAB" );

static const itype_id itype_test_armguard( "test_armguard" );

static const matype_id test_style_cmbt_j01( "test_style_cmbt_j01" );
static const matype_id test_style_sync_dodge_count( "test_style_sync_dodge_count" );

static const skill_id skill_dodge( "dodge" );
static const skill_id skill_melee( "melee" );
static const skill_id skill_unarmed( "unarmed" );

static const widget_id widget_activity_desc( "activity_desc" );
static const widget_id widget_zenfs_activity_desc( "zenfs_activity_desc" );

static const wound_type_id wound_bruise_small( "bruise_small" );

namespace
{
// Clears the creatures and the avatar before a test and after it
struct sync_scene {
    sync_scene() {
        reset();
    }
    ~sync_scene() {
        reset();
    }
    static void reset() {
        clear_map_without_vision();
        clear_creatures();
        clear_npcs();
        clear_avatar();
    }
};
} // namespace

// A strong unarmed blocker: style_none blocks with an arm from unarmed 1, and a worn
// BLOCK_WHILE_WORN arm guard adds to it.
static item &make_blocker( Character &you )
{
    you.set_str_base( 20 );
    you.set_skill_level( skill_unarmed, 10 );
    you.set_skill_level( skill_melee, 10 );
    you.set_stamina( you.get_stamina_max() );
    REQUIRE( you.wear_item( item( itype_test_armguard ), false ).has_value() );
    item *guard = you.worn.best_shield();
    REQUIRE( guard != nullptr );
    REQUIRE( guard->typeId() == itype_test_armguard );
    return *guard;
}

static double num_var( const Creature &c, const std::string &key )
{
    const diag_value *val = c.maybe_get_value( key );
    return val == nullptr ? 0.0 : val->dbl();
}

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

// #89640 moved blocking into a block roll and dropped the wear on a worn shield that takes the
// blow, which the old block_hit dealt: worn shields no longer wore out from blocking.
// The wear goes by the blow before the block cut it down (sync review): by the damage left after
// it, a strong block left too little to wear a sturdy guard, and a perfect one none at all.
TEST_CASE( "sync-89640_worn_shield_wears_when_it_blocks", "[melee][sync]" )
{
    sync_scene scene;
    avatar &you = get_avatar();
    item &guard = make_blocker( you );
    monster &zed = spawn_test_monster( "mon_zombie", you.pos_bub() + tripoint::east );
    // A STURDY item takes no wear from a blow below this
    const float sturdy = std::max( 10.0f, guard.resist( damage_bash, true, body_part_arm_l.id() ) *
                                   1.3333f );
    float blow = 200.0f;
    bool strong = false;

    SECTION( "a fair block" ) {
    }
    SECTION( "a strong block" ) {
        you.set_str_base( 40 );
        blow = 3.0f * sturdy;
        strong = true;
    }
    int blocks = 0;
    for( int i = 0; i < 100 && guard.damage() == 0; ++i ) {
        you.blocks_left = 1;
        you.set_stamina( you.get_stamina_max() );
        bodypart_id bp = body_part_arm_l;
        damage_instance dam( damage_bash, blow );
        if( melee::roll_and_block_hit( you, &zed, bp, dam ) ) {
            ++blocks;
        }
        if( strong ) {
            // What gets past the block couldn't wear the guard; the blow can
            REQUIRE( Creature::melee_damage_mult( you.last_hit_spread ) * blow < sturdy );
        }
    }
    REQUIRE( blocks > 0 );
    CHECK( guard.damage() > 0 );
}

// The hardcoded monster attacks (acid_barf, fungus_inject, fungus_bristle, fungus_fortify,
// flesh_golem, lunge) call block_hit after dodge_check, with no hit roll. #89640's block_hit
// reads the block rolled by the last deal_melee_attack, so they blocked on another attack's
// numbers; they now roll a fresh block.
TEST_CASE( "sync-89640_hardcoded_monster_attacks_roll_a_fresh_block", "[melee][sync]" )
{
    sync_scene scene;
    set_time_to_day();
    avatar &you = get_avatar();

    SECTION( "a blocker blocks a lunge" ) {
        make_blocker( you );
        monster &zed = spawn_test_monster( "mon_zombie", you.pos_bub() + tripoint::east );
        zed.set_dest( you.pos_abs() );
        REQUIRE( zed.attack_target() == &you );
        int blocks = 0;
        for( int i = 0; i < 20; ++i ) {
            you.blocks_left = 1;
            you.set_dodges_left( 0 );
            you.set_stamina( you.get_stamina_max() );
            you.set_all_parts_hp_to_max();
            you.remove_effect( effect_downed );
            zed.set_moves( 100 );
            REQUIRE( mattack::lunge( &zed ) );
            if( you.blocks_left == 0 ) {
                ++blocks;
            }
        }
        CHECK( blocks > 0 );
    }
    SECTION( "the last exchange's block doesn't carry over" ) {
        // Unarmed 0, no shield: no block at all, so a lunge lands in full (3 to 7 bash), whatever
        // a block in an earlier exchange left behind
        monster &zed = spawn_test_monster( "mon_zombie", you.pos_bub() + tripoint::east );
        zed.set_dest( you.pos_abs() );
        REQUIRE( zed.attack_target() == &you );
        for( int i = 0; i < 10; ++i ) {
            you.blocks_left = 1;
            you.set_dodges_left( 0 );
            you.set_all_parts_hp_to_max();
            you.remove_effect( effect_downed );
            you.last_dodge = 0.0f;
            you.last_block = 40.0f;
            you.last_hit_spread = -10;
            zed.set_moves( 100 );
            REQUIRE( mattack::lunge( &zed ) );
            CHECK( you.blocks_left == 1 );
            int hp_lost = 0;
            for( const bodypart_id &part : you.get_all_body_parts() ) {
                hp_lost += you.get_part_hp_max( part ) - you.get_part_hp_cur( part );
            }
            CHECK( hp_lost >= 3 );
            CHECK( hp_lost <= 7 );
        }
    }
}

// #89278's skeletal hulk (and #89642's Gozu, and older spiders, fusion zombies and mod monsters)
// give their own copy of "grab" a condition, which replaces the base attack's. The NO_GRAB and
// incorporeal guards lived only in that condition, so those grabs lost them.
TEST_CASE( "sync-89278_a_grab_override_keeps_the_no_grab_and_incorporeal_guards",
           "[grapple][mattack][grab][sync]" )
{
    sync_scene scene;
    avatar &you = get_avatar();
    you.set_dodges_left( 0 );
    monster &grabber = spawn_test_monster( "mon_test_sync_override_grabber",
                                           you.pos_bub() + tripoint::east );
    grabber.set_dest( you.pos_abs() );
    REQUIRE( grabber.attack_target() == &you );
    const mtype_special_attack &grab = grabber.type->special_attacks.at( "grab" );

    SECTION( "an ordinary target is grabbed" ) {
        CHECK( grab->call( grabber ) );
        CHECK( you.has_effect_with_flag( json_flag_GRAB ) );
    }
    SECTION( "an incorporeal target isn't" ) {
        you.add_effect( effect_incorporeal, 1_hours );
        CHECK_FALSE( grab->call( grabber ) );
        CHECK_FALSE( you.has_effect_with_flag( json_flag_GRAB ) );
    }
    SECTION( "a NO_GRAB grabber doesn't grab" ) {
        grabber.add_effect( effect_test_no_grab, 1_hours );
        CHECK_FALSE( grab->call( grabber ) );
        CHECK_FALSE( you.has_effect_with_flag( json_flag_GRAB ) );
    }
}

// #89640 subtracts the target's block roll from every hit roll. A grab is only dodged, as the
// grappling port has it: neither a monster's grab nor a character's rolls a block, so a block
// can't stop one without being spent.
TEST_CASE( "sync-89640_grabs_roll_no_block", "[grapple][grab][sync]" )
{
    sync_scene scene;
    avatar &you = get_avatar();

    SECTION( "a monster's grab" ) {
        make_blocker( you );
        you.set_dodges_left( 0 );
        monster &grabber = spawn_test_monster( "mon_test_counter_grabber",
                                               you.pos_bub() + tripoint::east );
        grabber.set_dest( you.pos_abs() );
        REQUIRE( grabber.attack_target() == &you );
        you.blocks_left = 1;
        REQUIRE( you.block_roll( &grabber ) > 0.0f );
        you.last_block = 99.0f;
        CHECK( grabber.type->special_attacks.at( "grab" )->call( grabber ) );
        CHECK( you.last_block == 0.0f );
        CHECK( you.blocks_left == 1 );
        CHECK( you.has_effect_with_flag( json_flag_GRAB ) );
    }
    SECTION( "a character's grab" ) {
        monster &blocker = spawn_test_monster( "mon_test_sync_blocker",
                                               you.pos_bub() + tripoint::east );
        blocker.blocks_left = 1;
        REQUIRE( blocker.block_roll( &you ) > 0.0f );
        blocker.last_block = 99.0f;
        you.set_stamina( you.get_stamina_max() );
        grapple::attempt_grab( you, blocker );
        CHECK( blocker.last_block == 0.0f );
        CHECK( blocker.blocks_left == 1 );
    }
}

// #89640's monster block roll had no check for a held monster, which get_dodge() has: a held
// feral or mi-go still blocked.
TEST_CASE( "sync-89640_a_held_monster_cannot_block", "[melee][grapple][sync]" )
{
    sync_scene scene;
    avatar &you = get_avatar();
    monster &blocker = spawn_test_monster( "mon_test_sync_blocker",
                                           you.pos_bub() + tripoint::east );
    blocker.blocks_left = 1;
    CHECK( blocker.block_roll( &you ) > 0.0f );
    blocker.add_effect( effect_grabbed, 1_hours );
    REQUIRE( blocker.has_effect_with_flag( json_flag_GRAB ) );
    blocker.blocks_left = 1;
    CHECK( blocker.block_roll( &you ) == 0.0f );
}

// #89470 lets a hit land down to a hit spread of -24, but on_dodge still ran for any spread up
// to 0: a defender could counter, and practice dodging, an attack that hit them.
TEST_CASE( "sync-89470_dodge_reactions_only_on_a_miss", "[melee][martial_arts][sync]" )
{
    sync_scene scene;
    avatar &you = get_avatar();
    you.martial_arts_data->add_martialart( test_style_cmbt_j01 );
    you.martial_arts_data->set_style( test_style_cmbt_j01, false );
    monster &attacker = spawn_test_monster( "mon_test_non_shearable",
                                            you.pos_bub() + tripoint::east );
    you.set_dodges_left( 5 );
    const float dodge = you.dodge_roll();
    REQUIRE( dodge > 0.0f );

    SECTION( "a glancing hit" ) {
        const int spread = you.deal_melee_attack( &attacker, static_cast<int>( dodge ) - 10, false );
        REQUIRE( Creature::melee_attack_hit( spread ) );
        REQUIRE( spread < 0 );
        CHECK( num_var( attacker, "cmbt_j01_dodged" ) == 0 );
    }
    SECTION( "a miss" ) {
        const int spread = you.deal_melee_attack( &attacker, static_cast<int>( dodge ) - 40, false );
        REQUIRE_FALSE( Creature::melee_attack_hit( spread ) );
        CHECK( num_var( attacker, "cmbt_j01_dodged" ) == 1 );
    }
    // Sync review: a grab or disarm misses at any negative hit spread, and that is a dodge
    SECTION( "an all-or-nothing attack missed by a little" ) {
        const int spread = you.deal_melee_attack( &attacker, static_cast<int>( dodge ) - 10, false,
                           melee_dodge_rule::all_or_nothing );
        REQUIRE( Creature::melee_attack_hit( spread ) );
        REQUIRE( spread < 0 );
        CHECK( num_var( attacker, "cmbt_j01_dodged" ) == 1 );
    }
    // Sync review: an undodgeable attack lands whatever the dodge roll, so it's never a dodge
    SECTION( "an undodgeable attack" ) {
        const int spread = you.deal_melee_attack( &attacker, static_cast<int>( dodge ) - 40, false,
                           melee_dodge_rule::undodgeable );
        REQUIRE_FALSE( Creature::melee_attack_hit( spread ) );
        CHECK( num_var( attacker, "cmbt_j01_dodged" ) == 0 );
    }
}

// Sync review: a monster's grab misses at any negative hit spread (it takes hold or it doesn't),
// but on_dodge ran only for a full miss, at -25 or less: a grab dodged by a little gave no dodge
// counter, ondodge EOC or practice. And an undodgeable attack ran on_dodge, then hit in full.
TEST_CASE( "sync-review_monster_attacks_count_dodges_by_their_own_miss", "[melee][grapple][sync]" )
{
    sync_scene scene;
    avatar &you = get_avatar();
    you.martial_arts_data->add_martialart( test_style_sync_dodge_count );
    you.martial_arts_data->set_style( test_style_sync_dodge_count, false );

    SECTION( "a grab" ) {
        // Its hit roll centers on the avatar's dodge roll: about a third of the grabs miss by
        // less than 25
        monster &grabber = spawn_test_monster( "mon_test_sync_clumsy_grabber",
                                               you.pos_bub() + tripoint::east );
        grabber.set_dest( you.pos_abs() );
        REQUIRE( grabber.attack_target() == &you );
        const mtype_special_attack &grab = grabber.type->special_attacks.at( "grab" );
        int misses = 0;
        int small_misses = 0;
        for( int i = 0; i < 60; ++i ) {
            you.clear_effects();
            grabber.clear_effects();
            you.set_dodges_left( 5 );
            you.set_stamina( you.get_stamina_max() );
            Messages::clear_messages();
            REQUIRE( grab->call( grabber ) );
            if( has_message_containing( "but you dodge" ) ) {
                ++misses;
                if( Creature::melee_attack_hit( you.last_hit_spread ) ) {
                    ++small_misses;
                }
            }
        }
        REQUIRE( small_misses > 0 );
        CHECK( num_var( you, "sync_dodges" ) == misses );
    }
    SECTION( "an undodgeable attack" ) {
        you.set_dex_base( 20 );
        you.set_skill_level( skill_dodge, 10 );
        monster &jabber = spawn_test_monster( "mon_test_sync_undodgeable_jabber",
                                              you.pos_bub() + tripoint::east );
        jabber.set_dest( you.pos_abs() );
        REQUIRE( jabber.attack_target() == &you );
        const mtype_special_attack &jab = jabber.type->special_attacks.at( "test_sync_undodgeable_jab" );
        int full_misses = 0;
        for( int i = 0; i < 10; ++i ) {
            you.set_dodges_left( 5 );
            you.set_all_parts_hp_to_max();
            REQUIRE( jab->call( jabber ) );
            if( !Creature::melee_attack_hit( you.last_hit_spread ) ) {
                ++full_misses;
            }
        }
        REQUIRE( full_misses > 0 );
        CHECK( num_var( you, "sync_dodges" ) == 0 );
    }
}

// #89469's activity widgets compared u_val('activity_level') with 500, but it is half the
// activity level: combat reads 250 (a throw scaled down from it less), so they showed nothing.
TEST_CASE( "sync-89469_activity_widgets_show_combat", "[widget][activity][sync]" )
{
    clear_avatar();
    avatar &ava = get_avatar();
    activity_tracker &tracker = ava.activity_history;
    widget activity_w = widget_activity_desc.obj();
    widget zenfs_w = widget_zenfs_activity_desc.obj();

    ava.reset_activity_level();
    tracker.new_turn();
    tracker.log_activity( EXPLOSIVE_EXERCISE );
    CHECK( activity_w.layout( ava ).find( "Explosive" ) != std::string::npos );
    CHECK( zenfs_w.layout( ava ).find( "Explosive" ) != std::string::npos );

    for( const float level : {
             COMBAT_EXERCISE, COMBAT_EXERCISE / 2
         } ) {
        CAPTURE( level );
        ava.reset_activity_level();
        tracker.new_turn();
        tracker.log_activity( level );
        CHECK( activity_w.layout( ava ).find( "Combat" ) != std::string::npos );
        CHECK( zenfs_w.layout( ava ).find( "Combat" ) != std::string::npos );
        CHECK( display::activity_level_str( level ) == "Combat" );
    }
    CHECK( display::activity_level_str( EXPLOSIVE_EXERCISE ) == "Explosive" );
    CHECK( display::activity_level_str( EXTRA_EXERCISE ) == "Extreme" );
    ava.reset_activity_level();
}

// #89557 caps medicine healing at the HP the limb's wounds hold back, but above that cap (after
// a bandage, say) the capped amount went negative, and the medicine took HP away.
TEST_CASE( "sync-89557_medicine_takes_no_hp_above_the_wound_cap", "[wounds][sync]" )
{
    clear_avatar();
    avatar &you = get_avatar();
    bodypart *arm = you.get_part( body_part_arm_l );
    arm->get_wounds().clear();
    you.set_part_hp_cur( body_part_arm_l, you.get_part_hp_max( body_part_arm_l ) );
    arm->add_wound( wound_bruise_small, you, 20 );
    const int hp = you.get_part_hp_cur( body_part_arm_l );
    you.regen( 1 );
    CHECK( you.get_part_hp_cur( body_part_arm_l ) >= hp );
    arm->get_wounds().clear();
}

static void load_mutation_attack( const std::string &attack )
{
    JsonObject jo = json_loader::from_string( R"({ "type": "mutation", "id": "test_sync_attack", )"
                    R"("name": "test", "description": "test", "points": 0, "attacks": )" + attack + " }" );
    jo.allow_omitted_members();
    mutation_branch mut;
    mut.load( jo, "audit_sync_combat_test" );
}

// #89444 removed the hardcoded tentacle attacks, the only users of a mutation attack's
// "hardcoded_effect". Such an attack then did no damage at all, silently (MUT-J16).
TEST_CASE( "sync-89444_hardcoded_mutation_attack_is_a_load_error", "[mutations][sync]" )
{
    CHECK_NOTHROW( load_mutation_attack( R"({ "attack_text_u": "You hit %s", )"
                                         R"("attack_text_npc": "%1$s hits %2$s", "chance": 10, )"
                                         R"("base_damage": { "damage_type": "bash", "amount": 1 } })" ) );
    // It used to load with no damage at all
    CHECK_THROWS_WITH( load_mutation_attack( R"({ "attack_text_u": "You hit %s", )"
                       R"("attack_text_npc": "%1$s hits %2$s", "chance": 10, )"
                       R"("hardcoded_effect": true })" ),
                       Catch::Matchers::Contains( "hardcoded mutation attacks are gone" ) );
}

// Sync review: a mod's WORLD_OPTION naming the old EXPLOSIVE_COMBAT_EXERTION bool failed to load
// ("there's no option"); it now reads as COMBAT_EXERTION, its value migrated as a save's is.
TEST_CASE( "sync-89469_world_option_reads_the_old_exertion_bool", "[options][mod_settings][sync]" )
{
    const auto read = []( const std::string & json ) {
        const JsonObject jo = json_loader::from_string( json );
        jo.allow_omitted_members();
        return read_world_option( jo );
    };
    using option = std::pair<std::string, std::string>;
    CHECK( read( R"({ "type": "WORLD_OPTION", "name": "EXPLOSIVE_COMBAT_EXERTION", "default": true })" )
           == option( "COMBAT_EXERTION", "explosive" ) );
    CHECK( read(
               R"({ "type": "WORLD_OPTION", "name": "EXPLOSIVE_COMBAT_EXERTION", "default": false })" )
           == option( "COMBAT_EXERTION", "normal" ) );
    CHECK( read( R"({ "type": "WORLD_OPTION", "name": "COMBAT_EXERTION", "default": "combat" })" )
           == option( "COMBAT_EXERTION", "combat" ) );
}
