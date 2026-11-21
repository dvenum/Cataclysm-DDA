# Triage: group C_refactors (12 PRs)

These are the large refactors and infrastructure changes. Everything here was read-only analysis. I did not build or run tests. The conflict and "applies cleanly" claims come from `git merge-tree` against `refs/fork/ours` and from `patch --dry-run` of the fork's hunks against scratch copies of upstream HEAD's files.

| Row | PR | Title | Kind | Verdict | Effort | Meets fork work | Notes |
|---|---|---|---|---|---|---|---|
| 13 | #89339 | More support for C++20: char8_t | build/CI | TAKE | trivial | Overlap files merge clean. The fork adds no `u8` literals and builds as C++17. | 13 fork-added `fs::path::generic_u8string()` calls and 2 `u8path` calls only matter if the fork moves to C++20 |
| 48 | #89274 | performance metrics report | feature (profiling) | TAKE-ADJUST | trivial | Conflicts with `do_turn.cpp` LOOP-B15 (`handle_weather_effects` gate). The `map.cpp` include conflict is next to `phase_change.h`. | Always-on `cata_timer` tree. Keep the fork's ungated weather call. |
| 73 | #89451 | repair_item_activity_actor | refactor | TAKE-ADJUST | small | `activity_type.cpp` (ACT-J01 generic_factory rewrite) conflicts with the removal of `call_finish`/`finish_functions`. ACT-J01/J03 hooks lose their beta talker for ACT_REPAIR_ITEM. | Drop both blocks. Add a `hook_target` override. The VINT-J09 id list moved but is not fixed. |
| 84 | #89509 | Remove legacy `int`-indexed item accessors | refactor | TAKE | trivial | none (no fork code uses the removed accessors) | #89473 needs it |
| 136 | #89593 | dialogue-split: npctalk.cpp → dialogue.cpp | refactor | TAKE-ADJUST | small | 9 of the fork's 94 npctalk hunks move: EOC-B09, EOC-B10, MGEN-B03, NPC-J04 | The fork's EOC effects all stay in npctalk.cpp. Fix the `talk_topic_exists` namespace. |
| 139 | #89473 | use `item_location` in use actions instead of `item *` | refactor | TAKE-ADJUST | medium | USE-B01, B06, B08, J01 and MUT-B08 code conflicts or stops compiling. 18 fork test call sites in 5 files. | Adds 15 broken strings ("…it->"). Makes the USE-P08 fix easy. |
| 142 | #89612 | Split vehicle functions → map_vehicle.cpp | refactor | TAKE-ADJUST | small | `get_moving_vehicle_targets` (MONAI-P01) | Series 1/6. The map.h include changes merge clean. |
| 156 | #89625 | Split cache functions → map_cache.cpp, IWYU for map.cpp | refactor | TAKE-ADJUST | trivial | `build_map_cache` character lights (SPAT-P02) | Series 2/6 |
| 163 | #89646 | map-split-damage → map_damage.cpp | refactor | TAKE-ADJUST | small | MAP-J01 bash EOCs (4 functions + 2 new statics), MAP-B06 `smash_items` | Series 3/6. Needs 3 includes. |
| 177 | #89660 | Split visibility functions → map_visibility.cpp | refactor | TAKE-ADJUST | small | MAP-P02 lazy dirty setters (7 functions) | Series 4/6 |
| 179 | #89666 | Split architectural functions → map_architectural.cpp | refactor | TAKE-ADJUST | small | MAP-P02 lazy setters, `build_outside_cache`, `build_floor_cache` | Series 5/6 |
| 182 | #89674 | Split creature functions → map_creature.cpp | refactor | TAKE-ADJUST | small | MONAI-B03 `reachable_flood_steps`. MAP-P02 lazy pathfinding caches: if these are missed, the game crashes. | Series 6/6 |

---

## Map split: #89612, #89625, #89646, #89660, #89666, #89674

**What it does.** Six pure moves. `map.cpp` shrinks from 12,347 lines at the fork point to 6,565. The rest moves into:

| File | Lines |
|---|---|
| map_vehicle.cpp | 1,509 |
| map_damage.cpp | 1,344 |
| map_architectural.cpp | 1,160 |
| map_creature.cpp | 924 |
| map_visibility.cpp | 729 |
| map_cache.cpp | 405 |

I compared the removed and added lines of each PR. The only content changes are these:
- #89666 makes `has_floor_gap_flag` a `map` member (it was a file-static function) and declares it in map.h.
- #89625 removes `src/map.cpp` from `tools/iwyu/bad_files.txt`, so IWYU now runs on it.
- Each new file gets its own `static const` ids and the `dbg` macro.

No other upstream PR in the window changes these functions, apart from #89274's `process_falling` timer. So the fork's versions of the functions carry over unchanged.

