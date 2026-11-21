# Group B_combat: triage (16 PRs)

Upstream clone: `refs/fork/ours` = `5510d452`, upstream `HEAD` = `46391a6f`. Conflict hunks below were read from `git merge-tree --merge-base=a1a646d6 refs/fork/ours <PR commit>` (upstream up to and including that PR, against the fork). Nothing was built or run.

| Row | PR | Title | Kind | Verdict | Effort | Meets fork work | Notes |
|---|---|---|---|---|---|---|---|
| 7 | #89278 | (kuutekitty/monter) Skeletal hulk wind-up "decimate" attacks, `slash` for executioner/reaper | content | TAKE-ADJUST | small | Grappling port: the hulk's `grab` override drops the fork's NO_GRAB/incorporeal guards | Override replaces the base `grab` condition. Placeholder `remove_message` strings |
| 25 | #89416 | kevlar hulk special attacks | content | TAKE | trivial | none | Needs #89278 (edits its attacks). 180/280 bash smash |
| 42 | #89444 | Limbify tentacle arms | content + refactor | TAKE-ADJUST | small | melee.cpp text conflict with the fork's `mutation_attack_damage()` helper (manual combat menu) | Removes `hardcoded_mutation_attack` (MUT-J16/CMBT-J17). Needs #89450 (bad sub_part ids) |
| 56 | #89450 | Add Tentacle Rakes combat | content + bugfix | TAKE | trivial | Touches GRAB/GRAB_FILTER conditions the grappling port also uses; fits | Fixes 24 techs testing a nonexistent effect `GRAB`. Needs #89444 |
| 76 | #89469 | Combat weariness 2 | balance/feature | OWNER | medium | Clashes with the fork's EXPLOSIVE_EXERCISE level and the owner's grappling decision 5. 8 conflicting files | COMBAT_EXERCISE=500, always on. Widget bug (`== 500` never matches) |
| 90 | #89531 | revamp the berserk code a bit | refactor | TAKE | trivial | none | Same behavior, early exit |
| 91 | #89526 | Slightly nerf Nauseating Stench | balance | TAKE | trivial | MONAI-B01 (fork made effect speed_mod reach monsters) | Drops speed -10, cough 3 to 8 |
| 124 | #89547 | Tentacle clothing coverage | content | TAKE | trivial | none (clean merges in flags.json, coats.json) | Needs #89444/#89450. The new `_dual` parts may be missing from `vector_tentacle_slap` |
| 134 | #89519 | Weariness display fix | bugfix/refactor | OWNER (follows #89469) | small | none on its own | Required with #89469 (COMBAT counts as EXTRA for move penalty). Won't compile without it |
| 140 | #89470 | Partial dodge and limb shielding/deflection | feature/balance | OWNER | medium | CMBT-B12/B13/B15 code and tests, grappling grabs, expected_dps_data.json conflict | Halves head HP. Dodge counters can fire on attacks that still land |
| 145 | #89611 | Avoid duplicate monster death messages | bugfix | TAKE | trivial | Partial fix of open fork finding MONDT-B19 | Agrees with the finding. QUIETDEATH half still open |
| 158 | #89641 | Some vanilla monsters can block | content | TAKE | trivial | none | Data for #89640's blocks, but valid under the old ones too |
| 165 | #89640 | Blocks on a sliding scale alongside partial dodge | feature/balance | TAKE-ADJUST (only with #89470) | medium | Undoes part of CMBT-J01 in `block_hit`. mattack_actors.cpp conflict with CMBT-B12. Makes grapple grabs blockable | Stale `last_*` state for hardcoded monattacks |
| 172 | #89658 | Remove debugmsg from spawn_summoned_monster | bugfix | TAKE | trivial | Partial fix of open finding MAGIC-B09 | Silent skip, as the finding suggests |
| 175 | #89642 | Gozu can drag you to Hell | content | TAKE-ADJUST | small | Grappling port: Gozu `grab` override drops the NO_GRAB/incorporeal guards | spells.json conflict is only an append next to #89409 (group F) |
| 181 | #89657 | Mi-gos can block | content | TAKE | trivial | none | Data for #89640 |

## Per-PR notes

