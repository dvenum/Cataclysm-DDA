#include <string>
#include <vector>

#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "creature.h"
#include "damage.h"
#include "incapacitation.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "martialarts.h"
#include "monster.h"
#include "player_helpers.h"
#include "point.h"
#include "subbodypart.h"
#include "type_id.h"

static const damage_type_id damage_bash( "bash" );

static const matec_id TECH_TENTACLE_GRAB_CRUSH( "TECH_TENTACLE_GRAB_CRUSH" );

// Guards Creature::deflect_hit_part: a knocked-out character can't dodge, block or shield a hit
// with a limb either (partial dodge #89470 against the master merge's knockouts).
TEST_CASE( "sync-89470_knocked_out_characters_cannot_shield_a_hit", "[melee][executions]" )
{
    clear_avatar();
    avatar &you = get_avatar();
    const bodypart_id torso = body_part_torso.id();
    int shielded = 0;
    for( int i = 0; i < 200; ++i ) {
        shielded += you.deflect_hit_part( torso, true ) != torso;
    }
    // Awake, some of the hits are shielded
    REQUIRE( shielded > 0 );

    REQUIRE( incapacitation::apply( you, nullptr, 10_minutes, false ) == "applied" );
    REQUIRE( incapacitation::unconscious( you ) );
    shielded = 0;
    for( int i = 0; i < 200; ++i ) {
        shielded += you.deflect_hit_part( torso, true ) != torso;
    }
    CHECK( shielded == 0 );
    clear_avatar();
}

// Guards Creature::deal_melee_hit: only a hit the target rolled a defense against
// (deal_melee_attack) can be shielded with a limb. A hit passed on from a mount to its rider, or
// one a script deals, lands where it is aimed, as it isn't blocked either.
TEST_CASE( "sync-89470_a_hit_with_no_defense_roll_lands_where_it_is_aimed", "[melee]" )
{
    clear_map();
    clear_avatar();
    avatar &you = get_avatar();
    monster &zed = spawn_test_monster( "mon_zombie", you.pos_bub() + tripoint_rel_ms::east );
    const bodypart_id torso = body_part_torso.id();
    const auto hit = [&]() {
        dealt_damage_instance dealt;
        you.deal_melee_hit( &zed, 1000, false, damage_instance(), dealt, {}, &torso );
        return dealt.bp_hit;
    };
    int shielded = 0;
    for( int i = 0; i < 200; ++i ) {
        you.clear_melee_defense();
        shielded += hit() != torso;
    }
    CHECK( shielded == 0 );

    // After a defense roll, a limb may take the hit
    shielded = 0;
    for( int i = 0; i < 200; ++i ) {
        you.deal_melee_attack( &zed, 1000 );
        shielded += hit() != torso;
    }
    CHECK( shielded > 0 );
    clear_avatar();
}

// Guards the tentacle sub body parts of #89444/#89450: each one's opposite is the matching part
// on the other tentacle, which names it back (a left part named a left "_r" part that doesn't
// exist, a load error).
TEST_CASE( "sync-89444_tentacle_sub_parts_name_their_opposites", "[bodypart][mutations]" )
{
    for( const std::string part : {
             "base", "middle", "tip", "dual_base", "dual_middle", "dual_tip"
         } ) {
        const sub_bodypart_str_id left( "tentacle_left_" + part + "_l" );
        const sub_bodypart_str_id right( "tentacle_right_" + part + "_r" );
        CAPTURE( part );
        REQUIRE( left.is_valid() );
        REQUIRE( right.is_valid() );
        CHECK( left->opposite == right );
        CHECK( right->opposite == left );
        CHECK( left->part_side == side::LEFT );
        CHECK( right->part_side == side::RIGHT );
    }
}

// Guards TECH_TENTACLE_GRAB_CRUSH (#89444): its strength bonus named "strength", which is no
// stat, so the bonus was a load error and added nothing.
TEST_CASE( "sync-89444_tentacle_crush_scales_with_strength", "[martial_arts][mutations]" )
{
    clear_avatar();
    avatar &you = get_avatar();
    REQUIRE( TECH_TENTACLE_GRAB_CRUSH.is_valid() );
    you.set_str_base( 8 );
    you.reset_stats();
    const float weak = TECH_TENTACLE_GRAB_CRUSH->damage_bonus( you, damage_bash );
    you.set_str_base( 20 );
    you.reset_stats();
    const float strong = TECH_TENTACLE_GRAB_CRUSH->damage_bonus( you, damage_bash );
    // 0.85 bash per point of strength
    CHECK( strong - weak == Approx( 12 * 0.85f ) );
    clear_avatar();
}
