# DATA-J02 survey, part 00 (67 types)

Tree: `/home/user/nickclysm`, branch `claude/sweet-cannon-ilgctf` (upstream merged to 2026-11-21). Read-only survey; line numbers are current code.

Abbreviations:
- **GF(x)**: `generic_factory<T>`; factory variable `x`.
- **src-tracked**: T has a `src` member, so two definitions of one id from the *same* mod throw `mod_error` (`mod_tracker.h:74-81`). GF types without `src` keep the last one silently (one `D_INFO` log line per type). Across mods, every GF type is last-wins.
- **J04**: the member is read with reader-less `mandatory()`, so `extend`/`delete`/`relative`/`proportional` on it always throw, even on copy-from (DATA-J04, `generic_factory.h:665-671`).
- **GF std**: all members are read through `optional()`/`mandatory()` with `was_loaded`. `extend`/`delete` work on container members. `relative`/`proportional` work on numeric `optional()` members.
- **append**: `push_back`/`emplace` with no clear. Copy-from keeps the parent's entries and adds the child's. Restating an entry duplicates it. Nothing can be removed.
- **first-wins (map)**: `std::map::emplace`. On copy-from, a restated key keeps the parent's value.
- **unvisited err**: the loader never reads `copy-from`/`abstract`, so the JSON loader reports them as unvisited members. This is loud, not silent.

## Table

