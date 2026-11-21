# Group D_engine: engine bug fixes and small features (23 PRs)

Read-only review against the fork branch `refs/fork/ours` (5510d452) and base `a1a646d6`. Nothing was built or run. "Fork" means nickclysm; finding IDs are from `Audit/`.

| Row | PR | Title | Kind | Verdict | Effort | Meets fork work | Notes |
|---|---|---|---|---|---|---|---|
| 3 | #89346 | Flipped trap/furniture detection in sleep.cpp (rollmats) | bugfix | TAKE-ADJUST | trivial | none (sleep.cpp untouched by fork) | Fixes the rollmat. Side effect: starting `ret` at 0 drops the negative comfort of bare floor and rough ground |
| 8 | #89385 | NPCs fetch construction components from containers, spread over sites | bugfix+feature | TAKE-ADJUST | small | Same functions as fork ACT-P03 memo and the ACT-J03 end hooks; applies cleanly | New "another builder finished it" exit runs no end hooks. ACT-B07 (vehicle cargo) stays open |
| 15 | #89407 | Loop around in dialogue boxes | bugfix (UI) | TAKE-ADJUST | trivial | npctalk.cpp opt_imgui, next to the fork's response_ind bounds fix | Uses `response_ind` as the count; that goes stale after a refused pick. Use `response_hotkeys.size()` |
| 35 | #89419 | Dragged furniture deletes items | bugfix | TAKE | trivial | none (`grabbed_furn_move` untouched) | Correct swap of the two tiles' items |
| 74 | #89460 | Activating explosives takes time | balance | TAKE | trivial | none | 1–2 s to arm grenades; barrel bombs now need fire. Series with #89567 |
| 77 | #89498 | Bionic canceled_mutations can be destroyed by a mutation | bugfix/design | TAKE | trivial | mutate_towards (fork MUT-B05 reorder, MUT-B12) | Turns the old "strains painfully" block into dead code |
| 80 | #89139 | Zone "Work: Dissect Corpses" | feature | TAKE-ADJUST | small | NPC zone jobs; fork `eoc_starters` (ACT-J02); ACT-J12/J13/P12 patterns | **Crash**: no `NEEDS_DISSECTING` case in `enum_to_string<do_activity_reason>` (cata_fatal) |
| 96 | #89517 | Better explosive use-action info | feature (UI) | TAKE | trivial | none | Untranslated `"<num> g of TNT"`; dead null check |
| 97 | #89499 | No deconstructing ter/furn that can't be carried | bugfix/content | TAKE-ADJUST | small | construction.cpp (fork untouched here); TERR-B15 open | Terrain branch `\|\|` makes every furniture-free tile "deconstructable" |
| 115 | #89550 | Prereq for Aquatic Sleeper | content fix | TAKE | trivial | mutations.json (other entries) | none |
| 117 | #89552 | More NO_AUTO_CONSUME | content | TAKE | trivial | none | none |
| 127 | #89575 | Fix invalid comparison in overmap_ui | bugfix | SKIP | trivial | **Duplicate of UIHUD-B03** (fork `1669b081`), a better fix | Keep the fork's version on conflict |
| 128 | #89559 | Massively expand crafting faults | feature+balance | TAKE | small | fault.cpp, item_degrade.cpp (fork ITEM-B04 fix nearby); CRAFT-B01 | New `attack_cost_modifier` field; failed smithing is harsher (balance note) |
| 129 | #88359 | Bypassing security is shoot on sight | feature/design | TAKE-ADJUST | medium | **Text conflict** faction.cpp (NPC-J02 copy-from); u_examine; MONAI-J01 `u_attack` | NPCs of 20 vanilla factions lose the "human" monfaction (likely regression) |
| 133 | #89166 | Obsolete and migrate cash cards | content/migration | TAKE-ADJUST | small | TEST_DATA/items.json (append only) | ATM "Purchase cash card?" still creates the removed `cash_card` (debugmsg) |
| 144 | #89567 | Move costs for activatable items pt. 1 | balance | TAKE | trivial | none; fork USE-B09 blindfold test unaffected | Series with #89460 |
| 151 | #89629 | Hub shares public goods | content | TAKE | trivial | none | Lets your camp/NPCs drink from Hub water (share_public_goods) |
| 164 | #89649 | Harder to miss AltarOS boiler mission | content fix | TAKE | trivial | none | Depends on #89409 and #89437 (standalone conflict) |
| 166 | #89645 | The terminal closes after murder | content | TAKE | trivial | none (uses NPC_DEATH death_eocs) | Adds a debug-only "Hub transposer" item |
| 168 | #89621 | New trait: Opiate Intolerance | content | TAKE-ADJUST | trivial | none | **Data bug**: material key `vit_mme` isn't a material, so the trait does nothing (free 3 points) |
| 169 | #89616 | Fouling breaks gun name colors | bugfix (display) | TAKE | trivial | none | Severity is display-only |
| 176 | #89643 | Learning bonus from raw skill XP | bugfix (UI) | TAKE | trivial | none (fork PROG fixes are in skill.cpp) | Guards division by zero |
| 101 | #89528 | Fix FEMA campsite NPC missions | content fix | TAKE | trivial | none; works with OMAP-B01/B02 fixes | Ids exist in fork data |