**Dependencies.** The six PRs are one series and must be taken in order:
- #89660 moves `mark_turn_light_dirty` out of the file #89612 created.
- #89666 depends on the earlier PRs' map.h state.
- #89274 (row 48) comes first: its `process_falling` timer is in code #89666 moves.

**How git sees it.** The split is not a rename: map.cpp stays, so git cannot follow the moved functions. A full merge of upstream into the fork leaves 10 conflict blocks in map.cpp:
- Three are includes and static ids.
- Seven are "fork's version of a big region vs. nothing": the regions upstream deleted.

The new files come in clean, but they hold upstream's (pre-fork) bodies. **Taking "theirs" for those blocks silently drops every fork change in moved code.**

Some of those losses won't even show as compile errors:
- **The worst one: `map::map()`.** It sits outside any conflict block, so it merges with the fork's lazy pathfinding-cache allocation (MAP-P02). `map_creature.cpp` would then hold upstream's `get_pathfinding_cache()`, which does `return *pathfinding_caches[...]` with no allocation. The result is a null dereference as soon as a submap loads, because `loadn` calls `set_pathfinding_cache_dirty`.
- `bash()`'s new `Creature *basher` parameter does give a compile error: map.h auto-merges with it, but map_damage.cpp's definition lacks it.
- The others only show up in tests:
  - `audit_bash_eoc_test` for MAP-J01.
  - `audit_overmap_worldgen_test` for MAP-B03, B06 and B07.
  - The benchmarks for MAP-P02, MONAI-P01 and SPAT-P02.
  - No test names MONAI-B03.

**Routing of all 40 fork hunks in `src/map.cpp`.** The diff is `git diff a1a646d6 refs/fork/ours -- src/map.cpp`, +330/−141 lines. In the last column, "exact" means the hunk applies with `patch -F0` to upstream HEAD's file of that name.