| type | loader | dup id | copy-from | re-load honoured | extend/delete | object lists | mods overriding (broken?) | saved? | effort |
|---|---|---|---|---|---|---|---|---|---|
| WORLD_OPTION | custom: sets `setDefault` on the active world's option (`worldfactory.cpp:2230`); also pre-read at world creation (`mod_setting.cpp:847`) | keyed by option name, last wins | n/a | n/a (one value) | n/a | none | 1 mod object, no conflict | default only; the world's value lives in worldoptions.json | trivial (already reworked) |
| mod_setting | GF(`mod_setting_factory`), src-tracked; pre-read by `mod_settings_reader` | redefinition **without** copy-from throws on purpose (`mod_setting.cpp:529-545`); same mod throws | yes (also in preview reader) | yes; kind-specific members reset only if `kind` changes | GF std; `choices` replaced whole | `choices`: replace only | TEST_DATA/TEST_mod_settings share a test id (intended) | values saved per world | fine |
| EXTERNAL_OPTION | custom: global `options_manager` `setValue` (`worldfactory.cpp:2247`); `stub` flag | last loaded wins (LATITUDE etc. in 6 mods) | n/a | n/a | n/a | none | 14 names set by several mods: load order decides | written to an info record only | small (DATA-J01 menu work) |
| option_slider | GF(`option_slider_factory`), not src-tracked | last wins (same mod silent) | yes | yes | `levels` is J04: no extend | `levels`: replace only | none | no (the levels it applies land in world options) | trivial |
| json_flag | GF(`json_flags_all`), src-tracked | GF | yes | yes (`vehicle_toggle` can be set but not removed) | GF std | none | Xedra_Evolved CRYOGENIC_ROT full restate: works | yes (item flags) | fine |
| jmath_function | GF(`get_all_jmath_func()`), src-tracked | GF | yes | yes | n/a | none | `scaling_factor` set by aftershock, MoM, MoMNoKnacks, Xedra_Evolved: load order decides | no | fine |
| var_migration | custom `std::map` (`global_vars.h:65`), now cleared in `unload_data` (`init.cpp:855`) | `emplace`: **first wins** for the same `from` | n/a | n/a | n/a | none | none in mods | rewrites save vars | trivial |
| connect_group | custom `unordered_map` (`mapdata.cpp:351`) | idempotent, keeps its index | unvisited err | n/a (id only) | n/a | none | Xedra_Evolved adds groups | no | trivial; but see the terrain/furniture extend bug |
| fault | GF(`fault_factory`), not src-tracked | last wins | yes | **partial** | GF std, except the two lists | `melee_damage_mod`, `armor_mod`: append | **core data broken**: copy-from faults stack multipliers | yes (item faults) | small |
| pp_generator | GF(`pp_generator_factory`), not src-tracked | last wins | yes | yes | `sub_generators` replaced whole | `sub_generators`: replace only | aftershock and hunvre both fully redefine `riot_damage`: load order decides | yes: overmap pp decisions keyed by (generator, sub-type ordinal) | fine |
| fault_fix | GF(`fault_fixes_factory`), not src-tracked | last wins | yes (fixed upstream, c44ca639) | yes; `requirements` replaced whole | GF std | `requirements`: replace | none | fix id in mend activity | fine |
| fault_group | GF(`fault_group_factory`), not src-tracked; reset/finalize now done (`fault.cpp:91-103`) | last wins | yes | **no** for `group` | none | `group` weighted list: append, no remove or reweight | aftershock fully redefines `electronic_general`: works | no | small |
| relic_procgen_data | GF(`relic_procgen_data_factory`), src-tracked | GF | yes | **no** for its 8 lists | none | all 8 weighted lists append | none override core | no (results saved by value) | small |
| effect_on_condition | GF(`effect_on_condition_factory`), src-tracked; inline EOCs bypass `load()` via `insert` | GF | yes | **partial**: `effect`/`false_effect` append | GF std otherwise | effect list: append only | No_Hope copy-from changes only `recurrence`: works. aftershock "disables" with a full redefinition: works. A copy-from that restates `effect` would keep the parent's effects (latent) | yes (queued/recurring ids) | small |
| field_type | GF(`get_all_field_types()`), src-tracked | GF | yes | yes, except `immunity_data` | GF std | `intensity_levels` replaced whole (MAP-B04 fixed); `immunity_data` lists append | MoM `fd_darkness`, aftershock 2: full restates, work | yes (submaps) | small |
| field_type_migration | custom `unordered_map::insert` (`savegame_json.cpp:5173`) | first wins | n/a | n/a | n/a | none | 1 mod object, no clash | applies to saves | trivial |
| weather_type | GF(`weather_type_factory`), src-tracked | GF | yes | **partial**: `condition` reset to `true` when omitted; `passive_effects` append | GF std otherwise | `passive_effects`: append | only core `moonlit_scenario_weather` uses copy-from, and it restates `condition` | yes (current weather, overrides) | trivial |
| weather_generator | GF(`weather_generator_factory`), not src-tracked | last wins | yes | yes (J04 on 4 base fields) | GF std | none | XedraWood, classic_zombies, TropiCataclysm, desert_region self copy-from `default`: works (all mutually exclusive) | no | fine |
| ammo_effect | GF(`get_all_ammo_effects()`), src-tracked | GF | yes | yes | GF std | vectors: replace or extend | none | no | fine |
| emit | **custom** `std::map` (`emit.cpp:9`); `check_consistency` is empty | `operator[]`: last wins, rebuilt from scratch | unvisited err | **no**: each definition is a fresh object | none | none | XedraWood `emit_pollen_stream`, aftershock `emit_clairvoyant`: full restates, work | no | small |
| activity_type | GF(`activity_type_factory`), src-tracked (ACT-J01 done) | GF | yes | yes | GF std | none | TEST_DATA self copy-from ACT_MEDITATE: works | yes (character activity) | fine |
| addiction_type | GF(`add_type_factory`), not src-tracked | last wins | yes | yes | GF std | none | none | yes | fine |
| movement_mode | GF(`move_mode_factory`), src-tracked | GF | yes | yes | GF std | none | none | yes | fine |
| vitamin | **custom** `std::map` (`vitamin.cpp:13`) | any redefinition is a **hard error** (`vitamin.cpp:92-93`) | unvisited err | **no** | none | `disease`, `disease_excess`, `decays_into` built fresh | none (impossible) | yes (character vitamin levels) | small |
| material | GF(`material_data`), src-tracked | GF | yes | yes; `resist` replaced whole when present | GF std | none | My_Sweet_Cataclysm `sugar` full restate; Magiclysm blood/hblood copy-from: work | no | fine |
| bash_damage_profile | GF(`damage_profile_factory`), src-tracked | GF | yes | yes | `profile` is J04: no extend/delete | n/a | none | no | trivial |
| bionic | GF(`bionic_factory`), src-tracked | GF | yes | **partial** | GF std for most | `activated_eocs`, `processed_eocs`, `deactivated_eocs`, `enchantments`: append | none override core | yes (+ bionic_migration) | small |
| bionic_migration | custom `std::map` (`bionics.cpp:493`) | first non-null wins; a null `to` is **erased right after insert** | n/a | n/a | n/a | none | aftershock 7, all with `to` | applies to saves | trivial |
| profession | GF(`all_profs`), src-tracked | GF | yes | **partial** | GF std, with custom readers that support extend/delete for skills, addictions and items | `pets`, `effect_on_conditions`: append; `spells`: first-wins map | XedraWood, MoM self copy-from (skills, proficiencies, items): work; innawood 13 full restates: work | yes (avatar profession/hobbies) | small |
| profession_blacklist | custom single static object (`profession.cpp:101`) | each load **resets** the whole list: last loader wins | n/a | n/a | none | none | XedraWood (blacklist 115), MoMNoKnacks (39), hunvre (whitelist 25): any two together, only the last applies | no | small |
| profession_group | GF(`profession_group_factory`), not src-tracked; reset/finalize done | last wins | yes | yes | `professions` is J04: **no extend** | replace only | aftershock fully redefines `adult_basic_background`: works | no | trivial |
| profession_item_substitutions | custom: `map<itype,vector>` + bonus vector (`profession.cpp:916`) | same `item` again is an **error**; bonuses append | n/a | n/a | none | substitutions/bonuses: append only | TEST_DATA adds new items only | no | medium (no ids) |
| proficiency | GF(`proficiency_factory`), src-tracked | GF | yes | yes | GF std | `bonuses`: replace or extend | aftershock/MoM share 3 ids | yes (+ migration) | fine |
| proficiency_category | GF(`proficiency_category_factory`), not src-tracked | last wins | yes | yes | n/a | none | aftershock/MoM share 1 | no | fine |
| proficiency_migration | custom `std::map::emplace` (`proficiency.cpp:719`) | first wins | n/a | n/a | n/a | none | none | applies to saves | trivial |
| speed_description | GF(`speed_description_factory`), src-tracked | GF | yes | yes | GF std (`values` re-sorted) | `values`: replace or extend | none | no | fine |
| mood_face | GF(`mood_face_factory`), src-tracked | GF | yes | yes | `values` is J04 | replace only | none | no | trivial |
| skill | **custom** static `vector<Skill>` plus `contextual_skills` map (`skill.cpp:47-48`) | erase then `push_back`: last wins, rebuilt; the redefined skill moves to the end of the list | unvisited err | **no** | none | level descriptions, `companion_skill_practice` built fresh | `metaphysics` (CrazyCataclysm, aftershock, MoM) and `spellcraft` (Magiclysm, Isolation-Protocol): full restates, load order decides | yes (skill levels keyed by id) | medium |
| skill_display_type | custom static vector (`skill.cpp:250`); **never cleared** in `unload_data` | last wins, rebuilt | unvisited err | no | none | none | none | no | trivial |
| dream | custom static `vector<dream>` (`mutation_data.cpp:59,960-969`) | no id: every object **appends** | unvisited err | n/a | none | whole type is an append-only list | DinoMod 12, Magiclysm 4, aftershock 8 add dreams; nothing can remove or change the 110 vanilla dreams | no | small to medium (needs ids) |
| mutation_category | **custom** `std::map` (`mutation_data.cpp:63,83-103`) | `operator[]`: last wins, rebuilt | unvisited err | **no** (all `optional(...,false,...)`) | none | none | none override core | indirectly (category vitamins) | small |
| mutation_type | custom `std::map` (`mutation_type.cpp:12`), **never reset** (DATA-B05 still open) | idempotent | unvisited err | n/a (id only) | n/a | none | Limb_WIP `HEAD`; shared by 5 mods: harmless | no | trivial |
| mutation | GF(`trait_factory`), src-tracked | GF | yes | **partial**: about 25 hand-rolled members | GF std for the `optional()` members | many append or first-wins (see notes) | **MoM INTERSTICE_RESONANCE_2/_3: enchantment applied twice**; Magiclysm 113, DinoMod 76, Xedra_Evolved 33, aftershock 46 self copy-from with `extend`: work | yes (+ TRAIT_MIGRATION) | medium |
| furniture | GF(`furniture_data`), src-tracked | GF | yes | mostly; `workbench`/`plant_data` rebuilt when present | flags and connect groups by hand: **`extend`/`delete` of `connects_to`/`rotates_to` hit `connect_groups`** | `examine_action` replaced whole | Rummaging 22 self copy-from, aftershock 12: work | yes (submaps + ter_furn_migration) | small |
| terrain | GF(`terrain_data`), src-tracked | GF | yes | mostly; `open`/`close`/`transforms_into`/`harvest_by_season` reset on purpose; `phase_targets`/`phase_temps` append | same connect-group bug as furniture | as furniture | **No_Hope `t_rock_floor`, `t_carpet_red`: `extend.connects_to` RAIL is silently lost**; innawood 4, Xedra_Evolved 4 full restates: work | yes | small |
| ter_furn_migration | custom `unordered_map::insert` (`savegame_json.cpp:5103,5109`) | first wins | n/a | n/a | n/a | none | no clashes | applies to saves | trivial |
| monstergroup | **custom** `std::map` (`mongroup.cpp:31,478-610`) | redefinition **extends** by default; `"override": true` replaces | **unvisited err** (the doc claims support) | **no**: an extension resets `is_animal`, `is_safe`, `replace_monster_group`, `new_monster_group_id` | none | `monsters`: append only, no remove or edit | **24 vanilla-group extensions silently drop `is_animal`/`is_safe`** (MoM, DinoMod, CrazyCataclysm, XedraWood, aftershock, No_Hope) | yes (overmap mongroups store the group id) | medium to large |
| MONSTER_BLACKLIST | custom: three static sets, union (`mongroup.cpp:392`) | no id: additive | n/a | n/a | none (no removal) | sets: add only | 7 mods add: works | no | small |
| MONSTER_WHITELIST | custom: union; `EXCLUSIVE` is sticky (`mongroup.cpp:399`) | additive | n/a | n/a | none | add only | 6 mods: works | no | small |
| speech | custom `map<speaker, vector>` (`speech.cpp:12-24`) | no id: appends | n/a | n/a | none | append only | CrazyCataclysm/Xedra_Evolved add lines to vanilla mi-go speakers (intended); none can remove vanilla lines | no | small (needs ids) |
| ammunition_type | custom `unordered_map`, merged in place (`ammo.cpp:63-68`) | last wins per field; a partial redefinition keeps the rest | unvisited err | effectively yes (`name` kept if absent) | none | none | `money`/`mana` shared by mods: works | no | trivial |
| start_location | GF(`all_start_locations`), src-tracked | GF | yes | yes | GF std | `terrain`: replace or extend | Defense_Mode 5, No_Hope 1 full restates; innawood copy-from: work | no | fine |
| scenario | GF(`all_scenarios`), src-tracked | GF | yes | **partial**: `start_of_cataclysm`/`start_of_game` read only when `!was_loaded`; `surround_groups` append | GF std otherwise; J04 on `points` | `surround_groups`: append | XedraWood 8, Magiclysm 9, DinoMod 5, MoM 6 self copy-from: work | yes (scenario id) | small |
| SCENARIO_BLACKLIST | custom single static object (`scenario.cpp:363-383`) | each load **resets** it: last loader wins | n/a | n/a | none | none | **classic_zombies or MoMNoKnacks wipes core's blacklist of the `defense_mode_fortified` stub**; any two whitelist mods: only the last applies | no | small |
| shopkeeper_blacklist | GF(`shop_blacklist_factory`), not src-tracked | last wins | yes | yes | reader with extend/delete by `operator==` | `entries` | none | no | fine |
| shopkeeper_whitelist | GF(`shop_whitelist_factory`), not src-tracked | last wins | yes | yes | as above | `entries` | none | no | fine |
| shopkeeper_consumption_rates | GF(`shop_cons_rate_factory`), not src-tracked | last wins | yes | yes | as above | `rates` | none | no | fine |
| enchantment | GF(`spell_factory`), src-tracked; inline enchantments `insert` directly | GF | yes | **partial**: `has` reset to HELD and `condition` to ALWAYS when omitted; `values`/`skills`/... are first-wins maps | GF std for `modified_bodyparts`, `mutations` | `intermittent_activation`, `special_vision`: append; `ench_effects`: first-wins; `spell_values`: replace | MoM fully restates 2 core enchantments: works; no copy-from in data | relics save by value | small |
| hit_range | custom static vector (`creature.cpp:3888`) | last wins | n/a | n/a | none | n/a | none | no | trivial |
| scent_type | GF(`scent_factory`), src-tracked | GF | yes | yes | `receptive_species` is J04: a mod cannot add its species to a vanilla scent | n/a | none | no | trivial |
| disease_type | GF(`disease_factory`), src-tracked | GF | yes | yes (J04 on durations and intensities) | GF std | none | none | no | fine |
| ascii_art | GF(`ascii_art_factory`), src-tracked | GF | yes | yes | J04 on `picture` (irrelevant) | none | none | no | fine |
| end_screen | GF(`end_screen_factory`), not src-tracked; reset/finalize done | last wins | yes | **no** for `condition`: reset to `false` when omitted | GF std otherwise | none | only TEST_DATA uses copy-from (latent) | no | trivial |
| snippet | custom `snippet_library` category map (`text_snippets.cpp:18-95`) | appends to its category; duplicate snippet `id` is an **error**; `"override": true` clears the category | n/a | n/a | none per entry | category: append or clear-all | 17 mods add to vanilla categories (intended); hunvre overrides the name categories | yes (snippet ids in items) | medium |
| help | custom map keyed by `order` + per-source offset (`help.cpp:40-97`) | no id; the same `order` twice from one source is an error | n/a | n/a | none | append only; vanilla pages cannot be replaced or removed | 4 mods add pages | no | small |
| item_group | custom `m_template_groups` map (`item_factory.cpp:5380-5438`) | redefinition **replaces** silently, even inside core | **only to the same id**, and then only `extend` (append) is read | no field-level re-load | `extend` = append entries; **no delete** | entries: replace whole or append | innawood 238 and Generic_Guns 148 full replaces (intended); Magiclysm 61, aftershock 72 self copy-from+extend: work. **Core has 3 conflicting duplicate ids** | spawned results only | medium to large |
| trait_group | custom `trait_groups` map (`mutation_data.cpp:1093-1113`) | redefinition **always appends** (emplace returns the existing group) | unvisited err | n/a | none | append only; no replace, no remove | MoM/Magiclysm/Xedra_Evolved extend `NPC_starting_traits` (intended); **aftershock defines 64 groups twice, so every entry rolls twice** | no | small (an override flag) to medium |