## Per-PR notes

### #89346: sleep comfort (TAKE-ADJUST, trivial)
`src/sleep.cpp:comfort_data::human_comfort_at` now takes the max over vehicle board, furniture, trap and terrain, which fixes a rollmat (trap) on a tile with furniture. Because `ret` starts at 0, it can no longer return a negative value. Bare floor was `0 - move_cost` = -2, rough ground and non-board vehicle tiles were lower, and all of them now read 0. That feeds `Character::sleep_readiness()` (src/character.cpp) and NPC sleep-spot ranking, so falling asleep on the ground gets slightly easier. This is probably unintended. Suggested adjustment: start `ret` from the terrain value (`ter comfort - move_cost`) and take the max of the rest. The fork's audit has no finding for this. Fork tests (`tests/npc_sleep_test.cpp`) only compare equal comforts.

### #89385: NPCs fetch from containers, spread over sites (TAKE-ADJUST, small)
- **What it does:** `fetchable_items_at()` lets `requirements_map` and `fetch_activity` see non-liquid items inside containers on loot tiles, and splits stacks out of them. Builders prefer free sites (`stable_partition` by `occupied_construction_sites()`), and the old `prune_same_tile_locations` is gone from construction. `build_construction_activity_actor::start` now sets `act.placement`. A builder whose site another builder finished ends quietly.
- **Fork overlap:** it applies cleanly.
  - `simulate_turn`: the partition goes right after `src_sorted`. That is before the fork's ACT-P03 `scoped_requirement_scan_memo` and its `work_left` / `scan_exhausted` logic, and it does not break their assumptions.
  - `requirements_map`: the fork's `already_there` set from ACT-P03 is in other hunks of the same function.
  - `fill_requirement_inventory` already counted contents through `temp_crafting_inventory` references. The PR fixes that count-vs-fetch mismatch for containers. ACT-B07 is the same mismatch for vehicle cargo, and it stays open.
- **Adjust:** the new exit in `build_construction_activity_actor::do_turn` (`!pc && construction_done_at(...)` then `act.set_to_null()`) runs no end hooks. Before, the avatar went through `cancel_activity()`, which ran the fork's ACT-J03 `cancel_EOC` and event. Run `act.take_end_hooks().run( you, … )` there; whether that counts as completed or canceled is the owner's call.
- **Minor performance:** `requirements_map` now builds an `item_location` for every nested item on every loot spot per fetch plan. The fork's memo doesn't cover this; watch the `zone_job_*` benchmarks.
- Fork NPC goals can already start `ACT_MULTIPLE_CONSTRUCTION` (it is in `eoc_starters`), so they benefit directly.

### #89407: dialogue wrap-around (TAKE-ADJUST, trivial)
In `dialogue::opt_imgui` the clamp now wraps at `response_ind` instead of `response_hotkeys.size()`. But `response_ind` is only the response count until something sets it:
- a CONFIRM or hotkey on a response whose condition fails sets it to that response's index and the loop goes on (action becomes "NONE");
- after that, DOWN and UP wrap inside `[0, that index)`. Example: refuse response 2 of 5, and the cursor is stuck on 0 and 1.

Upstream HEAD (`src/dialogue.cpp:1218`) still has this. Fix: wrap at `static_cast<int>( response_hotkeys.size() )`. It sits next to the fork's npctalk.cpp bounds check for `response_ind` (unknown hotkey). #89593 (row 136, another group) later moves `opt_imgui` to `dialogue.cpp`, so the fork's opt_imgui changes have to move with it.

