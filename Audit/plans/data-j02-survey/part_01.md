# DATA-J02 survey, part 01 (61 types)

Tree: `/home/user/nickclysm`, branch `claude/sweet-cannon-ilgctf`. I only read code; nothing was built or run. Mod-override counts come from a script that matched each mod object's id against `data/json` (`overrides` = mod objects whose id exists in core).

GF = `generic_factory`. Unless a row says otherwise, GF semantics are:
- a duplicate id from another mod replaces the object (last wins);
- a duplicate id from the same source throws `mod_error`, but only when the type has a `src` member;
- a redefinition without `copy-from` rebuilds the object from scratch;
- `copy-from` plus `optional()` keeps the fields an override leaves out;
- a deferred `copy-from` no longer overwrites later mods (the DATA-B03 fix in `init.cpp:307`).

"was_loaded ok" means every member goes through `optional()`/`mandatory()` with `was_loaded`. "ext/del on readers" means extend/delete work on the members read with a `generic_typed_reader`; plain `optional()` on a container throws if `extend` or `delete` names it.

## Counts

- **Fully fine (GF, no notable gaps): 22.** vehicle_part_location, ITEM_CATEGORY, SPECIES, monster_flag, LOOT_ZONE, recipe_category, weapon_category, climbing_aid, attack_vector, oter_vision, construction_category, construction_group, overmap_land_use_code, city, overmap_special_migration, and the 7 region_settings_* types.
- **Partial (GF-backed, with gaps): 17.** vehicle_part, vehicle, trap, ITEM, MONSTER, tool_quality, technique, martial_art, recipe_group, overmap_terrain, construction, overmap_connection, overmap_location, overmap_special, city_building, map_extra, omt_placeholder.
- **Custom loaders: 22.** item_action, vehicle_part_category, vehicle_part_migration, vehicle_group, vehicle_placement, vehicle_spawn, requirement, trap_migration, MIGRATION, charge_removal_blacklist, temperature_removal_blacklist, test_data, monster_adjustment, recipe, uncraft, practice, nested_category, effect_type, effect_migration, oter_id_migration, camp_migration, mapgen.
  - Of these, recipe/uncraft/practice/nested_category (`recipe_dictionary`) and effect_type (since HLTH-J02) behave like GF: copy-from, abstract, last wins, same-source error.

## Table