Counts: **20 fully fine** (GF, no material gaps), **19 partial** (GF with append-only or reset members, or J04-only lists), **28 custom**.

## Notes on types that are not fully fine

### Options group
- **WORLD_OPTION** (`worldfactory.cpp:2230-2244`, `mod_setting.cpp:1164-1196`): it is now a default-changer; the old `options` form throws. It is also read at world creation by `mod_settings_reader::read_object` (`mod_setting.cpp:833-848`), so it loads in **two passes**. For one option, last loaded wins.
- **EXTERNAL_OPTION** (`worldfactory.cpp:2247-2280`): sets a global option value directly; last loader wins. 14 names are set by several mods (`LATITUDE`/`LONGITUDE` by 6). `stub: true` is an intentional no-op. There is no id or copy-from concept, so it is not a generic_factory candidate.
- **option_slider** (`options.cpp:121-127`): `levels` is reader-less `mandatory`, so a mod cannot extend a slider with a level. Level deserialization uses `false` for `was_loaded`, which is fine.

### var_migration (`savegame.cpp:1985-1992`)
`migrations.emplace(from,to)`: first wins, so a mod cannot change a core migration. A missing `to` still becomes the literal `"NULL_VALUE"` (DATA-J05). The cross-world leak is fixed (`init.cpp:855`).