### #89498: canceled_mutations (TAKE)
`Character::mutate_towards` now sends a trait listed in a CBM's `canceled_mutations` to `handle_bio_mut_conflict`, a roll to destroy the CBM with wounds, instead of the old harmless block ("strains painfully…"). That old loop right below becomes dead code; delete it.
- **When it fires:** `mutation_ok` still filters canceled traits for random and selector mutation. So the change only fires on direct paths: EOC `u_mutate_towards`, prerequisite recursion, and the fork's `switch_mutations`.
- **Fork fixes:** consistent with MUT-B05, where the bionic check still comes before the prune. MUT-B03 (fixed, `9c2914c4`) makes destroying an extended weapon CBM safe.
- **Design note:** CBMs such as eye optics, the voice box and cardio implants can now be destroyed by mutating toward a trait they cancel.

### #89139: Dissect Corpses zone (TAKE-ADJUST, small)
- **Crash (also in upstream HEAD):** the PR adds `do_activity_reason::NEEDS_DISSECTING` but no case for it in `enum_to_string<do_activity_reason>` (src/activity_actor.cpp), whose `default:` is `cata_fatal`. `multi_zone_activity_actor::check_requirements` calls `io::enum_to_string( reason )` for an NPC on any "continue" reason, and the arguments are evaluated even when nobody sees the message. So a follower on the dissect job without a CUT_FINE tool in hand crashes the game. Fix: add a case, e.g. "There is at least one corpse to dissect."
- **Fork parity (ACT-J02):** add `{ ACT_MULTIPLE_DISSECT, { &build_plain<multi_dissection_activity_actor>, false } }` to `eoc_starters` (src/activity_actor.cpp). Also add it to the zone-jobs row in `doc/JSON/EFFECT_ON_CONDITION.md` (around line 5774). Otherwise `u_assign_activity` refuses it with a load error and JSON NPC goals can't use the new job.
- **Likely retry loop:** `multi_dissection_activity_actor::multi_activity_can_do` accepts QUARTERED and FIELD_DRESS_FAILED corpses, which `set_up_butchery` refuses ("It would be futile…"). The job then keeps reassigning the same corpse. Skip those corpses in `can_do`. Also, unlike manual dissection, `can_do` refuses any corpse of an empathized species even when the character has `prof_dissect_humans`.
- **Audit patterns:**
  - copies a whole `mtype` and each corpse item per scan (ACT-P12), including an unused `const mtype corpse`;
  - adds the new id to the hardcoded activity lists (ACT-J12, ACT-J13);
  - uses the `activity_var` claim stamps (ACT-B08).
- **Fork code:** the per-turn progress `set_var` in `butchery_activity_actor::do_turn` is safe with the fork's ACT-B01 fix (`time_to_butcher > 0` on that branch). The fork didn't touch `job_data`, `find_job_to_perform`, `loot()`, `butcher_corpse_activity` or butchery.cpp. Hotkeys 'Y' don't clash.

### #89517: explosive info (TAKE)
Shows "When activated:" with the armed item's countdown explosion, the delay, power in g of TNT, and a shrapnel label in place of casing and fragment mass. Nits: the `"<num> g of TNT"` format is not wrapped in `_()`; `if( bomb_act_use == nullptr )` tests an address and is always false. Nothing in fork tests reads these lines.