| type | loader | dup id | copy-from | re-load honoured | extend/delete | object lists | mods overriding (broken?) | saved? | effort |
|---|---|---|---|---|---|---|---|---|---|
| item_action | custom `std::map` `item_actions` | last wins, no error | no | n/a (only id, name) | no | none | none | no | trivial |
| vehicle_part | GF `vpart_info_factory` (+`handle_inheritance` for looks_like) | GF | yes | mostly. Not: `requirements.*` sub-objects (skills reset, lists rebuilt), `control_requirements`, `workbench`/`transform_terrain` sub-fields | ext/del on readers (flags, categories, qualities, emissions, allowed_tools...) | `enchantments` append-only. `variants`, `pseudo_tools`, `breaks_into`, `damage_reduction` replace-only | 3 (aftershock 2, classic_zombies 1), OK. **20 core appliances rely on the sub-object reset** | yes (part id+variant; vpart_migration) | small (risky: see notes) |
| vehicle_part_category | custom `std::vector` | both kept (duplicate tab) | no | rebuild | no | none | none | no | trivial |
| vehicle_part_location | GF `vehicle_part_location_factory` | GF | yes | ok | n/a | none | none | no | none |
| vehicle_part_migration | custom `std::map::emplace` | **first wins** | no | n/a | no | none | none (7 core) | applied on load | trivial |
| vehicle | GF `vehicle_prototype_factory` | GF | yes (121 core uses) | `name`/`items`/`zones` ok; **`parts` always appended** | zones only | **parts append-only** (no remove/move). `items` replace-only (extend throws) | 39 (classic_zombies 31, No_Hope 7...). Work; suv_electric_rack relies on append | yes (`type` vproto_id) | medium |
| vehicle_group | custom `unordered_map` `vgroups`, merge | **merged (entries appended)** | no | n/a | no | **append-only weighted list** | 25 blocks (aftershock 14, Magiclysm 8, XE 3), intended additive. aftershock defines `city_vehicles` twice in one file | no | small–medium (keep append default) |
| vehicle_placement | custom map, merge | merged | no | n/a | no | locations append-only | none | no | small |
| vehicle_spawn | custom map, merge | merged | no | n/a | no | spawn_types append-only | No_Hope `default_bridge`/`default_parkinglot`: append, works only through huge weights | no | small |
| requirement | custom `std::map requirements_all`, merge | **merge**: each non-empty list replaces, empty keeps old; no error | **no** (copy-from unvisited) | partial (a list can't be cleared) | **custom positional extend** (extend group i only); no delete | groups only extended in place | 74 (Magiclysm, XE, aftershock...). All extends within group count, OK | no | medium |
| trap | GF `trap_factory` | GF | yes | mostly. DATA-J04 mandatory difficulty/etc. | ext/del: none of its containers use readers (`flags`, `drops` plain) | `eocs` append-only | none | yes (trap ids; trap_migration) | small |
| trap_migration | custom `unordered_map::insert` | **first wins** | no | n/a | no | none | none (1 core) | applied on load | trivial |
| ITEM | GF `Item_factory::item_factory` (subtype slots) | GF | yes | mostly. `degradation_multiplier` read with `false`; slot fields need `subtypes` restated | ext/del on readers (flags, qualities, material, use_action, pocket_data...) | `faults`/`fault_groups` append-only | 321 (XE 93, aftershock 61...). DATA-B03 fixed | yes | small |
| ITEM_CATEGORY | GF `item_category_factory` | GF | yes | ok (spawn_rate fixed, ITEM-B09) | n/a | `priority_zones` replace-only | none | spawn rates per world | none |
| MIGRATION | custom `map<id, vector<migration>>` | non-variant last wins; from_variant both kept; no error | no | rebuild (small record) | no | n/a | 61 (Generic_Guns 53, aftershock 17), work. **Core has 4 conflicting dups** (see notes) | applied on load | small (dup warning) |
| charge_removal_blacklist | custom `std::set`, id-less union | accumulates | n/a | n/a | no delete | n/a | Magiclysm adds 2 | affects save load | trivial (add reset) |
| temperature_removal_blacklist | same | accumulates | n/a | n/a | no | n/a | none | affects save load | trivial |
| test_data | custom singleton sets | accumulates | n/a | n/a | no | pulp data append | TEST_DATA only | no | n/a |
| MONSTER | GF `mon_templates` | GF | yes | mostly. Hand-read `special_when_hit`, `baby_flags`, `reproduction`, `death_drops` replace whole | ext/del on readers (flags, special_attacks, weakpoints...) | `special_when_hit` single; `baby_flags` replace-only | 597 (Magiclysm 311, XE 220...), mostly copy-from | yes | small |
| SPECIES | GF `mon_species` | GF | yes | ok | ext/del flags, triggers | none | 14 full redefinitions. **Defense_Mode×MoM `HUMAN` drops bleeds+description; classic_zombies `ZOMBIE` drops VENOM_IMMUNE** (author error, not loader) | no | none |
| monster_flag | GF `mon_flags` | GF | yes | ok (only id) | n/a | none | 0 core overrides; 3 cross-mod | no | none |
| LOOT_ZONE | GF `zone_type_factory` | GF | yes | ok | n/a | none | none | yes (zone type ids) | none |
| monster_adjustment | custom vector, id-less | accumulates | n/a | n/a | no | rule list append-only | XedraWood, classic_zombies | no | small (needs ids to remove) |
| recipe_category | GF `craft_cat_list` | GF | yes | ok | ext/del subcategories | none | 3 copy-from+extend, OK | no | none |
| recipe | custom `recipe_dictionary` `std::map` (GF-like) | last wins; same source throws | yes (own deferral) | partial: inline `components/tools/qualities` not inherited (debugmsg); `name` (practice/nested) and `container_variant` read with `false` | ext/del on readers (proficiencies, using, flags...); **throws for components/tools/qualities** | inline lists replace-only; `blueprint_provides/requires/excludes` append | 634 (innawood 412, Generic_Guns 165...). **5 full redefinitions silently keep vanilla tools/qualities** | yes (known recipes, crafts) | medium |
| uncraft | same dictionary, `uncraft` map | same | yes | same | same | same | 17, none stale | yes (disassembly) | medium (with recipe) |
| practice | same dictionary, shares `recipes` map | same | yes | `name` must be restated | same | same | innawood 3 | yes | small |
| nested_category | same dictionary, shares `recipes` map | same | yes | `name` must be restated | `nested_category_data` plain, replace-only | same | none | no | small |
| recipe_group | GF `recipe_groups_data` | GF | yes, but load ignores was_loaded | **no**: `building_type` mandatory, `recipes`/`om_terrains` append | no | append-only | classic_zombies 2 full redefinitions, OK | no | small |
| tool_quality | GF `quality_factory` | GF | yes | `usages` append on copy-from | no | usages append-only | none | no | trivial |
| technique | GF `ma_techniques` | GF | yes but **requirements reset** | **no** for `reqs.*` (`ma_requirements::was_loaded` never set); bonuses append | ext/del on readers (flags, tech_effects, skill reqs...) | `eocs`/`post_hit_eocs`, `flat/mult_bonuses` append | none override (latent) | no | small |
| weapon_category | GF | GF | yes | ok | proficiencies plain | none | none | no | none |
| martial_art | GF `martialarts` | GF | yes | mostly | ext/del on readers (techniques, weapons, *_buffs...) | `autolearn`, 14×`*_eocs` append; inline buffs register in `ma_buffs` with src "" | 8 (aftershock 7 copy-from+extend OK; Perk_melee `style_none` drops `teachable:false`) | yes (`martial_arts_data`) | small |
| climbing_aid | GF | GF | yes | ok | n/a | none | none | no | none |
| attack_vector | GF `attack_vector_factory` | GF (no src → no same-source check) | yes | ok | plain vectors/sets: no | none | Limb_WIP 1 identical redefinition | no | none |
| effect_type | custom `std::map effect_types` (GF-like since HLTH-J02) | last wins; same mod throws | yes + abstract, own deferral | yes | on id lists + enchantments; hand-read members refuse | `apply_message` replaced; base/scaling mods per stat | 16 (MoM now copy-from+extend) OK | yes (effects; effect_migration) | medium to move to GF; no need |
| effect_migration | custom `std::map::emplace` | **first wins** | no | n/a | no | none | none | applied on load | trivial |
| oter_id_migration | custom map `insert_or_assign`; `A→A` deletes | last wins (by design) | n/a | n/a | self-map = delete | n/a | aftershock/Magiclysm/innawood/TropiCataclysm; 8 core dups harmless | applied on load | none |
| camp_migration | custom `std::map::emplace`, id-less | **first wins** | no | n/a | no | n/a | none | applied on load | trivial |
| overmap_terrain | GF `terrain_types` (`oter_type_t`), finalize builds `terrains` (`oter_t`) | GF | yes | mostly. `uniform_terrain` read with `false`; copy-from prepends looks_like | ext/del flags, connect_group, pp generators | embedded `mapgen` arrays append to mapgen registry | 4898 (alt_map_key 4899 objs). **alt_map_key: 51 sibling OMTs take another OMT's gameplay fields** | yes (oter ids; oter_id_migration) | small |
| oter_vision | GF | GF | yes | ok | `levels` plain | levels replace-only | alt_map_key 30 full redefinitions, OK | no | none |
| construction_category | GF | GF | yes | ok | n/a | none | none | no | none |
| construction_group | GF | GF | yes | ok | n/a | none | 1 cross-mod | no | none |
| construction | GF `construction_factory` (no src) | last wins, no same-source check | yes | partial: inline reqs not inherited (TERR-J06); `using` array appended | ext/del pre_terrain/post_flags/pre_flags | `using` append; eocs replace | none (82 mod-only) | yes (partial constructions by id) | small–medium |
| mapgen | custom: `oter_mapgen` / `nested_mapgens` / `update_mapgens` weighted lists | **all kept as weighted alternatives** | no | n/a | no | n/a | 116 overlapping ids. Mods "replace" with weight 1e5–1e9 | no | large |
| overmap_land_use_code | GF | GF | yes | ok | n/a | none | none | no | none |
| overmap_connection | GF `connections` | GF | yes but useless | **`subtypes` mandatory(false)**: copy-from must restate | no | subtypes replace-only | 4 full replacements of `local_road` (aftershock, TropiCataclysm, desert_region, hunvre), intended | no | small |
| overmap_location | GF `locations` | GF | yes | ok | **no** (flags/terrains plain flat_set; extend throws) | replace-only | 11 full redefinitions (aftershock, desert_region, TropiCataclysm), intended | no | trivial |
| city | GF `city_factory` (no src) | last wins | yes | ok | n/a | none | 351 mod cities, no overrides | copied into overmap | none |
| overmap_special | GF `specials` (**src tracking disabled**, `omdata.h:832`) | last wins, **no error even in one file** | yes | mostly | ext/del flags only | `overmaps`, `connections`, `joins`, `phases` replace-only; `check_for_locations_area` appends | 65 (desert_region 23, innawood 21, Sky_Island 19...), mostly `occurrences:[0,0]` blacklists, OK | yes (placements; migration) | small (src) / medium (lists) |
| overmap_special_migration | GF `migrations` | GF | yes | ok | n/a | none | railroads 2 (last wins) OK | applied on load | none |
| city_building | **alias**: loads into the same `specials` factory | shares namespace with overmap_special | yes | as overmap_special | as overmap_special | as overmap_special | 4 full redefinitions (house_09 ×3, house_detatched5), OK. **Core `charging_station_2x2` defined 3×** | as special | small |
| map_extra | GF `extras` | GF | yes | ok | flags via reader, plus a buggy manual block | none | Backrooms 15 + MoM 8 copy-from+extend flags, OK | yes (overmap extras) | trivial |
| omt_placeholder | GF `placeholder_map_data` (no src) | last wins | yes but **asserts** without `grid` | grid must be restated | n/a | none | none | no | trivial |
| region_settings_river | GF | GF (no src) | yes | ok | n/a | none | TropiCataclysm, desert_region (copy-from default) OK | region id in overmap | none |
| region_settings_lake | GF | GF | yes | ok | readers on shore lists; aliases replace-only | aliases replace-only | classic_zombies, desert_region OK | same | none |
| region_settings_ocean | GF | GF | yes | ok | n/a | none | XedraWood, classic_zombies, TropiCataclysm OK | same | none |
| region_settings_ravine | GF | GF | yes | ok (`ravine_bottom_terrain_id` read with `true`) | n/a | none | none override | same | none |
| region_settings_forest | GF | GF | yes | ok | n/a | none | 4 copy-from default OK | same | none |
| region_settings_highway | GF | GF | yes | ok | building bins via reader | bins | none | same | none |
| region_settings_forest_trail | GF | GF | yes | ok | trailheads via reader | bins | innawood, desert_region OK | same | none |

## Notes on types that are not fully fine

### item_action (custom)
- `src/item_action.cpp:248-258`: `item_actions[ia.id] = ia;` (last wins, `name` defaults to the id).
- The map is never cleared: there is no entry in `DynamicDataLoader::unload_data`, `src/init.cpp:806`. Core-only today, so the leak is latent.

### vehicle_part (partial)
- `parse_vp_reqs`, `src/veh_type.cpp:180-211`:
  - each `requirements.install/removal/repair` sub-object resets `skills` (`optional( src, false, "skills" … )`, `:190`);
  - unless `using` is given, it rebuilds the inline requirement `inline_<key>_<id>`, so a copy-from part that gives only `"removal": { "time": "5 m" }` loses the inherited skills and tools.
  - **20 core appliances rely on this.** Examples: `ap_heater_large` (removal `{time}`, so the parent's `mechanics 1` and `vehicle_screw` are dropped) and `ap_folding_solar_panel` (`"install": {}`).
  - Making these sub-objects honour `was_loaded` would change those 20 parts. Audit them first, or add `"replace": true`.
- `control_requirements` (`:319`) and `vpslot_workbench`/`vpslot_terrain_transform` (`:452-466`) read their sub-fields with `false`.
- `enchantments` (`:284`) are always pushed: a copy-from keeps the parent's and adds new ones, and none can be removed.
- `variants`, `pseudo_tools` and `damage_reduction` are replaced whole.

### vehicle_part_category (custom)
- `src/veh_type.cpp:1662-1667`: `push_back` into a vector with no dedupe, so a mod redefining a category gets two tabs. Trivial: GF with 4 fields.

### vehicle_part_migration, trap_migration, effect_migration, camp_migration (custom, first wins)
- `src/veh_type.cpp:1694` `emplace`, `src/savegame_json.cpp:5148` `insert`, `src/effect.cpp:1953` `emplace`, `src/overmap.cpp:1358` `emplace`.
- A mod cannot redirect a core migration. No shipped conflicts (checked by key).
- Fix: `insert_or_assign`, plus a warning on a same-mod duplicate.

### vehicle (partial)
- `vehicle_prototype::load`, `src/veh_type.cpp:1340-1361`: the `parts` loop always `insert`s at the end (`:1357`). A copy-from vehicle can only add parts; removing or moving one needs a full redefinition.
  - classic_zombies `suv_electric_rack` (copy-from `suv` + 4 parts) relies on this append.
- `items` is plain `optional()` (`:1360`): replace-only, and extend throws.
- `:1342` `vgroups[vgroup_id( id.str() )].add_vehicle( id, 100 )` runs on every load, including abstracts (`handle_inheritance` calls `load`):
  - each of the 17 core vehicle abstracts creates a phantom vehicle_group named after itself, holding an invalid prototype;
  - each redefinition adds the vehicle to its own-name group again.
- Finalize builds the blueprint (`vehicles::finalize_prototypes`, `:1515`) after vehicle parts and vpart migrations.

### vehicle_group / vehicle_placement / vehicle_spawn (custom, merge)
- `src/vehicle_group.cpp:80-88`, `:105-113`, `:186-205`: `map[id]` then `add`. A redefinition appends to the vanilla list, and nothing can be removed or reweighted.
- Every shipped override relies on append, so a GF move must keep append as the default (or turn the overrides into `extend`):
  - aftershock adds atomic cars to 9 vanilla groups and defines `city_vehicles`, `city_pileup`, `highway` and `parkinglot` twice in one file. Under GF that would throw "two definitions from the same source".
  - No_Hope's `default_bridge` (849000 "no_vehicles" + 151000 wrecks) and `default_parkinglot` (950000 / 50000) only work because huge weights drown core's 100. Core's `fuel: -1` vehicle still has a 20/1,000,100 chance.
- No `check_consistency` validates group contents.

### requirement (custom)
- `src/requirements.cpp:497-594`. `save_requirement` merges into `requirements_all[id]`: each list replaces the old one only when non-empty (`:568-579`).
  - A redefinition cannot clear a list.
  - A same-mod duplicate raises no error.
  - `copy-from`, `abstract` and `delete` are not read.
- `extend` is positional: `extend.components[i]` is appended to existing group *i*, and groups past the current count are silently dropped (`:582-593`).
  - All 108 shipped mod extends stay within the group count.
  - An extend of a requirement defined later in the same mod is lost (the target still has 0 groups when it runs).
- The same map holds the inline requirements of recipes (`inline_recipe_*`), constructions (`inline_construction_*`), vehicle parts (`inline_install_*`, …), faults and wounds. The merge rule therefore leaks into those types (see recipe).
- Effort medium: the positional-extend semantics are shared by every inline user.

### trap (partial)
- `src/trap.cpp:165-172`: inline `eocs` are always pushed, so copy-from duplicates the inherited ones (TERR-J06).
- `flags` (`cata::flat_set`, plain `optional()`) and `drops` have no reader, so `extend`/`delete` on them throw.
- DATA-J04: `difficulty`, `visibility` and `avoidance` are reader-less `mandatory()`, so `relative` throws even with copy-from.

### ITEM (partial, minor)
- `src/item_factory.cpp:4833-4852`: `faults` and `fault_groups` are added on every load (append-only).
- `:4780`: `degradation_multiplier` is read with `false`, and `degrade_increments_` is re-divided on each load.
- `itype::load_slots` reads `subtypes` into a local (`:3431`). A copy-from item that omits `subtypes` never runs its slot `deserialize`, so a slot field it sets is reported unvisited.
  - Worse, `extend`/`relative` on a slot field are silently ignored, because the extend object allows omitted members.
  - 154 shipped copy-from items omit `subtypes`; none of them touches a slot field (checked).
- `:4772`: `weapon_category` is read with `true`, so it is never defaulted.

### MIGRATION (custom)
- `src/item_factory.cpp:4913-4967`: a non-variant migration overwrites (last wins), with no duplicate warning. Core has 4 conflicting duplicates:
  - `boots_h20survivor_light` → `wetsuit_gloves` at `data/json/obsoletion_and_migration_0.J/migration_items.json:593` and → `wetsuit_booties` at `:638`. The first is clearly meant to be `gloves_h20survivor_light`, which therefore has **no migration**.
  - `egg_vocal_frog` and `egg_shift_frog` each have two different targets.

### charge_removal_blacklist / temperature_removal_blacklist (custom)
- `src/savegame_json.cpp:3276-3292`: id-less set unions with `jo.allow_omitted_members()`, so a typo in a member name is silently accepted.
- Still not reset in `unload_data` (DATA-B05), so Magiclysm's entries leak into the next world loaded in the session.

### MONSTER (partial, minor)
- `src/monstergenerator.cpp:1054-1064` (`special_when_hit`) and `:1158-1163` (`baby_flags`): replaced whole when present.
- `special_attacks` and weakpoints have keyed readers. Otherwise was_loaded is fine.

### SPECIES (fine loader; two shipped overrides lose data)
These are full redefinitions without copy-from, so standard GF semantics drop every field they omit:
- `data/mods/Defense_Mode/mod_interactions/mindovermatter/species.json:4` gives `HUMAN` only `flags`. With Defense_Mode and MoM loaded, `HUMAN` loses `bleeds: fd_blood` (core civilians stop bleeding, `mtype::get_bleed_type`, `src/mtype.cpp:431`) and its description.
- `data/mods/classic_zombies/monsters/species.json:4` redefines `ZOMBIE` without core's `VENOM_IMMUNE`.
- Fix (data): `copy-from` + `extend`.

### recipe / uncraft / practice / nested_category (custom, GF-like)
- `recipe_dictionary::load`, `src/recipe_dictionary.cpp:638-663`: own deferral, last wins, same-source throws.
- Inline requirements are rebuilt every load (`src/recipe.cpp:587`, `:727`). A copy-from child that omits `components`, `tools` or `qualities` gets a debugmsg, and extend/delete on them throws (`:717-723`). This is the open part of CRAFT-J01.
- **Finding not in the audit: full redefinitions keep the vanilla inline lists.** A recipe redefined without copy-from reuses the id `inline_recipe_<id>`, and the `requirements_all` merge (`src/requirements.cpp:568-579`) keeps any list the new definition omits. Five shipped overrides that drop a list still require the vanilla list:
  - innawood `mortar_build` (`data/mods/innawood/recipes/other/materials.json`): drops `concrete_mix_tool 50`, but it is still required;
  - innawood `brew_dandelion_wine` (still needs `surface_heat 6`);
  - innawood `well_pump` (still needs HAMMER 2, SAW_M, SCREW, WRENCH, GLARE);
  - Generic_Guns `slamfire_shotgun_d_welded_together` (still SAW_M, HAMMER 2, FILE);
  - Xedra_Evolved `bread_flour` in `changeling/brownie.json` (still needs `rock_quern`).

  Fix: in `recipe::load`, erase `requirements_all[req_id]` (or assign rather than merge) when `!was_loaded`. Constructions (`inline_construction_<id>`) and vehicle parts have the same latent issue.
- `name` for `practice` and `nested_category` is `mandatory( jo, false, "name" )` (`src/recipe.cpp:683`, `:706`), so overrides must restate it. `container_variant` is read with `false` (`:461`).
- `blueprint_provides` and `blueprint_excludes` are `emplace_back`ed without a clear (`:635-639`), so they duplicate on copy-from.
- `recipe`, `practice` and `nested_category` share the `recipes` map, so one id names one object across the three types (no collision in the data today).

### recipe_group (partial)
- `src/recipe_groups.cpp:52-80`: `building_type` is read with `get_string`, so every load, including copy-from, must restate it.
- `recipes.emplace` and `om_terrains[...].emplace_back` append. An existing recipe's description cannot be changed, and nothing can be removed.

### tool_quality (partial)
- `src/requirements.cpp:170-175`: `usages` are always appended.

### technique (partial, latent bug)
- `ma_requirements::was_loaded` (`src/martialarts.h:212`) is never set to true. So `reqs.load()` (`src/martialarts.cpp:411`, and `:496` for buffs) re-defaults every requirement on a copy-from:
  - `unarmed_allowed`, `melee_allowed`, weapon categories, buff and flag requirements, skill and damage minimums;
  - and extend/delete on them throws "no copy-from".
- `bonus_container::load` (`src/bonuses.cpp:198`) `emplace_back`s, so a copy-from child that restates `flat_bonuses` stacks its bonuses on the parent's.
- `eocs` and `post_hit_eocs` append (`:372-377`). No shipped technique uses copy-from, so all of this is latent.

### martial_art (partial)
- `src/martialarts.cpp:548-552` (`autolearn`) and `:572-610` (14 `*_eocs` lists) append.
- Inline buffs are loaded by `ma_buff_reader` through `ma_buffs.load( jsobj, "" )` (`:537`), so every inline buff has source `""`.
  - A mod that fully redefines a style and restates an inline buff of the same id gets "two definitions from the same source" (latent; no shipped style does this).
- Perk_melee's full redefinition of `style_none` omits `teachable: false`, so the style becomes teachable.

### effect_type (custom, now fine)
- `src/effect.cpp:1635-1842`: rewritten for HLTH-J02 with copy-from, abstract, extend/delete on id lists and enchantments, last wins, a same-mod error, and its own deferred list keyed by load number (DATA-B03-safe).
- Moving it to GF would be medium effort with no functional gain.

### overmap_terrain (partial, minor)
- `src/overmap_terrain.cpp:620`: `uniform_terrain` is read with `false`.
- `:597-606`: a copy-from without `looks_like` prepends the parent id.
- `:357-369`: an embedded `mapgen` array appends to the mapgen registry on every redefinition.
- **alt_map_key**: id-array entries such as `{"id": ["bank_roof","bank_roof_1"], "copy-from": "bank"}` (`data/mods/alt_map_key/overmap_terrain_blue.json:538`) copy *another* OMT into each sibling. 51 sibling OMTs pick up gameplay fields they did not have:
  - `bank_roof` gains `SIDEWALK`/`SOURCE_LUXURY` flags, `see_cost: high`, the `riot_damage` post-process generator and `city_building` vision;
  - `office_tower_collapse_a1` gets the wrong vision and see_cost;
  - `ws_regional_dump_*` gets the wrong vision.

  The mod only meant to change `sym`/`color`. This is not a loader bug, but GF offers no "patch each listed id in place" form (a per-id `copy-from` self would need 946 objects to become thousands).

### construction (partial)
- `src/construction.cpp:2411-2413`: the inline requirement is rebuilt from the child's JSON, so a copy-from child that omits its lists is free to build (TERR-J06).
- `:2415-2421`: the `using` array `emplace_back`s onto the inherited `reqs_using`.
- No `src` member, so a same-mod duplicate raises no error.

### mapgen (custom)
- `src/mapgen.cpp:879-930` dispatches on `om_terrain` (string, list, or 2-D grid), `nested_mapgen_id` or `update_mapgen_id`. Each definition is appended as a weighted alternative (`:839-874`; default weight 1000, `:803`).
- There is no id per definition, so a mod cannot remove or replace a vanilla mapgen. Shipped mods use weight 100000–1e9 instead:
  - classic_zombies `solar_removals.json`, XE, No_Hope `helipad` ("should probably be done by changing the vanilla version to a group and overriding that instead to avoid the duped map"), desert_region `lake_bed` 1e9.
  - Defense_Mode redefines the `public_works_*_roof` mapgen at equal weight. It is identical to core, so harmless.
- The JSON is stored at load and parsed in finalize (`calculate_mapgen_weights`).
- Effort large: needs an id per definition and a remove/override form.

### overmap_connection (partial)
- `src/overmap_connection.cpp:101-104`: `mandatory( jo, false, "subtypes" )`, so copy-from cannot add one subtype. All 4 shipped `local_road` replacements restate the list (intentional).
- `subtype::load` (`:50-60`) is all `false`.
- Small fix: key subtypes by `terrain` + locations.

### overmap_location (partial)
- `src/overmap_location.cpp:45-46`: plain `optional()` on `flat_set`s, so `extend: {terrains}` throws. Trivial: `string_id_reader`.

### overmap_special and city_building (partial)
- `src/omdata.h:828-832`: `src` is commented out ("fake specials … would count as a duplicate"), so there is no duplicate detection at all, not even within one file.
- `city_buildings::load` (`src/overmap.cpp:177-181`) is an alias into the same `specials` factory, so a city_building and an overmap_special share ids.
- **Core bug:** `charging_station_2x2` is defined three times:
  - two `city_building`s at `data/json/overmap/multitile_city_buildings.json:4555` and `:4566` (the second is the roofed variant and evidently should have a `_roofed` id);
  - one `overmap_special` at `data/json/overmap/overmap_special/specials.json:525`, which loads last.
  - The roofed city building silently never exists, and the regional city list's `charging_station_2x2` (weight 400) resolves to the special.
- Lists `overmaps`, `connections`, `joins` and `phases` are replace-only (`src/overmap_special.cpp:321-323`, `src/overmap_special_mutable.cpp:963-966`). `check_for_locations_area` appends (`:931-962`, documented).
- Shipped "removals" are full redefinitions with `overmaps: []` and `occurrences: [0,0]`, which work.

### map_extra (partial, latent bug)
- `src/map_extras.cpp:1363-1374`: the manual `delete` block reads `jo.get_object( "extend" )` (`:1370`). It is redundant, since `flags` already uses `string_reader`.
  - With both `extend` and `delete` present, it erases the flags that `extend` just added.
  - No shipped map extra uses `delete`.
- Fix: delete both manual blocks.

### omt_placeholder (partial, latent)
- `map_data_summary::load`, `src/overmap_map_data_cache.cpp:60-68`: `grid` is read into a local `std::array`. A copy-from without `grid` leaves 24 empty rows and hits `cata_assert( row.size() == 24 )`.

### monster_adjustment / test_data (custom accumulators)
- `src/monstergenerator.cpp:324-342`: id-less `push_back`, so a mod cannot cancel another mod's adjustment.
- test_data is test-only.

## Registered in init.cpp but loaded in a non-obvious way

- **ITEM**: one type with `subtypes` dispatch to slots (`itype::load_slots`, `src/item_factory.cpp:3370`). A slot is created only when `!was_loaded || !slotptr` (`:3363`).
- **overmap_terrain**: two tiers. `oter_type_t` loads through GF; `oter_type_t::finalize` (`src/overmap_terrain.cpp:682`) registers one `oter_t` per rotation or line in a second factory, and saves store those `oter_t` ids. An embedded `mapgen` array feeds the mapgen registry.
- **city_building**: an alias of `overmap_special` (same factory, decided by `jo.get_string("type")`).
- **recipe / practice / nested_category**: three types, one `recipes` map; `uncraft` has a separate map. `recipe_dictionary::finalize` drains its own deferred list, removes abstracts, then blacklists.
- **requirement**: one map shared with inline requirements generated by recipe, construction, vehicle_part, fault and wound loaders (`inline_*` ids). Finalized after Items (`src/init.cpp`, "Crafting requirements").
- **mapgen**: dispatch on `om_terrain` / `nested_mapgen_id` / `update_mapgen_id`. The JSON is kept and parsed at finalize (`calculate_mapgen_weights`).
- **vehicle**: the blueprint is built in `vehicles::finalize_prototypes`, after vehicle parts and vpart migrations. Each load also writes into `vgroups` (cross-type side effect).
- **vehicle_part**: custom `handle_inheritance` hook computes `looks_like` from the copy-from chain.
- **martial_art**: inline `*_buffs` objects are loaded into the `ma_buffs` GF as a side effect, with an empty source.
- **effect_type**: its own deferral list keyed by load number, separate from `DynamicDataLoader`'s.
- **region_settings_\***: 7 separate GFs here, referenced by id from `region_settings` (not in this part). All finalize in `region_settings::finalize_all`.
- **oter_id_migration**: `A→A` deletes a migration (the only id-less "delete" among these types).

## Worst five for modders (part 01)

1. **recipe** (with uncraft, practice, nested_category): most overridden (634); inline lists can't be inherited or extended; full redefinitions silently keep vanilla tools and qualities (5 shipped mods).
2. **vehicle_group / vehicle_spawn / vehicle_placement**: append-only, so vanilla entries can't be removed or reweighted; No_Hope relies on weight tricks.
3. **mapgen**: no way to replace or remove a vanilla mapgen; mods rely on weights up to 1e9.
4. **requirement**: custom merge (a list can't be cleared), no copy-from or delete, positional extend that silently drops extra groups, and the merge leaks into every inline requirement.
5. **vehicle**: parts are append-only, so a copy-from can't drop or move a part. Runners-up: technique (copy-from wipes requirements, latent) and overmap_special (no duplicate detection, shared namespace).

## Shipped data that silently fails today

- 5 recipe full redefinitions keep vanilla tools and qualities (innawood `mortar_build`, `brew_dandelion_wine`, `well_pump`; Generic_Guns `slamfire_shotgun_d_welded_together`; XE `bread_flour`).
- Core `charging_station_2x2` roofed city_building overwritten (shared namespace, no duplicate check).
- Core MIGRATION `boots_h20survivor_light` duplicated, so `gloves_h20survivor_light` is never migrated.
- alt_map_key: 51 OMTs inherit a sibling's flags, see_cost, vision and spawns.
- Author-side full redefinitions: Defense_Mode×MoM `HUMAN` loses bleeds and description; classic_zombies `ZOMBIE` loses VENOM_IMMUNE; Perk_melee `style_none` becomes teachable.