### connect_group, and the terrain/furniture extend bug
`connect_group::load` (`mapdata.cpp:358-373`) is trivial, but it has a load-time dependency. `map_data_common_t::load` resolves group names immediately through `set_groups` (`mapdata.cpp:1126-1135`, debugmsg if missing). connect_groups therefore only works because `data/json/connect_groups.json` (and `Xedra_Evolved/connect_groups.json`) sit at the top level and load breadth-first, before terrain subfolders.

**Bug:** in `map_data_common_t::load`:
- `extend.connects_to` and `extend.rotates_to` call `set_connect_groups` (`mapdata.cpp:1310-1315`).
- `delete.connects_to` and `delete.rotates_to` call `unset_connect_groups` (`mapdata.cpp:1325-1330`).
- They should modify `connect_to_groups` / `rotate_to_groups`.

### fault (`fault.cpp:276-317`)
`melee_damage_mod` and `armor_mod` are `emplace_back` with no clear (`fault.cpp:302,311`). A copy-from fault keeps the parent's modifiers and adds its own. `item::damage_adjusted_melee_weapon_damage` (`item_degrade.cpp:234-242`) applies every entry in sequence, so the multipliers stack. Core data hits this today:
- `fault_baton_bent` (copy-from `fault_aluminum_bat_bent`): bash ×0.8×0.7 = 0.56 instead of 0.7.
- `fault_staff_short_broken` (copy-from `fault_handle_broken`): bash ×0.1×0.25 = 0.025 instead of 0.25.
- Also affected: `fault_staff_long_broken_half`, `fault_staff_long_broken`, `fault_tonfa_handle_broken`.

