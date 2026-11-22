#include <functional>
#include <string>

#include "avatar.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "coordinates.h"
#include "faction.h"
#include "item.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "monfaction.h"
#include "monster.h"
#include "npc.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"

static const faction_id faction_free_merchants( "free_merchants" );

static const itype_id itype_FMCNote( "FMCNote" );

static const mfaction_str_id monfaction_human( "human" );

static const trait_id trait_BEE( "BEE" );

// sync-88359: a faction now leaves mon_faction empty when its JSON names none (it was "human"),
// so that it doesn't own the human monster faction. npc::get_monster_faction then fell through
// to its checks for NPCs without a faction: an ally of the player counted as the player's
// monster faction and a BEE NPC as a bee. They were human before, and are again.
TEST_CASE( "sync-88359_faction_npcs_are_human_as_allies_and_bees", "[npc][audit]" )
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
    CHECK( monfaction_human->associated_faction.is_null() );

    SECTION( "an ally of the player" ) {
        guy.set_faction_ver( 1 );
        guy.set_attitude( NPCATT_FOLLOW );
        REQUIRE( guy.is_player_ally() );
        CHECK( guy.get_monster_faction() == monfaction_human.id() );
    }
    SECTION( "a bee" ) {
        guy.set_mutation( trait_BEE );
        REQUIRE( guy.has_trait( trait_BEE ) );
        CHECK( guy.get_monster_faction() == monfaction_human.id() );
    }
}

// sync-88359: a civilian (a human monster) that saw a theft told no NPC once their factions
// named no mon_faction, and turned on the thief itself. The NPCs of the human monster faction
// get after the thief again.
TEST_CASE( "sync-88359_civilians_report_thefts_to_human_npcs", "[npc][monster][audit]" )
{
    clear_map();
    clear_creatures();
    clear_avatar();
    const tripoint_bub_ms pos = get_avatar().pos_bub();
    npc &guy = spawn_npc( ( pos + tripoint::east ).xy(), "test_talker" );
    on_out_of_scope restore( []() {
        clear_npcs();
        clear_creatures();
        clear_avatar();
    } );
    guy.set_fac( faction_free_merchants );
    guy.set_attitude( NPCATT_NULL );
    monster &civilian = spawn_test_monster( "mon_civilian_panic", pos + tripoint::west );
    REQUIRE( civilian.get_monster_faction() == monfaction_human.id() );
    civilian.anger = 0;

    item note( itype_FMCNote );
    civilian.witness_thievery( &note );
    CHECK( guy.get_attitude() == NPCATT_RECOVER_GOODS );
    CHECK( civilian.anger == 0 );
}