### #89278: skeletal hulk wind-up attacks (TAKE-ADJUST, small)
- **What it does:** The skeleton hulk gets a telegraphed wind-up.
  - `mon_skeleton_hulk_charge_decimate` adds `effect_skeleton_hulk_charge_up`. The next turn, `decimate` (90 cut, AP 40, range 2, unblockable) or `decimate_adjacent(_equal)` (40 bash plus a big throw) fires.
  - All its other attacks get `not charge_up` conditions. `cut_throat` is removed.
  - The executioner and the reaper get a new `slash` (40/50 cut).
- **Fork interaction (grappling port):** The hulk's list now has `{ "id": "grab", "condition": { "not": { "u_has_effect": "effect_skeleton_hulk_charge_up" } } }`.
  - A monster-level override goes through `MonsterGenerator::create_actor`, which clones the base attack and then runs `melee_actor::load_internal`. That function replaces `condition` outright.
  - So the hulk's grab loses the whole base condition. In the fork that includes the guards the grappling port added to `grab`/`grab_2`/`grab_3`: `not u_has_flag NO_GRAB` and `not npc_has_effect incorporeal` (`Audit/plans/tlg-grappling-port.md` phase 7). It also loses upstream's `GRAB_FILTER` and `maimed_arm` checks.
  - The incorporeal guard exists only in that JSON condition. `character_escape.cpp` and `monster.cpp` check incorporeal only for character grabbers. So this re-opens, for this monster, the bug "monster grabs worked on incorporeal targets" that the README lists as fixed by the port.
- **Adjust, either way:**
  - (a) Write the full guard set into the override:
    ```json
    "condition": { "and": [
      { "not": { "u_has_effect": "effect_skeleton_hulk_charge_up" } },
      { "not": { "u_has_flag": "GRAB_FILTER" } },
      { "not": { "u_has_effect": "maimed_arm" } },
      { "not": { "u_has_flag": "NO_GRAB" } },
      { "not": { "npc_has_effect": "incorporeal" } }
    ] }
    ```
  - (b) Better: move the NO_GRAB/incorporeal check into C++, in `melee_actor::call` when `is_grab`.
  - Option (b) also fixes a gap already in the fork. About 14 existing overrides drop the guards the same way:
    - `data/json/monsters/insect_spider.json`: 8 spider `grab` overrides on `maimed_leg`
    - `data/json/monsters/zed_fusion.json`: `grab_2`/`grab_3`
    - `data/mods/Magiclysm/monsters/ogre.json` and `monsters.json`
    - `data/mods/Xedra_Evolved/monsters/changeling.json` (×2)
- **Looks off:** `effect_skeleton_hulk_charge_up` has `"remove_message": "messages"` and `effect_skeleton_hulk_charge_removal` has `"remove_message": "message"`. These are placeholder strings. They stay invisible only as long as no character ever gets the effect.

### #89416: kevlar hulk (TAKE, trivial)
- **What it does:** The kevlar hulk reuses #89278's wind-up with new `decimate_bash` and `decimate_bash_equal` (180/280 bash, AP 15, spread damage, throw 86/43). `cut_throat` is removed, and its smash and wide swing get size, `maimed_arm` and charge conditions.
- **Dependency:** It edits attacks #89278 adds, so it needs #89278.
- **Fork interaction:** None. Its overrides touch no grab.

### #89444: limbify tentacle arms (TAKE-ADJUST, small)
- **What it does:**
  - ARM_TENTACLES/_4/_8 now use enchantment `modified_bodyparts`: they lose `arm_*`/`hand_*` and gain 2, 4 or 8 `arm_tentacle_*` limbs with sublimbs. A bionic arm keeps its side.
  - The flat 30 hand encumbrance and the +1 DEX are dropped. The 4 and 8 variants get aim and recoil enchantments.
  - The hardcoded tentacle slap/rake goes away: `TECH_TENTACLE_SLAP` on a new `vector_tentacle_slap`.
  - `hardcoded_mutation_attack()` and the four trait ids are deleted from `src/melee.cpp`.
- **Conflict:** `src/melee.cpp` (INCREMENTAL NEW).
  - In the fork, the manual combat mode (grappling port, `f4ef8db6`) split `Character::mutation_attacks()` into `mutation_attack_blocked()` and `mutation_attack_damage()`. `add_mutation_attack_entries()` (technique menu) uses them too.
  - Resolve it this way:
    - delete `hardcoded_mutation_attack()` and the `trait_ARM_TENTACLES*`/`trait_CLAWS_TENTACLE` constants in melee.cpp
    - keep `trait_CLAWS_TENTACLE` in `src/grapple.cpp`, which `grapple::attempt_grab` uses for aquatic grabs
    - drop the `if( mut_atk.hardcoded_effect )` branch from `mutation_attack_damage()`
    - keep the fork's helpers and `mutation_attacks()` body