Fix: clear when the array is present, or use a reader keyed by `damage_id`.

### fault_group (`fault.cpp:443-450`)
`fault_weighted_list.add` appends (`fault.cpp:447`). A copy-from cannot drop or reweight a fault, and restating one adds a second entry with summed weight. Needs a weighted-list reader with extend/delete keyed by `fault`.

### relic_procgen_data (`relic.cpp:205-277`)
All eight weighted lists append (`relic.cpp:213 ... 276`). The inner `mandatory( jo_inner, was_loaded, "weight", ... )` passes the **outer** `was_loaded`. On a copy-from, an entry missing `weight` therefore loads with weight 0 and is dropped silently by `weighted_list::add`. Latent: no copy-from in data.

### effect_on_condition (`effect_on_condition.cpp:100-139`)
`true_effect.load_effect` / `false_effect.load_effect` (`:123,126`) push onto the inherited `effects` vector (`dialogue.cpp:1495-1499` via `npctalk.cpp:9154`). A copy-from that restates `effect` therefore runs the parent's effects **and** the new ones. A copy-from with `"effect": []` cannot disable them. Only one copy-from EOC ships (No_Hope, recurrence only), so this is latent. `condition`/`deactivate_condition` are guarded by `has_member`, so they are fine.

Inline EOCs (`load_inline_eoc`, `:141-157`) are `insert`ed with no copy-from. An inline EOC whose id matches a JSON EOC from another source silently replaces it.

### field_type (`field_type.cpp:207-353`)
`load_immunity` (`:444-464`) appends all four immunity lists, so a copy-from field restating `immunity_data` doubles them. Everything else honours `was_loaded`, and `intensity_levels` now replaces (MAP-B04). `field_types::finalize_all` calls `finalize()` twice (generic `finalize()` already does it); this is harmless.

### weather_type (`weather_type.cpp:98-157`)
- `read_condition( jo, "condition", condition, true )` (`:156`) is unguarded, so a copy-from without `condition` becomes always-true.
- `passive_effects` appends (`:143-149`).

Both are trivial to fix. The only copy-from in data (`moonlit_scenario_weather`) restates `condition`.

### bionic (`bionics.cpp:337-479`)
- `activated_eocs`, `processed_eocs`, `deactivated_eocs` and `enchantments` append (`:407,411,415,427`).
- Inline enchantment ids are `"INLINE_ENCH_" + name + "_" + n` (`:426`). `name` is a `translation`, and `operator+` uses `translated()` (`translation.cpp:458-461`). The generated id is therefore **language-dependent**, and two bionics with the same display name would overwrite each other's enchantment. Latent: no collision in data today. Fix: use `id.str()` as mutation does.
- `faulty_bionics.push_back( id )` (`:473`) runs on every load, so a redefinition adds a duplicate. A redefinition that drops the flag cannot remove it. Build this list in finalize instead.

