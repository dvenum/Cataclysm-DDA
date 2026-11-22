# Design: one set of override rules for every JSON type (DATA-J02)

**Status:** decided with the owner on 2026-11-21 (see "Decisions" at the end); not built yet. Built on a survey of all 190 types registered in `src/init.cpp`, taken on 2026-11-21 after the upstream sync. The per-type tables are in [data-j02-survey/](data-j02-survey/) (`part_00.md` to `part_02.md`), with file:line for every gap.

## The problem

Whether a mod can change something depends on how that type happens to load:

| | Types | What a mod gets |
|---|---|---|
| Fully fine | 70 | `generic_factory`, every member read with `was_loaded`: `copy-from`, `abstract`, `extend`/`delete`, `relative`/`proportional`, last definition wins |
| Partial | 51 | `generic_factory`, but some members reset on `copy-from` or only append, or lists of objects can't be edited |
| Custom | 68 | Own loader: first definition wins, redefinition appends, the object is rebuilt from scratch, duplicates are kept, or there is no `copy-from` at all. About 15 of these have no ids (dreams, speech, help, blacklists) and are fine as accumulators. |

`doc/JSON/JSON_INHERITANCE.md` calls its list of supported types "non-exhaustive", so a modder can't tell any of this without reading C++.

The fork already fixed some of the audit's examples:
- `npc_template` now lets the last definition win (NPC-J02).
- Factions get `copy-from` and lose duplicates.
- `behavior` and `activity_type` moved onto `generic_factory`.
- `effect_type` now behaves like it.

### What is silently broken in shipped data today

These are found by the survey, and are all fixed by this work or alongside it.

**Mods:**
- **Monster groups lose their flags when a mod adds to them.** 24 mod extensions of vanilla monster groups drop `is_animal` or `is_safe`, because every extension resets those fields. The mods are MindOverMatter, DinoMod, CrazyCataclysm, XedraWood, Aftershock and No_Hope. Animal spawns then follow the wrong density, and safe groups stop counting as safe.
- **A MindOverMatter override doubles a trait's enchantment.** `INTERSTICE_RESONANCE_2` and `_3` end up with their inline enchantment twice.
- **BombasticPerks replaces core weak points.** `wps_humanoid_body` replaces the core humanoid weak points with a single groin entry for every humanoid.
- **Aftershock rolls 64 trait groups twice.** They are defined in two files and appended twice, so a 50% trait rolls at about 75%.
- **Aftershock's palette `lab_loot_research` is defined twice in one mod.** One copy is silently dropped.
- **Recipe redefinitions still need the vanilla tools they leave out,** because an inline requirement list that isn't restated keeps its old contents. Examples:
  - innawood: `mortar_build`, `brew_dandelion_wine`, `well_pump`
  - Generic_Guns: `slamfire_shotgun_d_welded_together`
  - Xedra_Evolved: `bread_flour`
- **No_Hope's terrain `connects_to` extensions go to the wrong field.** This is a loader bug in terrain and furniture `extend`/`delete`.
- **Loading `classic_zombies` or `MindOverMatterNoKnacks` brings back an empty "Fortified" scenario,** because each blacklist replaces the previous one, core's included.
- **alt_map_key copies gameplay fields onto 51 map tiles** that never had them, through its id-array entries.

**Core data:**
- **Three Robofac intercom NPC classes lose the stat bonuses they copy from their base.** `copy-from` resets `bonus_*`.
- **Faults that copy from another fault stack the parent's damage multipliers.** For example, a bent baton hits for ×0.56 instead of ×0.7.
- **Three item groups are defined twice with different contents.** One definition of each is lost: `nested_p226_9mm`, `boxed_410` and `airgun_hunting`.
- **`charging_station_2x2` is defined three times,** so its roofed variant never exists.
- **A duplicated item migration means one pair of gloves never migrates.**

## The rules this design proposes

**Every type with ids follows the same rules:**
1. **A redefinition replaces.** The same id again, without `copy-from`, is a new object: fields not given take their defaults.
2. **`copy-from` inherits.** Every field not given is inherited from the source, including copying from itself to change a few fields of a vanilla object. `abstract` works everywhere.
3. **The last definition wins** across mods, in load order.
4. **A duplicate within one mod is a load error.** It is always a data mistake.
5. **`extend` and `delete` work on every list member**, and `relative` and `proportional` on every number, as `generic_factory` already does for the types that use it.
6. **Lists of objects can be edited by key.** A mod adds, changes or removes one entry without restating the list.

**Collections are the exception.** Some types are contributions rather than objects:
- talk topic responses
- monster group entries
- item group entries
- trait groups
- vehicle groups
- the mapgen alternatives for a map tile

A redefinition of one of these appends today, and hundreds of shipped definitions rely on that: about 426 talk_topic definitions alone. Decision 1 below is whether that stays.

**Id-less types stay as they are,** since they accumulate by design: dreams, speech, help. Two exceptions merge instead of replacing: the scenario and profession blacklists. Today each one replaces the previous mod's, which is a bug.

## How it is built