### #89499: no deconstruct for heavy appliances (TAKE-ADJUST, small)
- **Bug (also in upstream HEAD):** in `construct::check_deconstruct` the furniture branch is right. The terrain branch is `!!ter.deconstruct || ( !furn.base_item.is_null() || !furn.has_flag( "NO_DECONSTRUCT" ) )`. With no furniture, `f_null` lacks the flag, so the function returns true for every tile without furniture. "Deconstruct furniture" (`constr_deconstruct`) is then offered on any terrain, NPC blueprints included. It fails only after 20 minutes, with "can not be disassembled" from `done_deconstruct`, which guards against a crash.
- **Fix:** in the terrain branch return `!!ter.deconstruct`. Optionally also fix the latent copy-paste noted in TERR-B15 (the terrain branch reads the furniture's base_item).
- **Data:** replaces EASY_DECONSTRUCT with NO_DECONSTRUCT on appliances that have a base `item`, which leaves `appliance_convert` as the way to move them. It also drops some `deconstruct` blocks (centrifuge, ASRG containment) and changes fridge masses. That is content direction and fine to take. NO_DECONSTRUCT isn't added to flags.json; the fork doesn't validate ter/furn flags either.

### #89575: overmap comparison (SKIP)
The fork's UIHUD-B03 (`1669b081`) removed the inverted `vision == unseen` load and the `cur_ter`/`ccur_ter` it fed, which nothing reads. Upstream only flips the test, so it still does a wasted `overmap_buffer.ter()` for every seen tile on every redraw (upstream HEAD still has `ccur_ter = cur_ter`). On the conflict, keep the fork's code.

### #89559: crafting faults (TAKE, balance note)
- **Engine:** a new fault field `attack_cost_modifier`, applied in `item::attack_time()` through `item::attack_time_modifier_from_faults()`. It is a good modding addition and cheap: a loop over the item's faults.
- **Data:** a new group `steel_smithed_craft_failures` (interior cracks, now -10% bash/stab/cut; "Unevenly weighted", +20% attack time; "Butchered craftsmanship", instant 1000 damage plus 50 degradation, which makes the item unrepairable). It goes on 46 smithed weapons. Bladed items keep `limp_edge` through the old group, and all 21 of them got the new group too.
- **Fork fixes:** consistent with ITEM-B04. In the fork, instant_damage makes one mechanical-fault roll instead of two, and `can_have_fault` skips faults already present. CRAFT-B01 doesn't change how often FAULT_ON_COMPLETION is set, because failure points come from `crafting_success_roll`. Still, low-skill smithing in the fork now loses components (CRAFT-B01) and also gets these harsher defects.
- **Gaps:** ITEM-J03 (fault lists append-only under copy-from) is still open. No mod uses the changed groups.

### #88359: security bypass is shoot on sight (TAKE-ADJUST, medium)
- **What it does:**
  - FACTION_SECURITY terrain (chain-link and wood fences) makes examining it a major offense.
  - A monfaction learns its owning NPC faction (`monfaction::associated_faction`, in a new finalize order).
  - `monster::attitude` returns MATT_ATTACK to the avatar when that faction is hostile.
  - `monster::attack_target` now requires HOSTILE rather than not-FRIENDLY.
  - `gun_actor::call` drops the `aggro_character` gate.
  - The faction `mon_faction` default changes from "human" to null.
- **Conflict (src/faction.cpp):** the fork's NPC-J02 rewrote the line as `optional( jsobj, was_loaded, "mon_faction", mon_faction, monfaction_human )`. Resolve it to `optional( jsobj, was_loaded, "mon_faction", mon_faction, mfaction_str_id::NULL_ID() )`, then drop the now-unused `static const mfaction_str_id monfaction_human` (an unused static can fail the build). Under the fork's faction `copy-from`, a child also inherits an explicit `mon_faction`, and the new finalize check then debugmsgs "already owned". No shipped faction copies one that has `mon_faction`, but a mod could.
- **Likely regression (also upstream HEAD):**
  - **Cause:** the empty monfaction id is a loaded sentinel, so `mfaction_str_id::NULL_ID().is_valid()` is true. `npc::get_monster_faction()` (src/npc.cpp) therefore returns "" instead of falling back to "human" for the 20 of 25 vanilla factions, and every mod faction, that set no `mon_faction`.
  - **Effect:** monfactions that list "human" now treat those NPCs by mood. In vanilla: mech_bot and passive_machine (no longer neutral), and attack_player_only, nether_hate_humans, flesh_abberation and jabberwock (no longer hate). In mods: DinoMod, Magiclysm, Defense_Mode and Personal_Portal_Storms. `monster::attitude`'s `u_fac->mon_faction` friend check also stops matching "human" monsters (civilians).
  - **Fix:** in `npc::get_monster_faction` use `!my_fac->mon_faction.is_null() && my_fac->mon_faction.is_valid()`. I could not run the game to confirm. Debug menu → creature info shows "Faction:" for an NPC.
- **Fork features:**
  - The fork's `u_examine` for the avatar (src/iexamine_eoc.cpp) copies `game::examine`'s `warn_player_maybe_anger_local_faction( false, true )`. Mirror the FACTION_SECURITY check there, or `u_examine` bypasses the new rule.
  - MONAI-J01 `u_attack` (src/talker_monster.cpp) sets the destination and calls the special attack, which reads `attack_target()`. With the HOSTILE requirement, an EOC can no longer make a monster use a special attack on a target it is merely neutral to. The fork test uses an aggression-100 zombie, so it should still pass, but mods lose that ability. Consider a "forced target" bypass. The grapple tests (`mon_debug_grabber`, aggression 100) should be unaffected.
- **Audit patterns:** a new avatar-only C++ rule in `monster::attitude` (MONAI-J06 theme; NPCs of other factions aren't considered). `faction_manager::get()` runs on each `attitude()` call for monsters with an owner faction, and it is a linear scan, so it is small but on a hot path. String flag lookup `has_flag( "FACTION_SECURITY" )`.

### #89166: cash card removal (TAKE-ADJUST, small)
Removes `cash_card` and `money` from vanilla. Migrations turn cards into `needle_wood` and money into thread with 0 charges. Finalize applies migrations to item groups and requirements, so `fletching` and the MindOverMatter and No_Hope groups are fine.
- **Left behind:** C++ still makes the item. `atm_menu::reset` (src/iexamine.cpp) offers "Purchase cash card?" whenever the bank balance is at least $10. New characters start with a random balance of up to $2000. `atm_activity_actor::finish` then builds `item( itype_cash_card )`, and `item()` does not migrate ids. In vanilla the result is a "Missing item definition: cash_card" debugmsg and an `undefined-cash_card` item. Upstream HEAD is the same.
- **Adjust:** hide the ATM card options when `!item::type_is_defined( itype_cash_card )` (Aftershock still defines it), or remove them.
- The fork's tests and the files the PR edits (crafting_test, item_test, TEST_DATA ammo) are untouched by the fork, apart from the appended TEST_DATA/items.json.

### #89621: Opiate Intolerance (TAKE-ADJUST, trivial)
`"vitamins_absorb_multi": [ [ "vit_mme", [ [ "vit_mme", 12 ] ] ] ]`: the first element is a **material** id (see `src/consumption.cpp:compute_default_effective_vitamins`), and `vit_mme` is a vitamin. So the ×12 never applies, and the trait is a free +3 chargen points that only cancels ADDICTIVE. Upstream HEAD is the same. Change the key to `"all"`. A validation pass for `vitamins_absorb_multi` materials would catch this (in the spirit of MUT-J01).

### #89649: boiler mission (TAKE)
Adds `remove_active_mission` to the labyrinth exits. The standalone conflict comes from the hunks that #89437 (labyrinth expansion: `teleport_labyrinth1_to_bridges`) and #89409 (`nl_been_to_fixbot`) introduced. Take it after those two.

### #89645 / #89629 / #89528: small content (TAKE)
- **#89645:** NPC_DEATH `death_eocs` with `has_beta` / `npc_is_avatar`. The fork runs NPC death EOCs with the killer as beta (`npc::die`), and `has_beta` is registered. The "Hub transposer" is a debug item with `"variants": []`; it spawns nowhere.
- **#89629:** robofac → your_followers `share public goods`. It affects `basecamp` water access and `npc::would_take_faction_goods` (drink and extract only).
- **#89528:** `fema_entrance` and `fema_camp_mutable` exist in fork data. These targets go through the fork's OMAP-B01/B02 fixes.

## Group summary

**Verdicts:** 14 TAKE, 8 TAKE-ADJUST (#89346, #89385, #89407, #89139, #89499, #88359, #89166, #89621), 0 OWNER, 1 SKIP (#89575).

**Series and dependencies:**
- #89460 then #89567 ("pt. 1"): item activation move costs. A later part may be in another group.
- #89649 needs #89409 and #89437.
- #89407 and #89139 edit `npctalk.cpp` before #89593 (row 136, dialogue split). Whoever takes #89593 has to carry the fork's opt_imgui changes, and the #89407 fix, into `dialogue.cpp`.
- #89559 and #89616 both edit fault JSON but are independent.
- Everything else stands alone.

**Riskiest items:**
1. #89139 crashes (`cata_fatal`) when a follower on the dissect job lacks a fine cutting tool. It is a one-line fix and also needs the `eoc_starters` entry.
2. #88359 is the only real text conflict here (faction.cpp, fork NPC-J02). It also probably moves most NPCs off the "human" monfaction, and it narrows the fork's `u_examine` and MONAI-J01 `u_attack`. Run the monster, grapple and NPC suites after merging.
3. #89499: the terrain-branch `||` offers deconstruction on every bare tile.
4. #89166: the ATM card purchase hits a missing item in vanilla.
5. #89621: the trait silently does nothing.

**Fork duplicates:** #89575 is UIHUD-B03 (keep ours). No other PR here duplicates a fork fix. #89385 fixes the container half of the count-vs-fetch mismatch whose vehicle-cargo half is ACT-B07 (still open).

**Recommendation:** take the whole group, with the eight small adjustments listed above. None of them needs more than a few lines. Only #88359 needs care (medium). Skip #89575.