### bionic_migration (`bionics.cpp:495-508`)
`migrations.emplace`, then `erase` if the found value is null. A migration with no `to` (documented as "remove the bionic", `OBSOLETION_AND_MIGRATION.md:119`) is erased immediately. `bionic::deserialize` (`:3528-3536`) then still removes the bionic, but with a "doesn't exist and has no migration" debugmsg instead of silently. Non-null: first wins.

### profession (`profession.cpp:231-406`)
- `pets` push_back (`:294`), `spells` is `map::emplace` (`:303`, first wins: a copy-from cannot change a spell level), and `effect_on_conditions` push_back (`:309`).
- `items` is re-read only when present. `both`/`male`/`female` arrays use `optional` + `item_reader`, so extend/delete work, but only when nested inside `items`.
- `points` is J04.

### profession_blacklist / SCENARIO_BLACKLIST
Both are one global object that **resets on every load** (`profession.cpp:111-114`, `scenario.cpp:365-369`), so the last object loaded replaces all earlier ones:
- **Silent failure today:** core `data/json/obsoletion_and_migration_0.I/blacklist_scenarios.json` blacklists the `defense_mode_fortified` stub scenario (empty description, `sloc_bar`). With `classic_zombies` (`exclusions.json`) or `MindOverMatterNoKnacks` (`scenario_blacklist.json`) loaded, their blacklist replaces core's, so the stub appears in scenario selection.
- Two whitelist mods together (e.g. innawood + Sky_Island): only the last whitelist applies. mod_interactions files rely on this layering on purpose.
- profession_blacklist: XedraWood (blacklist, 115 entries) + hunvre (whitelist, 25 entries) cannot both apply.

Fix: union blacklists; intersect or union whitelists explicitly, with a mode.

### profession_group (`profession_group.cpp:40-44`)
`professions` is reader-less `mandatory`, so a mod cannot `extend` a vanilla group and must restate it (aftershock does). This is the clearest DATA-J04 victim in my list.

### profession_item_substitutions (`profession.cpp:916-958`)
Three sub-modes (`item`, `trait`, `bonus`) live in one type. Re-declaring an `item` throws "Duplicate definition of item" (`:925`), and the trait mode throws too (`:944`), so a mod cannot change a vanilla substitution. Bonuses append. There are no ids.

### mood_face, bash_damage_profile, scent_type
Single reader-less `mandatory` list or map (`mood_face.cpp:49`, `map_accessories.cpp:47`, `scent_map.cpp` `receptive_species`): no extend/delete. For example, a mod species cannot be added to a vanilla scent's `receptive_species` without restating it.

### emit (`emit.cpp:39-50`)
Built fresh each time and stored with `emits_all[id] = et`. No copy-from, `check_consistency` is empty (`:61-63`), and `field` is never validated. The `str_or_var`/`dbl_or_var` members are why it was hand-written, but `optional` works with those readers elsewhere.

### vitamin (`vitamin.cpp:52-97`)
Hand-rolled reads. Any second definition throws "parsed vitamin overwrites existing definition" (`:92-93`), so no mod can change a vanilla vitamin at all. Small to move: the struct is plain and the ids are string-keyed in saves.

### skill / skill_display_type (`skill.cpp:160-223`, `:250-262`)
- Static vectors: erase, then push_back a freshly built object. Every redefinition must restate everything; omitted `sort_rank` gets a debugmsg plus 1000000.
- `string_id<Skill>::obj()` is a linear scan.
- About 15 call sites iterate `Skill::skills` directly (newcharacter, mission_companion...), so moving to GF is **medium**.
- `SkillDisplayType::skillTypes` is never cleared (`Skill::reset`, `:131-135`, clears only skills), so a mod display type leaks into the next world.

### dream (`mutation_data.cpp:960-969`)
No id; push_back into a global vector. There is no way to remove or change any of the 110 vanilla dreams (`data/json/dreams.json` has no ids). `object_keys()` in `init.cpp:174-176` calls `get_string("id")`, which marks `id` visited. A modder who adds `"id"` to a dream, hoping to override, therefore gets **no** unvisited-member error: the id is silently ignored and the dream appended. This applies to every id-less custom type (speech, help, blacklists, the migrations).

### mutation_category (`mutation_data.cpp:83-103`)
Built fresh with `optional(..., false, ...)`, then `mutation_category_traits[id] = new_category`. Last wins, the whole object must be restated, no copy-from. Small to move.

### mutation (`mutation_data.cpp:319-605`)
GF with src tracking, and the scalar `optional()` reads are fine. Hand-rolled members, by behaviour on copy-from:
- **append (vector):** `activated_eocs`/`processed_eocs`/`deactivated_eocs` (`:467-477`), `enchantments` (`:486-489`), `integrated_armor` (`:591`), `attacks` (`:598-605`).
- **first-wins map (`emplace`):** `variants` (`:350`), `vitamin_rates` (`:400`), `vitamins_absorb_multi`, `moncams`, `spells_learned` (`:501`), `craft_skill_bonus`, `lumination`, `anger_relations` (`:520`).
- **per-key overwrite, no delete:** `wet_protection`, `encumbrance_always`, `encumbrance_covered`, `encumbrance_multiplier_always`, `bionic_slot_bonuses`.
- **set insert, no delete:** `no_cbm_on_bp`, `restricts_gear`, `remove_rigid`, `allowed_items`.
- **`armor` uses `+=`** (`:578,580,586`): a copy-from that restates armor doubles it.
- `transform`/`personality_score` are rebuilt when present.