**The foundation:**
- **A registry of types next to `init.cpp`'s `add()` calls.** For each type it records the override rules: object or collection, and what can be keyed. `JSON_INHERITANCE.md`'s support table is generated from it, so the docs can't drift from the code.
- **A round-trip test.** For every type, a field-less `copy-from` must reproduce its parent exactly, by serializing or comparing every member. This is what finds the members that silently reset. It runs over all vanilla and in-repo mod data.
- **A same-mod duplicate check** for every type, including the `generic_factory` types that have no `src` member today (fault, fault_fix, addiction, the shopkeeper types and others).

**Per type, in order of how much it matters to modders:**
1. **Move the custom object types onto `generic_factory`:**
   - `vitamin`: any redefinition is a hard error today
   - `skill`, `emit`, `mutation_category`: rebuilt from scratch today
   - `monster_attack`: rebuilt when `attack_type` is given
   - `palette`
   - `item_group`: copy-from only to the same id today, with no delete
   - `requirement`: no copy-from or delete, and a positional `extend` that drops groups
   - the migration types: first wins and never cleared between worlds today, so one world's mod migrations leak into the next
2. **Fix the partial types' members** that reset or only append. The round-trip test lists them:
   - `mutation`'s ~25 hand-rolled members
   - `npc_class` bonuses
   - `character_mod`
   - `technique` requirements
   - `ITEM` slot fields when `subtypes` is omitted
   - `SPELL` flags, which can't be removed
   - `effect_on_condition`'s `effect`, which appends
   - fault multipliers
   - `weather_type` and `end_screen` conditions
   - enchantment conditions and value maps
3. **Keyed editing of lists of objects.** The keys:
   - monster group entries by monster or group id
   - item group entries by item or group id
   - vehicle parts by position and part
   - `pocket_data` by pocket type
   - talk responses: see decision 3
   - mutation enchantments, EOC lists and attacks by id
   - requirement components by item
4. **Collections:**
   - add `delete` and `replace` operations to the collection types, following decision 1
   - give mapgen a way to replace or remove earlier mapgen for a map tile (decision 4). Mods use weights of 100000 up to 1e9 for this today.

**Shipped data.** Every item listed under "What is silently broken" is fixed in the data or by the loader change. The round-trip test and the duplicate check guard them. Some data relied on the old behaviour and needs restating so it keeps working. For example, 20 core appliances rely on a vehicle part's `requirements.removal` resetting inherited skills and tools.

**Saves.** No type's ids change, so saves are unaffected. Loading follows the same order, and each type keeps its `finalize` and `check_consistency`; `talk_topic`'s consistency check gets registered (DATA-J03).

## Phases

| Phase | Content | Tests |
|---|---|---|
| 1 | Registry, generated support table, round-trip test, same-mod duplicate check; fix the shipped data the checks flag | The round-trip test over all core and mod data; the duplicate check |
| 2 | Partial types' members honour `copy-from` | Round-trip clean; a test per fixed member |
| 3 | Custom object types onto `generic_factory`; migrations last-wins and reset; blacklists merge | Override, `copy-from`, `extend`/`delete` and duplicate tests per type; every mod set loads |
| 4 | Keyed list editing; collection `delete`/`replace`; mapgen replacement | Per list: add, change, remove one entry; per collection: delete, replace |
| 5 | Docs: `JSON_INHERITANCE.md` rewritten around the rules and the generated table | |

This is about the size of the upstream sync. Phases 1 and 2 are mostly mechanical once the round-trip test exists. Phase 3 is per-type work, and phase 4 needs a small amount of shared code in `generic_factory` that every keyed list then uses.

## Decisions (2026-11-21)

The owner took all four proposals:
1. **Collections keep appending.** Talk topics, monster groups, item groups, trait groups, vehicle groups and mapgen alternatives still add on redefinition, and gain `delete` and `replace` operations.
2. **A duplicate id within one mod is a load error.** The shipped duplicates are fixed as part of the work.
3. **List entries are named by natural keys,** plus an optional `"id"` any entry can be given. Vanilla talk responses without an id can be matched by their exact text.
4. **Mapgen gets `"replaces": true`,** which supersedes every earlier mapgen for its map tiles. Later mods can still add alternatives.

## The questions as they were put

1. **Collections that append on redefinition today** (talk topics, monster groups, item groups, trait groups, vehicle groups, mapgen alternatives).
   - Proposed: they stay collections. A redefinition keeps adding, and `delete` and `replace` are added for when a mod wants to remove or replace.
   - The alternative: they follow the object rule (a redefinition replaces), with the in-repo mods converted. Third-party mods that add to vanilla groups or topics would then silently replace them instead.
2. **A duplicate id within one mod.**
   - Proposed: a load error, as `generic_factory` already does for some types. The shipped duplicates are fixed as part of this.
   - The alternative: a warning.
3. **How to name an entry in a list that has no id.** This applies to talk responses, and to item group and monster group entries that don't name a single item or monster.
   - Proposed: natural keys where they exist (item id, monster id, pocket type, part and position), plus an optional `"id"` field a mod can give any entry so it can be addressed later. A talk response without an id can be matched by its exact text, as a fallback for vanilla responses.
   - The alternative: only "replace the whole list".
4. **Mapgen.**
   - Proposed: a mapgen can say `"replaces": true`, so it supersedes every earlier mapgen for its map tiles. Later mods can still add alternatives after it.
   - The alternative: give mapgens ids so they can be deleted by id. That works for removal, but vanilla mapgens would need ids first.