- **Agrees with open findings:** MUT-J16 and CMBT-J17 (hardcoded tentacle ids). MUT-J16 also asks to drop `hardcoded_effect` or reject it at load.
  - After this PR, `mut_attack::hardcoded_effect` is still read in `src/mutation_data.cpp`, but nothing uses it.
  - A mod attack with `"hardcoded_effect": true` then silently does zero damage. No shipped data or docs use it after the PR.
  - Suggest making it a load error.
- **Fork fix that helps it:** CHAR-J01 (`30c665af`, enchantment body parts now revert). Without it these conditional limb swaps wouldn't undo.
- **Looks buggy on its own:**
  - `arm_tentacle_l_2/_3/_4` list `sub_parts` with `_r` ids that don't exist (`tentacle_left_2_base_r`; the sublimbs are defined as `_l`). #89450 fixes this, so take the two together.
  - Still wrong at upstream HEAD: `arm_tentacle_l_2/_3/_4` have `"side": "right"` and the accusative "right tentacle".
  - The ARM_TENTACLES_8 description says "four tentacles". #89547 fixes that.
- **Grappling port, worth a play-test:**
  - Non-`_dual` tentacles have lift 0.2 × arm type 0.5 = 0.1, exactly the `Character::get_working_arm_count()` cutoff. Any lift loss (wounds, encumbrance, the `grabbed` effect's lift modifier) leaves a 4- or 8-tentacle character with 0 working arms.
  - `grapple::grab_refusal()` then refuses the grab ("You can't manage that in your current condition.").

### #89450: tentacle rakes (TAKE, trivial)
- **What it does:**
  - Adds `TECH_TENTACLE_SLAP_CLAWS`, `TECH_TENTACLE_CLAW` and `TECH_TENTACLE_GRAB_CRUSH` to the tentacle limbs.
  - Fixes #89444's sub_part ids.
  - Removes CLAWS_TENTACLE's old encumbrance.
  - Fixes a real bug in 24 mutation techniques (and 2 Magiclysm ones): `{ "not": { "u_has_effect": "GRAB" } }` tested an effect id that doesn't exist (`GRAB` is a technique and a flag). It is now `u_has_flag`.
- **Fork interaction:** The audit didn't catch that bug. The grappling port puts GRAB (victim) and GRAB_FILTER (`grabbing`, grabber) on character holds too (`grapple.cpp`), so the fixed conditions behave sensibly in the fork.
- **Optional fork touch:** `TECH_TENTACLE_GRAB_CRUSH` requires "target grabbing, me grabbed". It could also accept `{ "u_is_grabbing": "npc" }` (the fork's EOC condition), so a tentacled character can crush what it holds.

### #89469: combat weariness 2 (OWNER, medium)
- **What it does:**
  - Adds `COMBAT_EXERCISE = 500` (EXTRA is 10). `melee_attack_abstract`, `reach_attack` and `on_try_dodge` log it.
  - `maximum_exertion_level()` returns it at weariness 0.
  - Since BMR (`get_bmr()`) scales with the 5-minute average activity, clamped to that maximum, a fresh fighter burns calories very fast. That is what makes them weary.
  - My reading: a minute of steady fighting averages about 100× BMR for that 5-minute window, roughly 500-600 kcal. Today it is about 3×. Weariness 1 then caps it at ×8.
  - The PR adds a 48-minute boxing test that expects weariness 2.
- **Clash with the fork:** The grappling port added the TLG level `EXPLOSIVE_EXERCISE = 12` (`src/game_constants.h`) behind the world option `EXPLOSIVE_COMBAT_EXERTION` (off by default, `src/options.cpp`).
  - Fork functions involved: `Character::melee_exertion_level()` (`src/character_health.cpp`), `Character::maximum_exertion_level()`, `on_try_dodge()`, `melee_attack_abstract()`, `reach_attack()`, `reduce_moves_from_attack()`, `Creature::attack_air()`, `grapple::exertion_level()`, the `ranged.cpp` throw, the calorie diary column in `avatar.cpp`, `display::activity_level_str()`, and the `activity.json` widgets.
  - The owner decided (`Audit/plans/tlg-grappling-port.md`, "Decisions (2026-11-02)", item 5) that the combat exertion rebalance is opt-in, and that "with the option off, nothing changes". #89469 makes a far bigger rebalance (×50 vs ×1.2) always on.
  - `tests/grapple_world_test.cpp` "explosive_combat_exertion_option" asserts `maximum_exertion_level() == EXTRA_EXERCISE` with the option off. It fails if the PR is taken as is.
- **Text conflicts:** `character.cpp` (`on_try_dodge`), `character_health.cpp` (`maximum_exertion_level`), `game_constants.h` (constant plus both maps), `display.cpp`, both `activity.json` widgets, `doc/JSON/NPCs.md`, `doc/PLAYER_ACTIVITY.md`, and melee.cpp in two places.
  - Careful at melee.cpp:850. The upstream side brings back the "too unwieldy" check and the long-attack popup after the hit roll. The fork moved both before the hit roll (CMBT-B10, `8c8af264`). Take only the `set_activity_level` line, or CMBT-B10 is undone.
- **What the owner chooses between:**
  - (1) Keep the fork's opt-in EXPLOSIVE and skip #89469/#89519. Optionally take the test-helper changes.
  - (2) Take upstream's always-on COMBAT. That overrides decision 5 and needs `grapple_world_test` updated.
  - (3) Merge them: keep `melee_exertion_level()` (10/12) for every move-cost multiplier. Log COMBAT only when an option says so (for example, turn `EXPLOSIVE_COMBAT_EXERTION` into a choice of extra, explosive or combat). Make `maximum_exertion_level()` case 0 follow it.
  - In every case keep the move penalty computed as EXTRA, as #89519 does. Through `melee_exertion_level()` a weary character's attacks would otherwise be 8/500, about 60× slower.
- **Upstream bugs to fix if taken:**
  - `u_val('activity_level')` is `floor(level/2)`, so COMBAT reads 250, not 500.
    - The new `data/json/ui/activity.json` and zenfs clauses (`== 500`) never match, so the widget goes blank in combat.
    - NPCs.md documents 500.
    - The `activity_level_index()` comment says 1000.
  - zenfs and sidebar-mobile reuse the clause id `extreme`.
  - `PLAYER_ACTIVITY.md` lists COMBAT_EXERCISE as a valid activity or recipe level. A mod using it would burn up to 500× BMR, so I'd leave it out of that list.

### #89519: weariness display fix (OWNER, follows #89469)
- **What it does:** `exertion_adjusted_move_multiplier()` treats COMBAT as EXTRA. `activity_tracker::tracker/intake` become private, with a `debug_get_intake()` accessor.
- **Fork interaction:** None. The fork never touches those members.
- **Dependency:** It references `COMBAT_EXERCISE`, so take it exactly when #89469 is taken. It is needed then.

### #89470: partial dodge and limb deflection (OWNER, medium; recommend taking with the adjustments below)
- **What it does:**
  - `Creature::melee_damage_mult(hit_spread)` scales damage linearly from 0 at hit_spread -25 to 1 at +25. `melee_attack_hit()` (hit_spread > -25) replaces `hit_spread >= 0` in `melee_attack_abstract`, `perform_special_attacks`, `monster::melee_attack` and `melee_actor::call`.
  - `Creature::deflect_hit_part()`: a character that doesn't block may shift a head hit to the torso or an arm, and a torso hit to an arm or leg. The chance depends on dodge, melee skill × reaction and limb block scores.
  - Hit-quality text is added ("A glancing hit.").
  - `item::effective_dps()` integrates over 11 damage buckets.
  - **It also halves head, eyes and mouth HP (60 to 30), bionic heads (150 to 75) and mutant heads and muzzles.** That balance change is hidden in this PR.
- **Conflicts:**
  - `src/melee.cpp` `Character::perform_special_attacks()`: keep the fork's `melee::hit_roll_against(t, …)`, which is CMBT-B12's HARDTOHIT double roll, and use `melee_attack_hit()`.
  - `data/mods/TEST_DATA/expected_dps_data.json`: the polearms and the `qt_aztec_spear` blocks, where the fork has CMBT-B15's ×0.8. Regenerate the whole file from `expected_weapon_dps` after the merge. Don't hand-merge it: the fork's polearm values aren't exactly the old value × 0.8, and the merged file keeps the fork's `polesaw_on`.
  - `Creature::deal_melee_hit()` merges cleanly. Keep the fork's order there: `modify_damage_dealt_with_enchantments` after the rider redirect, CMBT-B03 `0631addd`.
- **Fork fixes and features it changes:**
  - **Dodge counters on attacks that land.** `Creature::deal_melee_attack()` still calls `on_dodge` (dodge counters, ondodge buffs and EOCs, dodge practice) for hit_spread ≤ 0. Hits now land down to -24, so a defender can counter and also be hit.
    - `tests/audit_combat_test.cpp` "cmbt-b11-counters_tally_counts_only_the_attackers_hits" expects "0 enemies hit!" when the NPC counters. It will fail intermittently.
    - Suggest firing `on_dodge` only when `!melee_attack_hit(hit_spread)`, or accept this and fix the test.
  - **CMBT-B12 test.** "cmbt-b12_hardtohit_makes_melee_attackers_roll_twice" has `REQUIRE( plain < 0.95 )`. With the -25 threshold, my estimate is that the default avatar's plain hit rate rises from about 0.66 to about 0.92, close to that bound. Check it, and retune the target or the bounds if needed. The squared relation still holds.
  - **CMBT-B13 and B11 tallies now count glancing hits:** `did_hit` (hit_you_effect), `ma_ctx.hit`, `last_finished_attack.hit`, `melee_stats.hit_count`, the DEFT miss refund, and the `character_melee_attacks_*` events. That is consistent, but the docs for the EOC context `hit` should say so.
  - **Grappling.**
    - Monster grabs in `melee_actor::call` now succeed on partial hits (hitspread -24..-1). The grab branch runs after the dodge check, and `do_grab` (including `pull_from_vehicle`) doesn't scale.
    - Character grabs (`grapple::attempt_grab`, `hitspread < 0`) and `avatar::disarm` keep the old threshold.
    - Suggest one rule for grabs. My suggestion: grabs stay all-or-nothing at ≥ 0, with an `is_grab` check before the grab branch.
- **Performance:**
  - `effective_dps()` now resolves damage for 11 buckets per crit/non-crit pass. It sits under NPC weapon scoring (`Character::melee_value()`, `average_dps()`, open CMBT-P07) and the advanced inventory's damage sort comparator.
  - Re-run the fork's `npc_weapon_eval` benchmark (NPC-P02).
  - `deflect_hit_part()` adds a `get_dodge()` call per hit on a character (open CMBT-P12, which builds an `anatomy`).
- **No clash:** CMBT-B04 (weak-point sorting), the weak-point defaults (deflection only applies to characters, which have no weak points) and CMBT-B14. Glancing hits do trip `damage_required` thresholds less often. That is expected.
- **Owner's choice:** upstream's continuous-hit model (with a half-HP head) versus today's hit-or-miss model, which the fork's combat fixes and tests were tuned on. Later upstream combat PRs (#89640 already) build on it, so skipping it means diverging.

### #89611: duplicate monster death messages (TAKE, trivial)
- **What it does:** Removes the hardcoded "The %s dies!" from `mdeath::normal`. `monster::die` prints a configured message once. Without one, it prints the default only for NORMAL corpses that aren't `quiet_death`/QUIETDEATH/`no_corpse_quiet`.
- **Compared with MONDT-B19** (`Audit/20-monster-data-creatures.md`, open, no fork fix): same approach (print only in `die()`, drop the copy in `mdeath::normal`). It fixes the duplicate.
- **What is still open:** Any monster with a `death_function` gets the non-empty default "The %s dies!" from `monster_death_effect::load`. That message is still printed even for QUIETDEATH monsters. The GEN_DORMANT pseudo-types from `MonsterGenerator::generate_fake_pseudo_dormant_monster` still keep their message.
- **Follow-up:** Gate `show_death_message()` on the quiet flags for configured messages too, and clear `death_message` in the pseudo-dormant generator. No fork test checks death messages.

### #89640: sliding-scale blocks (TAKE-ADJUST, medium; only with #89470)
- **What it does:**
  - `Character::block_roll()` and `monster::block_roll()` give a block score that `Creature::deal_melee_attack( source, hitroll, allow_block = true )` subtracts from the hit spread, next to dodge. `deal_melee_attack` is no longer virtual; the fork has no override.
  - `block_hit()` now reads `last_hit_spread/last_block/last_dodge`. It counts as a block when it cut the damage multiplier by at least 0.05, and labels the result block (more than half), parry (a third or more) or dodge.
  - Stamina cost scales with the effect. The 2000-stamina cutoff and the reaction roll are gone. Block counters need block_share ≥ 0.33.
  - Crits use `last_dodge + last_block`.
- **Conflicts and adjustments:**
  - `src/mattack_actors.cpp` `melee_actor::call()` (INCREMENTAL NEW). Combine both sides:
    ```cpp
    target->deal_melee_attack( &z, melee::hit_roll_against( *target, [acc] { return melee::melee_hit_range( acc ); } ), blockable );
    ```
    Otherwise HARDTOHIT (CMBT-B12, owner: "keep it") is lost for monster special attacks.
  - `src/melee.cpp` `Character::block_hit()`. Upstream moved the sleep/winded/fearparalyze/driving and size checks before `ma_ongethit_effects( *this )` and dropped the context. The fork's CMBT-J01 (`7dfdc8e4`) fires ongethit first, with a `ma_event_context` (opponent, bp, damage), so get-hit hooks work while winded, asleep or driving.
    - Keep the fork's block at the top. Add only the size check after it, with no second ongethit call.
    - `tests/audit_martial_art_events_test.cpp` "cmbt-j01_ongethit_and_ondodge_eocs_see_the_attacker" catches it if this goes wrong.
    - `ma_onblock_effects( *this, block_ctx )` with the fork's context merges cleanly. Keep it.
  - The `perform_special_attacks()` and `mutation_attacks()` hunks repeat the #89470 and #89444 resolutions.
- **Decisions the fork must make:**
  - **Grabs become blockable for free.** `grapple::attempt_grab()` (two `deal_melee_attack` calls) and `avatar::disarm()` get the default `allow_block = true`. A target's block roll now lowers the grab or disarm roll, but no block is spent, no stamina is paid and no counter fires.
    - The port's design is a grab "with a hit roll it can dodge". Pass `false` there unless the owner wants parried grabs. In that case also spend the block.
    - Monster `grab` attacks are `blockable` by default, so they get blocked too.
  - **Held monsters.** In the fork, `monster::get_dodge()` returns 0 for a held monster (grappling port). The new `monster::block_roll()` has no such check, so a held feral or mi-go (#89641, #89657) still blocks. Add the same GRAB check for consistency.
- **Looks buggy upstream:**
  - **Stale block state.** `block_hit()` reads the defender's `last_*` from the most recent `deal_melee_attack`, whoever made it.
    - Six hardcoded attacks call `block_hit` after `dodge_check` without one: `mattack::acid_barf`, `fungus_inject`, `fungus_bristle`, `fungus_fortify`, `flesh_golem` and `lunge` (`src/monattack.cpp`). They block, or fail to, on the previous exchange's numbers, and still spend `blocks_left`.
    - The same happens when `deal_melee_hit` redirects a hit on a mount to its rider.
    - Suggest resetting `last_block/last_hit_spread` in `dodge_check`, or having those attacks call `deal_melee_attack`.
  - **Two counters for one attack.** A dodge counter (`on_dodge` at ≤ 0) and a block counter (block_share ≥ 0.33) can both fire. The fork's `d73fb417` (a technique may be several kinds of counter, which brought Xedra Evolved's devil tail back) makes that reachable for one technique.
  - `block_roll()` casts the float block score to int for `logarithmic_range(int,int,int)`. That truncates the fractional martial-art bonuses CMBT-B16 made float again. It is minor: one point of score.
- **Related open finding:** CMBT-J07 (blocking tuned in C++). It is still hardcoded, with different constants.

### #89642: Gozu drags you to Hell (TAKE-ADJUST, small)
- **What it does:**
  - The Gozu gets `grab` (only on fear-paralyzed targets, `grab_strength` 50, `GRABS`) and a spell that sends a grabbed, paralyzed target to a new `gozu_hell` dimension.
  - The new dimension has overmap, mapgen, region settings, an exit vortex trap, an achievement and a MindOverMatter teleport-immunity hook.
- **Fork interaction:** The same grab-override problem as #89278. `{ "id": "grab", "cooldown": 2, "condition": { "npc_has_effect": "fearparalyze" } }` drops `GRAB_FILTER`, `maimed_arm`, `NO_GRAB` and `incorporeal`. Add the guards in an `and`, or fix it engine-side as in #89278.
- **Check in play:** The fork's dimension travel (`game::travel_to_dimension`) doesn't call `grapple::release_*`. The victim's `grabbed` effect is only removed later as an orphan grab in `character_escape.cpp`. That is probably harmless, but the port's rule is that a hold ends however it ends.
- **Dependency:** The spells.json standalone conflict is only an append next to #89409 (group F). Under the fork's MAGIC-J03 load checks, the new spell's EOC id resolves.

### #89526, #89531, #89547, #89641, #89657, #89658 (TAKE)
- **#89526:** The fork applies effect `speed_mod` to monsters (MONAI-B01). Where monsters in `fd_nauseating_smell` get the effect, removing `speed_mod` -10 also stops it slowing them in the fork. In upstream it never slowed them. I didn't check whether field effects reach monsters here. This is minor.
- **#89531:** A pure refactor of the berserk retreat check in `avatar_action::move`, with an early exit. The fork's edits in that file are elsewhere.
- **#89547:**
  - Arm-tentacle clothing coverage (an `ALLOWS_ARM_TENTACLES` flag, sleeveless coats, tentacle sleeves). ARM_TENTACLES (2) now gains `arm_tentacle_l_dual`/`_r_dual`.
  - Possible bug: `vector_tentacle_slap` doesn't list the `_dual` parts, and their `similar_bodypart` is `arm_l`. Unless finalize's similar-part expansion covers them, two-tentacle characters lose the slap and claw techniques. Check it in game.
- **#89641/#89657:** `block` data for ferals, mutants, nether creatures and mi-gos. It works under the current flat block and was tuned for #89640. Under #89640, with the fork's grapple rolls, these monsters also resist grabs (see #89640).
- **#89658:** Removes the player-visible "failed to place monster" debugmsg. That is the "silent skip" half of open finding MAGIC-B09 (`Audit/27-magic-enchantments.md`). The finding suggests `add_msg_debug` instead, and pre-filtering occupied tiles.

## Group summary

- **Verdicts:** 9 TAKE, 4 TAKE-ADJUST, 3 OWNER, 0 SKIP.
- **Series and dependencies:**
  - #89278 then #89416 (hulk attacks).
  - #89444, #89450 and #89547 (tentacle limbs). #89444 alone has 9 sub_part ids that don't exist, which #89450 fixes. I didn't confirm whether those fail at load or only when used.
  - #89469 then #89519 (combat exertion). #89519 can't be taken alone.
  - #89470 then #89640 (partial dodge, then blocks built on `melee_damage_mult`, `melee_attack_hit` and `last_*`). #89641 and #89657 supply monster block data for #89640.
  - #89642 sits next to #89409 (group F) in spells.json, as an append only.
- **Riskiest:**
  - #89469: against the owner's grappling decision 5. A naive merge breaks `grapple_world_test`, and the melee.cpp hunk can undo CMBT-B10.
  - #89470: semantic shifts the fork's combat tests and the grappling port depend on (dodge counters on landed hits, glancing grabs, half head HP, DPS regeneration).
  - #89640: undoes part of CMBT-J01 if merged carelessly, drops HARDTOHIT from monster special attacks if the mattack_actors.cpp hunk is resolved upstream-side, makes grabs blockable for free, and brings stale block state into hardcoded attacks.
- **Fork gap found while checking:** Any monster-level `condition` on `grab` replaces the base condition, so the grappling port's NO_GRAB and incorporeal guards are already lost on about 14 existing overrides (spiders, fusion zombies, Magiclysm, Xedra). Moving the guard into `melee_actor::call` fixes those and makes #89278 and #89642 plain TAKE.
- **Recommendation:** Take the content and small fixes now (#89416, #89450, #89526, #89531, #89547, #89611, #89641, #89657, #89658, with #89278, #89444 and #89642 adjusted). Get an owner call on the two combat redesigns:
  - **Exertion:** I'd merge COMBAT behind the fork's existing world option rather than replace it.
  - **Partial dodge plus blocks:** I'd take them as a pair, with the CMBT-B12 and CMBT-J01 resolutions above, an explicit rule for grabs, the `on_dodge` threshold fix, the stale-state fix, regenerated DPS data and retuned cmbt-b11 and cmbt-b12 tests. The owner should approve the head-HP halving explicitly.