**Inline enchantment id collision (silent failure today):** inline enchantments are named `"INLINE_ENCH_" + id + "_" + n` with `n` restarting at 0 (`:487`). With a **self** copy-from that restates `enchantments`:
1. The child's first inline enchantment gets the same id as the parent's.
2. `spell_factory.insert` replaces the enchantment *object* with that id.
3. The trait's vector, now `[X_0, X_0]`, points at it twice.
4. `Character::recalculate_enchantment_cache` (`character.cpp:5175-5201`) applies each entry.

MindOverMatter `mutations/mutation_overrides.json` overrides `INTERSTICE_RESONANCE_2` and `_3`. It means to replace the enchantment, adding `psi_nether_attention`. What happens instead: ARTIFACT_RESONANCE becomes +2500 instead of +1250, and the effect is applied twice.

Copy-froms with a different id (`EATHEALTH_active`, Perk_melee, Xedra_Evolved changelings) silently inherit the parent's enchantments plus their own. That may be intended for `_active` variants, but it cannot be turned off.

### furniture / terrain (`mapdata.cpp:1221-1377`, `:1416-1503`, `:1653-1705`)
- Connect-group extend/delete bug: see connect_group above. **Silent failure today:** No_Hope `terrain.json` `t_rock_floor` and `t_carpet_red` use `"extend": { "connect_groups": ["RAIL"], "connects_to": ["RAIL"] }`. `connects_to` RAIL goes into `connect_groups`, so the floors never connect to rails.
- `flags`: a manual extend/delete block; restating replaces.
- terrain `phase_targets`/`phase_temps` append (`:1444,1456,1461`). A copy-from that restates them doubles the lists and then fails the size check with an error. Latent.
- `examine_action` is replaced whole. Furniture `workbench`/`plant_data` are rebuilt when present (`:1695-1702`), so a partial override loses inherited keys.

### monstergroup (`mongroup.cpp:478-610`)
Redefining an existing id **extends** it (`:488-492`) unless `"override": true`. The extension path then re-reads these with hard defaults:
- `is_animal` (`:502`)
- `replace_monster_group` (`:592`)
- `new_monster_group_id` (`:593-594`)
- `is_safe` (`:598`)

**Silent failure today**, 24 extensions of vanilla groups that set those flags:
- MindOverMatter: GROUP_SAFE (`is_safe`), GROUP_COWS_DAIRY (both), GROUP_CAVE, GROUP_COWS, GROUP_BEARS, GROUP_DEERS, the dog groups, GROUP_RABBIT_WILD, ...
- DinoMod: GROUP_SAFE (`is_animal`), GROUP_RIVER, GROUP_SWAMP, GROUP_STREAM.
- CrazyCataclysm: GROUP_CAVE, GROUP_FOREST, GROUP_RIVER, GROUP_SWAMP.
- XedraWood: GROUP_FOREST, GROUP_RIVER, GROUP_SWAMP.
- aftershock: GROUP_CAVE.
- No_Hope: GROUP_WILDERNESS_FOREST_ZOMBIE.

Effect:
- These groups use `SPAWN_DENSITY` instead of `SPAWN_ANIMAL_DENSITY` (`mapgen.cpp:311,6795`).
- GROUP_SAFE and GROUP_COWS_DAIRY stop counting as safe for `overmapbuffer::is_safe` (`overmapbuffer.cpp:2494-2497`), which is used by mission starts and companions.

Other gaps:
- `copy-from` is never read, so it is an unvisited error, yet `JSON_INHERITANCE.md:161-182` lists monstergroup as supporting inheritance.
- No way to remove or reweight one entry; `monsters` only appends.

Effort **medium to large**:
- The default extend semantics are relied on by about 20 mods.
- `freq_total` arithmetic.
- Group ids are saved in overmaps.
- `Monster2Group` etc. iterate the map.

The trivial stopgap: read the four flags only when present on an extension.

### MONSTER_BLACKLIST / MONSTER_WHITELIST (`mongroup.cpp:392-408`)
Unions into static sets. Nothing can un-blacklist, and `EXCLUSIVE` cannot be turned off once any mod sets it.

### speech (`speech.cpp:16-24`)
No id; appends per speaker. Fine for adding; impossible to remove or replace vanilla lines.

### ammunition_type (`ammo.cpp:63-68`)
Edited in place in an `unordered_map`, so a partial redefinition works by accident. `default` is read with `throw_on_error=true` but is not required.

### scenario (`scenario.cpp:59-169`)
- `start_of_cataclysm`/`start_of_game` are inside `if( !was_loaded )` (`:120`). A copy-from that sets them is reported as an unvisited member, not applied.
- `surround_groups` appends (`:166-169`).
- `points` is J04.