| # | Base line | Function | Finding | Lives in after the split | patch |
|---|---|---|---|---|---|
| 1 | 40 | includes `dialogue.h`, `effect_on_condition.h` | MAP-J01 | map_damage.cpp | manual |
| 2 | 86 | include `phase_change.h` (next to #89274's `perf.h`) | MGEN-P01 | map.cpp | fuzz |
| 3 | 99 | include `talker.h` | MAP-J01 | map_damage.cpp | manual |
| 4 | 131 | `static efftype_id effect_airborne` | grappling 5a | map.cpp (context `effect_boomered` moved to map_cache.cpp) | manual |
| 5 | 360 | `map::map()`: no eager pathfinding caches | MAP-P02 | map.cpp | exact (merges silently) |
| 6 | 399 | `set_transparency_cache_dirty(int)` + comment | MAP-P02 | map_architectural.cpp | fuzz |
| 7 | 413 | `set_transparency_cache_dirty(p, field)` | MAP-P02 | map_architectural.cpp | exact |
| 8 | 426 | `set_seen_cache_dirty(p)` | MAP-P02 | map_visibility.cpp | exact |
| 9 | 439 | `set_seen_cache_dirty(z)` + `set_outside_cache_dirty` | MAP-P02 | split: map_visibility.cpp + map_architectural.cpp | manual |
| 10 | 456 | `set_floor_cache_dirty` | MAP-P02 | map_architectural.cpp | exact |
| 11 | 465 | `cast_reached`, `bump_geometry_revision`, `set_lightmap_cache_dirty` | MAP-P02 | split: visibility / architectural / visibility | manual |
| 12 | 489 | `set_lightmap_cache_dirty_below` | MAP-P02 | map_visibility.cpp | exact |
| 13 | 509 | `memory_cache_dec_set_dirty` | MAP-P02 | map_visibility.cpp | exact |
| 14 | 529 | `memory_cache_ter_set_dirty` | MAP-P02 | map_visibility.cpp | exact |
| 15 | 1597 | `get_moving_vehicle_targets` (per-level `vehicle_list`) | MONAI-P01 | map_vehicle.cpp | exact |
| 16 | 2074 | new static `forget_decoration_if_seen` | MAP-P02 (b8e7484f) | map.cpp | exact |
| 17 | 2184 | `furn_set` | MAP-P02 | map.cpp | exact |
| 18 | 2682 | `ter_set` | MAP-P02 | map.cpp | exact |
| 19 | 3958 | `decay_fields_and_scent` (age the real entry; all levels) | MAP-B03, LOOP-B15 | map.cpp | exact |
| 20 | 4309 | `smash_items`: wheel miss | MAP-B06 | map_damage.cpp | exact |
| 21 | 4376 | `smash_items`: eject in all directions | MAP-B06 | map_damage.cpp | exact |
| 22 | 4547 | new statics `run_bash_eoc`, `ter_furn_bash_context` | MAP-J01 | map_damage.cpp | exact |
| 23 | 4677 | `bash_ter_furn`: `eoc_on_hit` | MAP-J01 | map_damage.cpp | exact |
| 24 | 4853 | `bash_ter_furn`: `eoc_on_destroy`; both `bash()` overloads take `basher` | MAP-J01 | map_damage.cpp | exact |
| 25 | 4964 | `bash_field`: skip removed fields | MAP-J01 review | map_damage.cpp | exact |
| 26 | 4972 | `bash_field`: field `eoc_on_destroy` | MAP-J01 | map_damage.cpp | exact |
| 27 | 6136 | `add_item` with copies | MAP-B07 | map.cpp | exact |
| 28 | 7631 | new static `field_type_lights` + `on_field_modified` | SPAT-P02 / MAP-P04 | map.cpp | exact |
| 29 | 8698 | `reachable_flood_steps` (range 0) | MONAI-B03 | map_creature.cpp | exact |
| 30 | 9059 | `reconcile_loaded_items`: skip empty tiles | MAP-P01 | map.cpp | exact |
| 31 | 9444 | `loadn`: missing submap | MAP-B08 | map.cpp | exact |
| 32 | 9473 | `loadn`: skip submaps without vehicles | MAP-P02 | map.cpp | exact |
| 33 | 9490 | `loadn`: phase change only where it can apply | MGEN-P01 | map.cpp | exact |
| 34 | 9745 | `produce_sap` season | LOOP-B06 | map.cpp | exact |
| 35 | 10624 | `build_outside_cache` allocates the level | MAP-P02 | map_architectural.cpp | exact |
| 36 | 10783 | `build_floor_cache` allocates the level | MAP-P02 | map_architectural.cpp | exact |
| 37 | 11044 | `build_map_cache`: unlit characters add no light | SPAT-P02 | map_cache.cpp | exact |
| 38 | 11511 | `creature_on_trap`: airborne creatures skip traps | grappling 5a | map.cpp | exact |
| 39 | 12034 | `get_pathfinding_cache` (lazy), `set_pathfinding_cache_dirty` ×2 | MAP-P02 | map_creature.cpp | exact |
| 40 | 12072 | `get_pathfinding_cache_ref` | MAP-P02 | map_creature.cpp | exact |

**Totals by destination:**

| Destination | Hunks | Lines |
|---|---|---|
| map.cpp | 15 | +150/−75 (these merge by themselves) |
| map_damage.cpp | 9 | +61/−8 |
| map_visibility.cpp | 4, plus parts of 2 | about +25/−8 |
| map_architectural.cpp | 5, plus parts of 2 | about +30/−14 |
| map_creature.cpp | 3 | +17/−5 |
| map_vehicle.cpp | 1 | +31/−22 |
| map_cache.cpp | 1 | +17/−9 |

In all, 33 of the 40 hunks apply exactly, 2 apply with fuzz, and 5 (the includes, `effect_airborne` and the two cross-file setter hunks) are a few lines of hand work.

**Statics and includes.** All new fork statics stay with their callers:
- `forget_decoration_if_seen` and `field_type_lights` stay in map.cpp.
- `run_bash_eoc` and `ter_furn_bash_context` go to map_damage.cpp.

No fork code uses the old file-static `has_floor_gap_flag` in a way the member version breaks.

These includes are needed:
- map_damage.cpp: `dialogue.h`, `effect_on_condition.h` and `talker.h`.
- map.cpp: `phase_change.h`. IWYU (#89625) already dropped `perf.h` from map.cpp, so #89274's include conflict there goes away.

**MAP-P02 must be re-applied as a whole.** That means all of these:
- map.cpp: hunk 5.
- map_architectural.cpp: hunks 6, 7, 9b, 10, 11b, 35 and 36.
- map_visibility.cpp: hunks 8, 9a, 11a, 12, 13 and 14.
- map_creature.cpp: hunks 39 and 40.

Mixing the two versions breaks things. With the fork's lazy constructor and upstream's `get_pathfinding_cache`, the game crashes. With the fork's "skip a level with no cache" setters and upstream's `build_outside_cache`/`build_floor_cache` ("return if there is no cache"), temporary maps are left with outside and floor caches that are never built.

**map.h and build files.**
- map.h auto-merges. The fork adds `bash_params::basher` and the `basher` parameter. Upstream changes includes and forward declarations (IWYU), adds the `has_floor_gap_flag` declaration and a TODO comment.
- Upstream dropped `rng.h`, `line.h`, `value_ptr.h` and `cata_assert.h` from map.h. They still arrive through `item.h` and `coordinates.h`. The dropped forward declarations are covered by full includes. I see no breakage for fork-added files, but I did not build.
- No build files need changes. `src/CMakeLists.txt` and `android/app/jni/CMakeLists.txt` glob `src/*.cpp`, and the Makefile uses `wildcard`. The MSVC projects that the fork's release workflow builds use wildcards: `Cataclysm-libAL` takes `a*–l*`, `Cataclysm-libMZ` the rest, so the new `map_*.cpp` files go to libMZ and `dialogue.cpp` to libAL.

**Effort.** Medium as a series, about 2–3 hours:
1. Resolve `src/map.cpp` as upstream's file.
2. `patch` the fork's map.cpp diff into the 7 files using the table above.
3. Hand-place the 5 manual hunks.
4. Build.
5. Run `audit_bash_eoc_test`, `audit_overmap_worldgen_test`, `audit_map_*`, `audit_loop_sweep_test`, `audit_perf_world_inventory_test` and the map benchmarks.

**Verdict.** TAKE-ADJUST, all six together, now.

## #89593: dialogue split (npctalk.cpp → dialogue.cpp)

**What it does.** A pure move of the dialogue core into a new `src/dialogue.cpp` (2,256 lines). It moves:
- `avatar::talk_to`.
- All `dialogue::` and `const_dialogue::` methods: values, actors, `dynamic_line`, `gen_responses`, `add_response*`, `opt_imgui`, `build_debug_info`.
- `talk_trial`, `talk_response`, the `talk_effect_t` constructors and `load_effect`/`set_effect*`/`apply`/`update_missions`.
- The `json_talk_response`/`json_talk_topic`/`dynamic_line_t` loading, and `load_talk_topic`/`unload_talk_topics`.
- `npc::pick_talk_topic`, `npc::get_specified_talk_topic`, the item whitelist, `npc::handle_sound`, `npc_trading::cash_to_favor` and `dialog_helper::bye_message`.
- The statics `json_talk_topics`, `load_inline_topic` and `translate_gendered_line`, plus 11 static ids.

It also wraps `get_talk_topic`/`get_all_talk_topic_ids` in `namespace talk_topic_all` and updates `npc.cpp` and `debug_menu.cpp` to match.

What stays in npctalk.cpp (now 7,539 lines): `game::chat`, every `talk_effect_fun_t` `f_*` EOC effect builder, the parser table and `parse_string_effect`. I compared every chunk of dialogue.cpp to the base: the only differences are includes, the namespace, and `opt_imgui`, which earlier upstream PR #89407 had already changed.

**How the fork's 94 npctalk.cpp hunks route.** +2,024/−165 lines in all. 91 of the 94 apply to upstream HEAD's files as they are (87 exact, 4 with fuzz); 3 include and static-id hunks (h02, h06, h08) are hand work.

Nine hunks (+41/−17) go to **dialogue.cpp**, and all apply exactly:

| Hunk (fork line) | Function | Finding |
|---|---|---|
| h12 (+1724) | `avatar::talk_to` (drop duplicate `get_topics` loop) | EOC-B09 |
| h13 (+1756) | new `mission_talk_topics`, `built_in_talk_topics`, `talk_topic_exists` | MGEN-B03 |
| h14 (+1842) | `dialogue::dynamic_line` uses `mission_talk_topics` | MGEN-B03 |
| h15 (+2643) | `dialogue::add_gen_response` (flags follow the response) | EOC-B10 |
| h16 (+3053) | `dialogue::opt_imgui` (bounds check) | EOC-B10 |
| h81 (+10363) | `talk_effect_t::apply` (`npc_attitude_cause::effect`) | NPC-J04 |
| h92 (+11037) | `json_talk_response::gen_responses` (failure_topic) | EOC-B10 |
| h93 (+11054) | `json_talk_response::gen_repeat_response` (argument order) | EOC-B10 |
| h94 (+11452) | `npc::pick_talk_topic` (`npc_attitude_cause::opinion`) | NPC-J04 |

The other 85 stay in **npctalk.cpp**. h25 (`f_add_wound`) needs fuzz only because of group A's wound PR. They group like this:
- **h01–h09:** includes and static ids, by hand. Union the fork's includes with upstream's trimmed list. Upstream keeps `ACT_TARGET_PRACTICE`, which the fork moved out for ACT-J02.
- **h10:** `require_character`/`require_item`/`require_faction` (EOC-B13).
- **h11:** `game::chat` "Move there!" walk (NPC-B14).
- **h17–h80:** EOC effects:
  - mutations and bionics (MUT-J01, MUT-J03), `u_pick_bodypart` (EOC-B12) and wounds.
  - `spawn_item`/`sell_item`, and the item errands `u_pickup_item`/`u_wield_item`/`u_wear_item`/`u_drop_item` (h35).
  - `consume_item_sum` (EOC-B05), `location_variable`/`copy_location` (EOC-B08) and bulk trade (EOC-B13).
  - `u_assign_activity` with targets (ACT-J02) and `u_examine` (h51).
  - `u_attack`/`u_choose_technique`/monster attacks (MONAI-J01, h54) and `set_weather` (LOOP-J01, h55).
  - `set_mod_setting` (DATA-J01, h56).
  - `u_show_text`/`run_eoc_checklist` (EOC UI, h59).
  - `weighted_list_eocs` (EOC-B04).
  - `set_attitude`/`set_mission`/faction relations (NPC-J05, h62) and grappling (h64).
  - Spawn EOCs (EOC-J01, h65–h74), overmap effects (OMAP-J01, h75) and `pickup_items` (EOC-B11, h76).
  - `u_walk_to`/`u_cancel_walk` (h77) and `trigger_event` (EVT-B10).
- **h82–h91:** parser table entries and `parse_string_effect`.

**Things that need adjusting:**
1. **`talk_topic_exists` must live in dialogue.cpp.** It reads the static `json_talk_topics`, which is now private to dialogue.cpp.
2. **npctalk.h auto-merges with `talk_topic_exists` inside `namespace talk_topic_all`.** The fork's call at `src/mapgen.cpp:3333` (MGEN-B03) is unqualified, so it fails to compile. Either qualify it, or move the declaration out of the namespace.
3. **dialogue.cpp needs `#include "npc_attitude_cause.h"`** for h81 and h94.
4. **Silent-loss risk.** In a full merge, the four big npctalk.cpp conflict blocks (672, 667, 59 and 723 lines on the fork side) hold only moved code: I checked that none of the functions in them stays in npctalk.cpp. Resolving them as "theirs" is correct only if the 9 hunks are then ported. Only the MGEN-B03 piece would show as a link error; EOC-B09, EOC-B10 and NPC-J04 would only be caught by `audit_eoc_dialogue_test` and `npc_ai_events_test`.

**Effort.** Small, about 1–1.5 hours. **Verdict:** TAKE-ADJUST.

## #89473: `item_location` in use actions

**What it does.** Every use-action entry point now takes `item_location &` (`const item_location &` for `can_use`/`can_call`) instead of `item *` or `item &`:
- all ~180 `iuse::` functions and `use_function_pointer`;
- `iuse_actor::use`/`can_use` and every actor;
- `use_function::call`/`can_call`;
- `itype::invoke`/`tick`;
- `Character`/`avatar`/`npc::invoke_item`;
- `item_action_map` (now `item_action_id → item_location`) and `map_actions_to_items`.

It also:
- moves the file-static `try_form_loc`/`form_loc` from iuse_actor.cpp to `item_location::try_form_loc`/`form_loc( Character *, … )`;
- adds `item::get_usable_item_loc`, `Character::inv_dump_loc`, `get_pseudo_items_locs` and `outfit::worn_locations`/`inv_dump_locs`;
- changes the return types of `npc::get_healing_item` and `find_firestarter_with_charges` to `item_location`.

**Dependencies.** It builds on #89509: it uses `is_worn_or_wielded` and `is_worn( *it )` in `stimpack`/`radglove`, so take #89509 first. It also edits `repair_item_actor::use`, which #89451 rewrote.

**Textual conflicts.** At this PR, with all earlier upstream PRs taken, there are 5 blocks:
- **iuse.cpp: `iuse::lumber`'s signature (USE-B06).** Keep the fork's `cut_log_into_planks` wrapper and use the new signature.
- **iuse_actor.cpp: `holster_actor::use` (USE-B01).** Keep the fork's unconditional `ret--` over upstream's `opts.size() != it->num_item_stacks()`, which is the bug itself.
- **iuse_actor.cpp: three blocks in `effect_on_conditions_actor::use` (USE-B08, USE-J01).**

**Fork code that stops compiling.** This is `src/`:
- **`src/iuse_actor.h:1290`.** The fork-added `effect_on_conditions_actor::can_use( const Character &, const item &, map *, const tripoint_bub_ms & ) const override` no longer overrides anything. Make it `const item_location &`.
- **`src/iuse_actor.cpp:6025-6052`, the body of that `can_use` (USE-J01).** It uses `p.is_worn( it )`, `it.tname()`, `const_cast<item &>( it )` and the static `try_form_loc( Character &, … )` that #89473 deletes. With the location passed in, the whole lookup goes: build the talker from `it`.
- **`src/iuse_actor.cpp:6125-6180`, `effect_on_conditions_actor::use`.** It uses `it.tname()`, `p->is_worn( it )`, `it.typeId()`, `it.type->comestible`, `p->has_item( it )`, `try_form_loc( *p, here, pos, it )` and `item_location( *p, &it )`.
  - USE-B08's "find the item where it is" becomes `loc = it;`.
  - Upstream's version still rebuilds a character-relative location (`item_location( *p->as_character(), &*it )`) and still dereferences `p` unguarded. USE-B08 is not fixed upstream, so keep the fork's logic.
- **`src/iuse_actor.cpp:~2972`.** The `debugmsg( …, it.tname() )` the fork added in `holster_actor::use` must become `it->tname()`.
- **`src/iuse_actor.cpp:4668`, `install_bionic_actor::can_use` (MUT-B08).** `p.is_installable( &it, false )` must become `it.get_item()`.

This is `tests/`:
- **`audit_item_use_actions_test.cpp`:**
  - :111 `iuse::lumber( &they, &saw, pos )`.
  - :208 `actor->use( &they, rune, &here, pos )`. `rune` is a map item, so build `item_location( map_cursor( &here, pos ), &rune )`.
  - :232 `lowered->invoke( &they, blindfold, … )`, the USE-B09 test, on a worn item.
  - :285 `iuse::fishing_rod( &they, &rod, … )`.
- **`audit_item_use_test.cpp`:** :43 and :58 `iuse::papr_blower( nullptr, &blower, pos )`. The blower is a local item, so it needs to be placed on the map to get a location.
- **`audit_items_terrain_test.cpp`:** :199 `iuse::break_stick( &u, &stick, … )`, :211 `u.invoke_item( stick.get_item(), "BREAK_STICK", … )` and :225 `iuse::break_stick( &u, stick.get_item(), … )`.
- **`audit_mod_validation_test.cpp`:** :214, 224, 231, 243, 270 and 300 `invoke_item( rune.get_item(), … )` must pass `rune`. :240 `use->can_call( u, *rune, target )` must pass `rune`.
- **`audit_mutations_bionics_sweep_test.cpp`:** :294 and :297 `install->can_call( you, cbm, … )` with `const item cbm`. Put the CBM in the inventory to get a location.

The new parameters are non-const `item_location &`, so the tests need named `item_location` variables, not temporaries.

**Fork code that merges and still compiles.** `it->` works on `item_location`. I checked every fork hunk:
- iuse.cpp: `good_fishing_spot`/`fishing_rod`/`fish_trap` (USE-B10), `papr_blower` null carrier (USE-B02), `hotplate` (USE-B07; `heat_solid_items` now takes `item_location &`, which matches), `radiocar` (USE-B05), `capture_monster_act` grapple release and `break_stick` (USE-B03).
- iuse_actor.cpp: `salvage_results` (USE-B11) and `heal_actor::get_stopbleed_level` (USE-B04).
- The fork's `pre_obtain_moves` fix in `Character::invoke_item` (USE-J01 review) lands in the new `item_location` overload.

No fork-added code anywhere else in `src/` calls `invoke_item`, `itype::invoke`/`tick`, `use_function::call`/`can_call`, `map_actions_to_items`, `get_healing_item`, `find_firestarter_with_charges` or `npc_start_fire`, or defines a new `iuse_actor` or `iuse` function. I grepped all 64k fork-added lines.

So the only USE fixes that need real work are USE-B01, B06, B08, J01 and MUT-B08. The rest only need their tests updated.

**Problems in the PR itself:**
- **15 player-facing strings in `iuse_actor.cpp` were broken by a search-and-replace of `it.` to `it->`.** For example, "You need to wear the %1$s before activating it->", "…to play it->" and "This CBM is not sterile, you can't install it->". There were none before this PR. This also invalidates their translations. Fix them while merging.
- **`item_location::form_loc( Character *you, … )` dereferences `*you` in its fallback.** That fallback only runs when the item is not found. Callers in `item.cpp:4835` and `item_gun_tool_ammo.cpp:4367/4412` pass a possibly null `carrier`. Rare, but it is a null dereference.
- **Good for USE-P08.** `can_use` now has the location, which is the "proper fix" USE-P08 asks for. But `iuse_transform::can_use` still calls `transform_fits_parent_pocket( p, *it, … )`, which rebuilds the location. Passing `it` there would now be a small follow-up.

**Effort.** Medium, about 3–5 hours. **Verdict:** TAKE-ADJUST. Skipping it is not realistic: iuse.cpp and iuse_actor.cpp got 5 upstream PRs in these 3 weeks, and every later use-action PR will be written against these signatures.

## #89451: `repair_item_activity_actor`

**What it does.** It turns the last legacy finish-function activity, ACT_REPAIR_ITEM, into `repair_item_activity_actor`. The actor stores the repaired item, the tool, an optional vehicle tool, the stage and `no_menu`, and serializes them. Old saves map the legacy type to null. The PR also:
- removes `activity_type::call_finish`, `activity_handlers::finish_functions` and the ACT_REPAIR_ITEM "evil hack" in `vehicle::use_vehicle_tool`;
- adds a test-only helper, `Character::process_activity_until_invalid`.

**How it meets fork work:**
- **The textual conflict is in `activity_type.cpp`.** ACT-J01 (cbbf71f0) rewrote the file to use generic_factory (`id_` became `id`) and touched the `finish_functions` loop and `call_finish`. Upstream deletes both. Resolve by deleting both. `activity_type.h` and `player_activity.cpp` auto-merge correctly: `do_turn` keeps the fork's end hooks around `actor->finish` / `set_to_null()`.
- **ACT-J01/J03 hooks lose their beta talker.** The fork's `start`/`completion`/`cancel`/`do_turn` EOC hooks use `activity_actor::hook_target()`, which defaults to `act.targets.front()`. The old ACT_REPAIR_ITEM put the tool in `act.targets`. The new actor keeps its items in members, so the beta talker becomes empty. No JSON in the repo defines hooks for ACT_REPAIR_ITEM. To keep it consistent with doc/PLAYER_ACTIVITY.md ("the item the activity works on"), add `item_location hook_target( const player_activity & ) const override { return repaired_item; }`. It is a 3-line change; the fork already has a similar override at `activity_actor_definitions.h:2531`.
- **`u_assign_activity` (ACT-J02).** Since ACT_REPAIR_ITEM is now actor-backed and not an EOC starter, a literal `"ACT_REPAIR_ITEM"` is now rejected at load. Before, it started a broken legacy activity with no targets. This is an improvement, and nothing in the repo uses it.
- **VINT-J09** (vehicle repair tools hardcoded to 6 ids) is not fixed. The id list just moves from `vehicle_use.cpp` to `repair_item_actor::use` (`welder`, `welder_crude`, `welding_kit`, `soldering_iron`, `small_repairkit`, `large_repairkit`, plus PSEUDO and on a vehicle). If the fork fixes VINT-J09 later, that is where the fix goes.
- **Smaller notes.** `enum_to_string<repair_stage>`'s fatal message says "Invalid based_on_type", a copy-paste slip. `activity_actor_definitions.h` now includes `iuse_actor.h`, which slows compiles a little.

**Effort.** Small, about 30 minutes. **Verdict:** TAKE-ADJUST.

## #89509: remove `int`-indexed item accessors

It removes these and moves all callers to `item_location`:
- `Character::i_at(int)`, `get_item_position`, `use(int)`, `wear(int, bool)` and `takeoff(int)`;
- `outfit::i_at`/`get_item_position`.

`longsalvage_activity_actor` now saves its tool as an `item_location`; an old `index` member is read and ignored. It adds `is_worn_or_wielded`.

The standalone merge is clean. I grepped the fork's added code (src and tests) for `i_at(` on characters, `get_item_position`, `worn_position_to_index`, and `wear`/`takeoff`/`use` with an int: no hits. Every `i_at` the fork added is `map::i_at`.

**Verdict.** TAKE, trivial. #89473 needs it.

## #89339: C++20 `char8_t`

It adds a third-party header, `src/third-party/char8_t-remediation`, plus `from_u8string`, a `U8` macro, and fmt and printf converters for `std::filesystem::path` and `std::u8string`. Callers are wrapped so `fs::path::generic_u8string()` works under both C++17 and C++20.

Upstream is still C++17 at HEAD (Makefile, CMake and MSVC `stdcpp17`), and so is the fork. The header resolves through the existing `src/third-party` include path in the Makefile (`-isystem`), the CMake `third-party` target, and MSVC `Cataclysm-common.props`, including the fork's Windows release build.

The fork adds no `u8"…"` literals and no `char8_t` code. For a later C++20 move only, these fork-added calls return `std::u8string` under C++20 and would need `from_u8string()`:
- `src/game_io.cpp:662, 669, 687, 694, 725` (`std::filesystem::path final_path` / `staged`);
- `src/mapbuffer.cpp:99, 107, 362, 444, 454`;
- `src/mod_setting.cpp:773`;
- `src/worldfactory.cpp:2414`;
- `tests/audit_data_sweep_test.cpp:304`.

`std::filesystem::u8path` (deprecated in C++20) appears at `tests/audit_data_sweep_test.cpp:406` and `tests/audit_perf_load_test.cpp:735`.

**Verdict.** TAKE, trivial.

## #89274: performance metrics report

It extends `cata_timer` (perf.h): it switches to `steady_clock`, adds "wait" timers whose time is subtracted from their parents, and prints self time and percentages. It then wraps about 40 steps of `game::do_turn` plus some hot functions in timers:
- `process_npc_turn`, `process_sound_markers`, `mon_info_update`, `cleanup_dead`;
- `handle_action`, `get_player_input`, `handle_input` (a wait timer);
- `move_hordes` (per overmap), `process_falling`, `process_explosions`;
- `process_activity` in the test helpers.

It prints the tree to debug.log at the end of each game in `run_game_loop`. The timers are always on.

**How it meets fork work:**
- **`do_turn.cpp` conflict.** Upstream wraps `handle_weather_effects` in a timer inside the old `levz >= 0 && !u.is_underwater()` gate. The fork removed that gate for LOOP-B15. Keep the fork's ungated call, with a timer if wanted. The fork's other `do_turn.cpp` change, the motion alarm in `monmove` (moved out of the anonymous namespace), merges cleanly.
- **`map.cpp` conflict.** It is only `#include "perf.h"` next to the fork's `phase_change.h`. After the map split, IWYU removes `perf.h` from map.cpp anyway.
- **The fork's perf work.** The fork measures with `tools/benchmark/run_benchmarks.py` scenarios, not `cata_timer`, so nothing clashes. The overhead is a few dozen map lookups and clock reads per turn, plus `string_format` calls that run per NPC, per overmap and per activity ("Processing sounds for %s", "move hordes at %d/%d", "do turn for %s"). That is microseconds against a turn that costs milliseconds, so it is not measurable in the fork's benchmarks.
- **The wrapped activity loop.** `do_turn` now wraps `u.process_activity()` in `while( moves > 0 && activity )`. `process_activity` already loops the same way, so behaviour is unchanged.
- **A possible crash at game exit.** In `print_stats_recursively`, `self_duration * 100 / adj_duration` divides by zero if a node with children spent no non-wait time. That is an unlikely edge case.

**Verdict.** TAKE-ADJUST, trivial.

---

## Group summary

**Verdicts:** TAKE 2 (#89339, #89509), TAKE-ADJUST 10 (#89274, #89451, #89593, #89473 and the six map split PRs), OWNER 0, SKIP 0.

**Series and dependencies:**
- **Map split:** #89612 → #89625 → #89646 → #89660 → #89666 → #89674. Take all six together, after #89274.
- **Item location:** #89451 → #89509 → #89473. #89473 uses #89509's `is_worn_or_wielded` and edits code #89451 rewrote.
- **#89593** stands alone. #89473's npctalk.cpp change (`f_activate` passing `*it`) is in code that stays in npctalk.cpp.
- **Order to take them in** (upstream merge order): #89339, #89274, #89451, #89509, #89593, #89473, then the six map split PRs.

**Riskiest items:**
1. **The map split's silent partial merge of MAP-P02.** The lazy `map::map()` merges into map.cpp, while upstream's `get_pathfinding_cache` in map_creature.cpp still dereferences with no allocation, so the game crashes on the first submap load (`loadn` calls `set_pathfinding_cache_dirty`). Re-apply the MAP-P02 hunks across all four files as one unit.
2. **#89473's rewrite of the fork's EOC use action (USE-B08, USE-J01).** It is the most intricate fork code in the area, and the tests that guard it (`audit_mod_validation_test`, `audit_item_use_actions_test`) must be edited at the same time. Fix the 15 "it->" strings too.
3. **#89593: silent loss of EOC-B09, EOC-B10 and NPC-J04** if the npctalk.cpp conflict blocks are resolved as "theirs" without porting the 9 hunks. Only MGEN-B03 fails at link time.

**Recommendation: take all 12 now.** Each refactor is cheap to re-apply today, because upstream moved the code without changing it. Most fork hunks apply verbatim to the new file with `patch`, so this is routing, not rewriting.

Skipping them is expensive. Git cannot follow function moves, so every later upstream change to the ~6,000 lines now in `map_*.cpp`, the ~2,250 in `dialogue.cpp`, or any use action would have to be ported into the fork's old files by hand, one function at a time. The split author is also still splitting files.

| Refactor | Cost to take now | Cost of skipping |
|---|---|---|
| Map split (6 PRs) | about 2–3 h: route 25 moved hunks (+180/−66), 5 by hand, build, run the map audit tests and benchmarks | every upstream PR touching vehicles, caches, bash, visibility, architecture or pathfinding becomes a manual port |
| Dialogue split (#89593) | about 1–1.5 h: 9 hunks (+41/−17), the `talk_topic_exists` namespace fix, one include | every dialogue or talk-topic PR becomes a manual port. Low cost per PR, but recurring. |
| #89473 item_location | about 3–5 h: 6 source spots, 18 test call sites in 5 files, 15 strings | every later use-action PR fails to compile against the fork. The highest recurring cost. |
| #89451 | about 0.5 h: delete 2 conflict blocks, add `hook_target` | later activity and repair PRs conflict |
| #89509, #89339 | about 0 | later item-access and path PRs fail to compile |
| #89274 | about 0.25 h: one conflict block | later timer additions conflict in `do_turn.cpp` |

The total is roughly one working day (7–11 hours) including build and test runs.
