# DATA-J02 survey, part 02 (61 types)

Tree: `/home/user/nickclysm`, branch `claude/sweet-cannon-ilgctf`. I read the code as it is now; the line numbers are current.

Terms used in the table:
- **gf** means `generic_factory`. Its `load()` starts a fresh `T def`, so a redefinition without `copy-from` replaces the whole object, last wins. To override only some fields, a mod writes `"copy-from": "<same id>"`.
- **untracked** means the class has no `src` member. Two definitions from the same source then go through with no `mod_error` (mod_tracker.h:74-81): last wins, silently.
- "standard" means: all members use `optional()`/`mandatory()` with `was_loaded`; `extend`/`delete` work on readers (generic_typed_reader); `relative`/`proportional` work on numbers, except on `mandatory()` members (DATA-J04).

Counts: **28 fully fine** (gf), **15 partial** (gf with gaps), **18 custom**. The 18 custom are 9 id-keyed loaders that matter, 7 that don't apply (no ids, or append-only aggregate lists) and 2 that are ignored.

| type | loader | dup id | copy-from | re-load honoured | extend/delete | object lists | mods overriding (broken?) | saved? | effort |
|---|---|---|---|---|---|---|---|---|---|
| region_settings_city | gf `region_settings_city_factory` (untracked) | last wins | yes | yes; `city_size` is mandatory, so no relative | houses/shops/parks: weighted reader, extend/delete by key | weighted lists, editable | 6 mods use copy-from+extend on `default` (OK) | no (referenced by region_settings) | n/a |
| region_settings_terrain_furniture | gf `region_settings_terrain_furniture_factory` (untracked) | last wins | yes | yes | `ter_furn` (auto_flags_reader) | id list | desert_region copy-from (OK) | no | n/a |
| region_terrain_furniture | gf `region_terrain_furniture_factory` (untracked) | last wins | yes | yes | replace_with_* weighted, extend/delete | weighted | ~110 overrides: total-conversion mods restate in full, innawood/Magiclysm copy-from+extend (OK; two total conversions together: last wins) | no | n/a |
| region_settings_forest_mapgen | gf (untracked) | last wins | yes | yes | `biomes` | id list | Magiclysm copy-from+extend (OK) | no | n/a |
| region_settings_map_extras | gf (untracked) | last wins | yes | yes | `extras` | id list | MoM, innawood copy-from+extend; desert_region restates (OK) | no | n/a |
| forest_biome_component | gf `forest_biome_feature_factory` (untracked) | last wins | yes | yes | `types` weighted | weighted | TropiCataclysm/desert/innawood copy-from (OK) | no | n/a |
| forest_biome_mapgen | gf (untracked) | last wins | yes | yes, but `terrain_furniture` is a plain map (whole replace) | components/groundcover/terrains | weighted/id | 17 overrides with copy-from+extend (OK) | no | n/a |
| map_extra_collection | gf (untracked) | last wins | yes | yes | `extras` weighted | weighted | 43 overrides: CrazyCat/XedraWood/MoM/XE/DinoMod copy-from+extend; desert_region restates in full (wipes the others' extends if it loads after them) | no | n/a |
| region_settings | gf `region_settings_factory` ("region_settings_new", untracked) | last wins | yes | yes; `cities` is mandatory | ids only; `default_oter`/`urbanity_increase` are plain vectors (replace only) | none | 7 copy-from overrides of `default` (OK) | **yes**: region id per overmap (`placed_regions`, savegame.cpp:2139) | n/a |
| dimension | gf `dimension_factory` (untracked) | last wins | yes | `jo.read` (keeps the value if absent): fine for one field | n/a | none | 7 mods redefine `default` (OK); Sky_Island defines it twice (identical) | **yes**: `dimension_prefix` | n/a |
| dimension_region_layout | gf `dimension_regions_factory` (untracked) | last wins | parent copied, but the generator is **rebuilt** with `mandatory(jo,false,…)` | **no** for the generator | none | none | Megacity restates `default` (OK) | indirectly (placed regions) | small |
| ITEM_BLACKLIST | custom: appends `{whitelist, set}` to a vector | n/a (no id) | n/a | n/a | append only | n/a | 8 in mods (OK) | no | n/a |
| TRAIT_BLACKLIST | custom static set | n/a | n/a | n/a | append only | n/a | none | no | n/a |
| TRAIT_MIGRATION | custom `std::map::emplace` | **first wins**, silent; **never reset** | no | n/a | no | n/a | 35 in mods, no clashes | **yes**: applied to traits on save load | trivial |
| colordef / MOD_INFO | ignored here | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| faction | custom vector, deduped | last wins (NPC-J02); same-source: debugmsg | yes, parent must be **defined earlier** (no deferral, no abstract) | without copy-from: rebuild (mandatory) | relative on numbers; `price_rules` reader; `relations` merge by key (no delete); `epilogues` replace only | price_rules editable; epilogues replace | none in mods (TEST_DATA only) | **yes**: whole objects; only some fields refreshed from template | small |
| npc | custom `std::map`, erase+emplace | last wins (NPC-J02); same-source: debugmsg | yes, earlier parent only, no abstract | hand-written `has_*`/`jo.read`, keeps copied values | none; `mission_offered` and `death_eocs` replace | replace only | No_Hope `thug`/`bandit` full redefinition: **now works** (the audit's "first wins" is fixed) | NPCs saved in full; template used only at spawn | medium |
| npc_class | gf `npc_class_factory` | last wins | yes | **no** for 8 `bonus_*` distributions (reset every load); spells first-wins | proficiencies, price_rules; skills/bionics/mutation_rounds merge by key, no delete | spells cannot change level; a string `shopkeeper_item_group` appends | No_Hope NC_THUG/SCAVENGER/SOLDIER, innawood, MoM NC_BONE_SEER restate in full, dropping core bonus_aggression/bravery and NC_SOLDIER proficiencies; **core NC_ROBOFAC_INTERCOM(_DAY/_NIGHT) lose base bonus_str/int/per** | **yes**: NPC `myclass` | small |
| talk_topic | custom `std::map<string, json_talk_topic>`, every definition **merges** | all definitions kept (appended) | **no** | appends; `dynamic_line` replaced if given; `portrait_override` **reset** on every extension | none | responses/repeat_responses/speaker_effect **append only**, no delete/replace | ~426 mod extensions rely on append (work); none can remove a vanilla response | **yes**: NPC chatbin topic ids | medium |
| behavior | gf `behavior_factory` (fork) | last wins | yes | yes | children/conditions extend/delete with before/after | editable | 63 in mods, 15 copy-from (OK) | no | n/a |
| npc_decision_category | gf `category_factory` | last wins | yes | yes | goals | id list | none | no | n/a |
| MONSTER_FACTION | gf `faction_factory` (id member `name`) | last wins | yes | yes | by_mood/neutral/friendly/hate | id lists | 27 ids overridden, mostly copy-from+extend (OK) | **yes**: monster faction id | n/a |
| sound_effect | custom (SDL builds only), appends files under a key | both kept (pooled) | no | n/a | append | n/a | none | no | n/a |
| sound_effect_preload | custom vector | both kept | no | n/a | append | n/a | none | no | n/a |
| playlist | custom map, `playlists[id] =` | last wins, whole | no | rebuild | none | files replace | none | no | trivial |
| gate | gf `gates_data` | last wins | yes | yes | walls | id list | none | no | n/a |
| overlay_order | custom map keyed by overlay id | last wins per key | n/a | n/a | n/a | n/a | 3 in mods (OK) | no | n/a |
| mission_definition | gf `mission_type_factory` | last wins | yes | yes; `difficulty`/`value` mandatory, so no relative | origins/goal flags | none | 21 (innawood, classic_zombies, Sky_Island restate; CrazyCat copy-from) OK | **yes**: mission instances | n/a |
| butchery_requirement | gf `butchery_req_factory` | last wins | yes | `requirements` read by hand: merges by modifier, overwrites sizes | none | cannot remove a modifier | none | no | small |
| harvest_drop_type | gf | last wins | yes | **no**: `harvest_skills.clear()` resets to survival | n/a | n/a | none | yes (`dropped_from` on items) | trivial |
| harvest | gf `harvest_list_factory` | last wins | yes | yes | entries: extend, delete by `drop` | delete+extend (no in-place edit) | 56 (XedraWood restates, Magiclysm/XE copy-from+extend/delete) OK | no | n/a |
| monster_attack | custom `attack_map`, erase+emplace | last wins (debugmsg in test mode only) | **no** | with `attack_type`: rebuild; without it: clones the existing attack and patches it (undocumented) | none | n/a | none | **yes**: special-attack cooldowns by id | medium |
| palette | custom `std::map`, `palettes[id] = ret` | last wins, **no duplicate detection even in one mod** | **no**; `"extending": true` (mods only) appends | rebuild unless extending | none; extending only appends placings per key | cannot remove a key or placing | ~25 redefinitions; **aftershock `lab_loot_research` defined twice in one mod, one silently lost** | no | medium |
| rotatable_symbol | custom sorted vector keyed by symbol | core throws; mods last wins | n/a | n/a | n/a | n/a | none | no | n/a |
| limb_score | gf | last wins | yes | yes | n/a | n/a | 1 (OK) | no | n/a |
| character_mod | gf | last wins | yes, but… | **no**: `mod_type`, `limb_score_op`, `min`, `max` re-default; `limb_score(s)` cleared, then an **empty id** inserted | none | limb_score list replace | none | no | small |
| body_part | gf `body_part_factory` | last wins | yes | mostly; `legacy_id`/token cleared and `opposite_part` reset on copy-from | flags etc. via readers | `limb_scores`, `effects_on_hit`, `qualities`, `armor` **replace only** | aftershock and Isolation-Protocol restate all vanilla parts in full; Limb_WIP copy-from (OK) | **yes**: body map keyed by id | small |
| sub_body_part | gf | last wins | yes | yes (`opposite` reset on purpose) | locations_under etc. | n/a | aftershock/IP restate (OK) | no | n/a |
| body_graph | gf | last wins | yes | yes | n/a | `parts`/`rows` whole replace | none | no | n/a |
| anatomy | gf | last wins | yes | yes | **no**: `parts` is mandatory without a reader, so extend/delete throw | parts replace only | none | no | trivial |
| morale_type | gf `morale_data` | last wins | yes | yes | n/a | n/a | XE `morale_nightmare` (OK) | **yes**: morale points | n/a |
| SPELL | gf `spell_factory` | last wins | yes | mostly; `flags` and `learn_spells` only add | most readers; **not** flags | flags cannot be removed; `"flags"` on a copy **adds** instead of replacing | 2122 in mods, 72 copy-from; **XE `changeling_spring_summon_smashing_plants_actual_summon_spell` keeps parent VERBAL/NO_HANDS/NO_LEGS** | **yes**: known spells | small |
| spell_migration | custom map `emplace` | **first wins**, silent | no | n/a | n/a | n/a | none | **yes**: applied on save load | trivial |
| magic_type | gf (untracked) | last wins | yes | **no** for `failure_eocs` (skipped when was_loaded); `cannot_cast_flags` only add | n/a | eocs | 31 in mods, no overrides | no | trivial |
| clothing_mod | gf `all_clothing_mods` | last wins | yes | `mod_value` **appends** | none | append only | none | no | small |
| ter_furn_transform | gf | last wins | yes | maps filled by `emplace`: on a copy, inherited keys **cannot be changed** (first wins), only added | none | add only | Sky_Island mod_interactions override (OK) | no | small |
| event_transformation | gf | last wins | nominal: `impl_` **rebuilt** from locals | **no** | none | n/a | none | no (events/achievements are) | small |
| event_statistic | gf | last wins | nominal: `impl_` rebuilt, `stat_type` local | **no** | none | n/a | none | no | small |
| score | gf | last wins | yes | yes | n/a | n/a | none | no | n/a |
| achievement | gf `achievement_factory` (shared) | last wins | yes | yes | hidden_by | requirements replace | none | **yes**: achievement ids | n/a |
| conduct | same factory as achievement | last wins, also **across** achievement/conduct | yes, across types | yes | as above | as above | none | **yes** | n/a |
| widget | gf `widget_factory` | last wins | yes | yes (separator/padding flags recomputed) | widgets, flags | clauses replace whole | 84 copy-from+extend sidebars (OK) | panel layout ids (config) | n/a |
| weakpoint_set | gf `weakpoints_factory` | last wins | yes | a copy merges weakpoints by id (adds or replaces) | **no delete** | weakpoints: add/replace by id, no remove | classic_zombies restates (OK); **BombasticPerks `wps_humanoid_body` replaces the whole core set with one `groin` entry** | no | small |
| damage_type | gf (untracked) | last wins | yes | yes; `onhit_eocs`/`ondamage_eocs` only add | n/a | eocs append | Isolation-Protocol `bullet` restates in full, drifts (drops core `mon_difficulty`, monster `BULLETPROOF`) | no | small |
| damage_info_order | gf (untracked) | last wins | yes | yes | n/a | n/a | none | no | n/a |
| wound | gf (untracked) | last wins | yes | yes | n/a | n/a | none | **yes**: wound type | n/a |
| wound_fix | gf (untracked) | last wins | yes | yes (lists replace) | n/a | proficiencies/requirements replace | none | no | n/a |
| mod_migration | custom maps (`insert`) | **first wins**; one id can sit in both maps | no | n/a | n/a | n/a | 3 in mods | **yes**: rewrites world mod lists | trivial |
| faction_mission | gf (untracked) | last wins | yes | yes | n/a | n/a | none | no (C++ uses fixed ids) | n/a |
| mod_tileset | custom vector (TILES only) | n/a (no id) | n/a | n/a | n/a | n/a | n/a | no | n/a |

## Notes on the types that aren't fully fine

### Custom loaders

- **talk_topic**: `src/dialogue.cpp:97` (`std::map<std::string, json_talk_topic>`), `load_talk_topic` at :2154, `json_talk_topic::load` at :2003-2064.
  - Every definition merges into the existing topic. Responses always append (:2034-2050), or go before the standard exits with `insert_before_standard_exits`.
  - `repeat_responses` and `speaker_effect` also only append.
  - `portrait_override` is reset to nullopt on every extension (:2024-2029, the FIXME).
  - No `copy-from`, no way to delete or replace a response.
  - `json_talk_topic::check_consistency` (:2143) is still an empty TODO and is not registered in `init.cpp`.
  - Unchanged from the audit. 426 mod definitions reuse vanilla topic ids, so any change must keep append as the default.
- **palette**: `src/mapgen.cpp:4290` (`std::map`), `load` at :4387-4395, `load_internal` at :4504.
  - `palettes[id] = ret`: last wins, with no copy-from and no duplicate detection.
  - `"extending": true` (ignored for `dda`, :4508) calls `add()` on the old palette. That appends placings per key (:4492) and inlines the old palette's sub-palettes at that moment, so the result is a snapshot. Keys and placings cannot be removed.
  - **Shipped silent failure:** aftershock defines `lab_loot_research` in both `maps/mapgen/arcology_support.json:4` and `maps/mapgen/lab_cryo_room.json:170`. Files load sorted (`filesystem.cpp:412-414`), so the lab_cryo_room copy wins. The arcology version, which adds `SUS_science_personal_effects` to `X`/`d` and a filing cabinet to `l`, is discarded without a message.
- **monster_attack**: `src/monstergenerator.cpp:1321-1332`.
  - `attack_map` erase+emplace; the overwrite debugmsg fires only in `test_mode`.
  - `create_actor` (:1334) builds a new actor when `attack_type` is given: full rebuild with `was_loaded=false`.
  - When `attack_type` is omitted, it clones the existing attack and patches it (`mattack_actor::load` sets `was_loaded = true`, `mattack_actors.cpp`). That is an undocumented self-copy-from. Copying from another id is impossible.
  - Inline `special_attacks` in MONSTER use the same path.
- **npc** (`src/npc.cpp:359`, `load` :394-609) and **faction** (`src/faction.cpp:27`, :55-69, :140-207):
  - NPC-J02 is fixed: last wins, hand-written copy-from, and same-source duplicates are reported by `init.cpp:262-279` (`untracked_types`).
  - Remaining gaps:
    - the parent must already be loaded: no deferral, it throws (`npc.cpp:403`, `faction.cpp:131`);
    - no `abstract`;
    - npc `mission_offered`/`death_eocs` and faction `epilogues` replace whole;
    - faction `relations` can only add or overwrite keys;
    - faction `currency` re-appends a price rule on each copy (:180).
  - Faction saves: `faction_manager::get` refreshes only currency, price_rules, lone_wolf, limited_area_claim, name, desc, mon_faction, epilogues, mutiny and missing relations (faction.cpp:600-626). An override of likes_u/size/power/consumes_food never reaches an existing save.
- **TRAIT_MIGRATION**: `src/mutation_data.cpp:51,1032`.
  - `emplace`, so first wins.
  - `trait_migrations` is **never cleared**: `reset_all` (:939) clears the blacklist but not this. Migrations from one world's mods leak into the next world loaded in the session, and they run on save load (`savegame_json.cpp:814,841`).
- **spell_migration**: `magic.cpp:4068`, `emplace`, first wins.
- **mod_migration**: `mod_manager.cpp:128,132`, `insert`, first wins. One id can be both renamed and removed.
- **playlist**: `sdlsound.cpp:631`, whole replace.
- **sound_effect, sound_effect_preload, ITEM_BLACKLIST, TRAIT_BLACKLIST, overlay_order, rotatable_symbol, mod_tileset**: no ids, or keyed aggregates that are append-only by design. Nothing to fix for DATA-J02.

### generic_factory types with gaps

- **npc_class**: `src/npc_class.cpp:245-253`.
  - `bonus_str/dex/int/per/aggression/bravery/collector/altruism = load_distribution(jo, …)` returns `distribution()` when the member is absent (:179-183). Every copy-from loses them.
  - **Shipped silent failure (core):** `NC_ROBOFAC_INTERCOM`, `_DAY` and `_NIGHT` copy from `NC_ROBOFAC_INTERCOM_BASE`, which exists only to give `bonus_str 8-10`, `bonus_int` and `bonus_per`. All three are lost (`data/json/npcs/robofac/robofac_intercom/robofac_intercom.json`).
  - Other gaps:
    - `spells` uses `_starting_spells.emplace` (:290), so a copy cannot change an inherited spell's level;
    - a string `shopkeeper_item_group` appends to the inherited groups (:261);
    - `skills`/`bionics`/`mutation_rounds` can add or overwrite by key but not delete;
    - `traits` and `work_hours` replace.
- **character_mod**: `src/character_modifier.cpp:115-155`.
  - The locals `modtypestr` (default `""`) and `lsop` (default `"*"`) re-default `modtype` and `limbscore_modop` on every load.
  - `limbscores.clear()` (:121), then with no `limb_score`, `mandatory(jobj, was_loaded, …)` leaves `ls` empty and `limbscores.emplace(ls, 1.0f)` (:135) inserts an **empty limb_score id**.
  - `min`/`max` go through `load_float_or_maxmovecost` (:92), which returns 0 when absent.
  - So copy-from is unusable without restating `value`.
- **dimension_region_layout**: `overmap_worldgen.cpp:313-337`.
  - After copy-from, a new generator is built and `deserialize` uses `mandatory(jo, false, …)` (:341-371), so every generator field must be restated.
  - `generation_mode` has no initialiser (`overmap_worldgen.h:182`), so an object without it switches on an uninitialised value.
  - The shared_ptr generator is shared between parent and copy before the rebuild.
- **event_transformation** (`event_statistics.cpp:727-742`) and **event_statistic** (:1269+): new_fields/constraints/drop_fields, `stat_type` and `field` are locals, and `impl_` is rebuilt from `event_source::load(jo)`. Copy-from gives nothing beyond `description`.
- **harvest_drop_type**: `harvest.cpp:118` runs `harvest_skills.clear()` before the `optional()` calls, so a copy falls back to `{survival}`.
- **SPELL**: `magic.cpp:436-449`.
  - `flags` is inserted into the inherited set, with no replace and no delete.
  - `learn_spells` uses `insert` (:540).
  - **Likely shipped mis-override:** Xedra_Evolved `changeling_spring_summon_smashing_plants_actual_summon_spell` (`spells/changeling_seasonal_magic_spring.json:426`) writes `"flags": ["SILENT","RANDOM_DURATION"]` "because the original's settings prevent it". It still carries the parent's VERBAL, NO_HANDS and NO_LEGS.
  - The doc's "Flags are not always inherited" (JSON_INHERITANCE.md:203) has it backwards: they are always inherited and cannot be dropped.
- **magic_type**: `magic_type.cpp:76`: `failure_eocs` is read only when `!was_loaded`, so a copy cannot set it (it is reported as an unread member). `cannot_cast_flags` only adds (:65-69).
- **body_part**: `bodypart.cpp:391-401` clears `legacy_id`/token on every copy-from, including `torso` copied onto itself; the token is mostly unused now (:599). `limb_scores`, `effects_on_hit`, `qualities` and `armor` replace whole (:454-490).
- **anatomy**: `anatomy.cpp:58`: `mandatory` without a reader means `"extend": {"parts": …}` throws "disabled for mandatory". A mod cannot add a limb to `human_anatomy` without restating the list.
- **weakpoint_set**:
  - `weakpoint.cpp:670-697`: a copy merges by weakpoint id; `weakpoints::remove` exists but is not reachable from a weakpoint_set's JSON.
  - **Shipped silent failure:** BombasticPerks `perkdata/belowthebelt.json:3` defines `wps_humanoid_body` with no copy-from and a single `groin` entry. The core set's arm, part_grabbing, leg and head entries are all replaced, for every humanoid that uses the set (17 references in `data/json/monsters`) whenever the mod is on. It should be `"copy-from": "wps_humanoid_body"`.
- **ter_furn_transform**: `magic_ter_fur_transform.cpp:104-124` uses `emplace` into maps, so an override cannot retarget an inherited terrain, furniture or flag key.
- **clothing_mod**: `clothing_mod.cpp:88`: `mod_values.push_back` accumulates across copies.
- **butchery_requirement**: `butchery_requirements.cpp:60-90` reads by hand.
  - `check_consistency` is still not registered in `init.cpp` (DATA-J03 is still open).
  - "Magic Types" is now registered (init.cpp:1221); talk topics are not.
- **damage_type**: `damage.cpp:159-165`: eocs only append.
  - Isolation-Protocol `damage.json:3` restates `bullet` without copy-from and has fallen behind core: no `mon_difficulty`, no monster `BULLETPROOF` immunity flag. BULLETPROOF is unused by core monsters today, so the impact is low, but it shows the "restate everything" drift.
- **DATA-J04 (`mandatory` blocks relative)** affects the numeric mandatory members here: `region_settings_city.city_size`, `mission_definition.difficulty`/`value` and `mattack_actor.cooldown`.

### Not fully fine, but working as designed

- npc_class overrides in No_Hope, innawood and MoM are full redefinitions. They drop core fields such as NC_SOLDIER `proficiencies` and `bonus_aggression`/`bravery`. The authors could not have written them as copy-from anyway, because of the `bonus_*` bug above.

## Registered in init.cpp but loaded in a non-obvious way

- **Region settings family**: 16 types (nine of them here) each get their own `add()` and factory. `init.cpp` has a single finalize entry, "Region Settings" → `region_settings::finalize_all` (regional_settings.cpp:650), which finalizes all 16 factories in dependency order: components → biomes → ter/furn → extras → sub-settings → region_settings.
  - There is no `check_consistency` for any of them.
  - `region_settings::finalize` reverses `default_oter` (:644), so the JSON is top-down and the in-memory order bottom-up.
  - The region_settings factory is named `"region_settings_new"`, so its error messages say that.
- **dimension / dimension_region_layout**: now reset and finalized (init.cpp:836-837, :1008-1009), which closes that part of DATA-B05. Layouts are finalized before dimensions, which check `region_layout.is_valid()`.
- **achievement and conduct**: one factory. `is_conduct_` is set from the JSON `type` (achievement.cpp:438), so a `conduct` and an `achievement` with the same id overwrite each other, and copy-from works across the two.
- **talk_topic** also loads from inside responses: inline topics go through `load_inline_topic` (dialogue.cpp:1474) into the same map. An `"id": [ … ]` array merges the same JSON into several topics.
- **monster_attack**: the same `create_actor` serves top-level attacks and MONSTER inline `special_attacks`. Whether a redefinition patches or rebuilds depends on whether `attack_type` is present (above).
- **palette**: named palettes go through `mapgen_palette::load`. Inline palettes in mapgen use `load_temp` (no id). `extending` is honoured only for non-`dda` sources.
- **npc / faction**: not generic_factory, but `init.cpp` load_object reports same-source duplicates for them through `untracked_types = {"faction","npc"}` (:265-279). Their copy-from cannot be deferred.
- **ITEM_BLACKLIST / TRAIT_BLACKLIST**: collected at load and resolved in finalize (`item_factory.cpp:1910` whitelist/blacklist merge; `mutation_data.cpp:1077`).
- **sound_effect / sound_effect_preload / playlist** do nothing in non-SDL builds (`sounds.cpp:2079-2081` stubs). **mod_tileset** only works with TILES (init.cpp:659-664); its sprites are loaded later by the tileset loader.
- **colordef** is loaded earlier by the colour manager, and **MOD_INFO** by the mod manager; both go to `load_ignored_type` here.
- **mod_interactions** (all types): loaded after every mod with src `"<mod>#<other>"` (init.cpp:771, game_io.cpp:571). So Sky_Island's `mod_interactions/mindovermatter` `ter_close_tears_and_portals` is a legitimate cross-source override, not a duplicate.
- **DATA-B03** is fixed for generic_factory types (`superseded_by_later_load`, init.cpp:190-233). It does not apply to the custom loaders, which never defer.

## Effort to give these full generic_factory semantics

- **trivial**: TRAIT_MIGRATION (`insert_or_assign` plus a clear in reset), spell_migration, mod_migration, harvest_drop_type, anatomy (add a reader), magic_type, playlist.
- **small**: npc_class (guard `bonus_*` with `has_member`; spells as a map with a reader), character_mod, dimension_region_layout (keep the generator unless its fields are given; clone instead of sharing), event_transformation/event_statistic (store fields as members, build `impl_` in finalize), SPELL flags (reader with delete; recompute `spell_tags` in finalize), clothing_mod, ter_furn_transform (`insert_or_assign` plus delete by key), weakpoint_set (`delete` by id), body_part lists, damage_type eocs, butchery_requirement, faction (deferral and abstract).
- **medium**:
  - **talk_topic**: keep the merging loader. Add `delete: {responses:[{text|id}]}` and `replace_responses`, only overwrite `portrait_override` when present, and register a check. Moving it onto gf would break 426 mod extensions.
  - **palette**: a gf move needs `extending` kept as the merge default for mods, plus delete by key and duplicate detection. Placings are shared_ptr pieces.
  - **monster_attack**: polymorphic actors need clone-based `handle_inheritance`, and the inline attacks share the loader.
  - **npc**: `npc` cannot be copied; deferral and list edits need the hand copy kept in step.