### enchantment (`magic_enchantment.cpp:597-725`)
- `has` defaults to HELD and `condition` to ALWAYS whenever they are omitted (`:626-639`), so a copy-from changes when the enchantment is active.
- `values`, `skills`, `encumbrance_modifier`, `melee_damage_bonus`, `incoming_damage_mod*` use `map::emplace` (`:239-245`): a copy-from cannot change an inherited value.
- `ench_effects` is first-wins (`:646`).
- `intermittent_activation` and `special_vision` append.
- `special_vision` descriptions pass the outer `was_loaded` to their inner `mandatory`.

### end_screen (`end_screen.cpp:55-64`)
Unguarded `read_condition( ..., false )` (`:60`): a copy-from without `condition` is never shown. Only a TEST_DATA object uses copy-from.

### snippet (`text_snippets.cpp:18-95`)
- Per-category append. `"override": true` clears the whole category, but stale `snippets_by_id` entries stay.
- A duplicate snippet `id` is an error (`:77-79`), so a mod cannot replace one vanilla snippet text.
- Snippet ids are saved in items.

### help (`help.cpp:40-97`)
Keyed by `order` plus a per-source offset; no id. Mods can only add pages.

### item_group (`item_factory.cpp:5356-5438`)
- Without copy-from, the group is `isd.reset()` and rebuilt: last wins, with no same-source check. **Core has three conflicting duplicate ids** that silently lose one definition:
  - `nested_p226_9mm` (twice in `itemgroups/Weapons_Mods_Ammo/nested_guns.json`)
  - `boxed_410` (`gunstore_guns.json` vs `ammo/boxed_ammo.json`)
  - `airgun_hunting` (twice in `airguns_guns.json`)
  
  Four more duplicate pairs are identical.
- `copy-from` must equal the group's own id (`:5394-5399`, debugmsg otherwise). Only `extend` is read: entries are appended, nothing can be deleted or reweighted, and a non-extend copy-from reads no entries (`:5420-5424`).
- `ammo`/`magazine` are read again, but the existing group keeps its chances (`make_group_or_throw`, `:5128-5140`).
- A `subtype` mismatch throws `std::runtime_error`, not `JsonError`. No such mismatch exists in data today.
- Inline groups from professions, monsters and others share the same map.
- `JSON_INHERITANCE.md` lists item_group as supporting inheritance with no caveat.

### trait_group (`mutation_data.cpp:1086-1194`)
`make_group_or_throw` uses `emplace` (`:1096-1099`), so **every redefinition appends** to the existing group. There is no override flag, copy-from, abstract or delete. **Silent failure today:** aftershock_exoplanet defines the same 64 `collection` trait groups in both `npcs/Backgrounds/trait_groups.json` and `npcs/mutant_npcs/trait_groups.json`. Each entry is present twice and rolls twice: a 50% entry becomes about 75%.

Stopgap (trivial): honour `"override": true` like monstergroup and snippet, and warn on same-source duplicates.

## Registered in init.cpp but loaded in a non-obvious way

- **mod_setting, option_slider, WORLD_OPTION**: two passes. `mod_settings_reader` (`mod_setting.cpp:763-1048`) pre-parses only these types from the chosen mods' files at world creation, with its own copy-from and deferral (`:820-826`). The normal `DynamicDataLoader` pass loads them again into the factory or the active world.
- **EXTERNAL_OPTION**: has a side effect on global options at load time, and `data/core/external_options.json` is reloaded per world.
- **profession_item_substitutions**: dispatches on member presence into three sub-modes (`item` / `trait` / `bonus`).
- **item_group / trait_group**: also created inline from many other types (`item_group::load_item_group`, `load_trait_group(JsonArray...)`) into the same maps, under generated ids.
- **effect_on_condition, enchantment**: inline objects are `insert`ed into the factory from other types' loaders (`effect_on_condition.cpp:149-153`, `magic_enchantment.cpp:405-409`). They bypass `load()`, copy-from and deferral, and they can silently replace a JSON object of the same id from another source.
- **snippet**: names are reloaded separately (`reload_names`), and there is a hash-to-id migration step after loading.
- **terrain/furniture**: `map_data_common_t::load` resolves connect_group names at load time, so connect_group files must load earlier. That holds only because of the breadth-first file order (`init.cpp:667-695`). Finalize also resolves trap ids and other references (`set_ter_ids`, `mapdata.cpp:1141`).
- **fault / fault_fix / fault_group**: finalized together in `faults::finalize` (`fault.cpp:99-110`). `fault_fix::finalize` writes into faults through `const_cast` (`fixes.emplace`, `:386-388`), so faults must be finalized first.
- **init.cpp `object_keys()`** (`init.cpp:167-186`) runs for every object and calls `get_string("type"/"id"/"abstract")`, which marks those members visited. id-less custom types (dream, speech, help, the blacklists, the migrations) therefore never report a stray `id`. Duplicate-definition debugmsgs exist only for `faction` and `npc` (`init.cpp:266-279`).
- **mutation_type, SkillDisplayType**: never reset in `unload_data`, so they leak across worlds in one session.
