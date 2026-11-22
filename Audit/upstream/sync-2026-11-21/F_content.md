# Triage: group F_content (22 upstream PRs, vanilla content)

Paths below: `src/...` and `data/...` are relative to the repo. "Fork" means `refs/fork/ours` (= `/home/user/nickclysm` HEAD `5510d452`). Nothing was built or run; everything here comes from reading diffs, the fork's code and the Audit, plus a few scripted checks over the PR diffs (listed in the summary).

| Row | PR | Title | Kind | Verdict | Effort | Meets fork work | Notes |
|---|---|---|---|---|---|---|---|
| 23 | #89409 | Fixbots! (part 1) | content (+2 hardcoded mattacks) | TAKE | trivial | Overlaps monattack.cpp, monstergenerator.cpp, monster_deaths.json, spells.json, a labyrinth terrain file. Fork edits are elsewhere in those files; merges cleanly. Spells pass MAGIC-J03; computer passes MGEN-B03 | Bug: haywire spawner's cap of 50 never applies. Hardcoded ids. Obelisk copy-paste condition. Base of the Labyrinth chain |
| 27 | #89410 | Highland grotto | content | TAKE | trivial | none (new terrains copy from parents with no tool actions, so MAP-B05 has no effect) | Also a one-word text fix and a TEST_DATA whitelist line |
| 29 | #89357 | review dialogue (robofac intercom) | content (dialogue) | TAKE | trivial | `u_has_faction_trust` goes through the fork's null-safe `require_faction` (EOC-B13); the intercom NPC has the robofac faction | "Buy now, pay later" now needs 5 robofac trust; otherwise "No." |
| 44 | #89437 | Labyrinth expansion | content | TAKE | trivial in order (conflicts alone) | Text depends on #89409. Fork's one-line MAP-B05 edit in the same terrain file merges. New computer topics pass MGEN-B03 | One bridge terminal's "extend" condition looks inverted (unsure) |
| 47 | #89412 | Update exodii_merchant_talk.json | content (dialogue) | TAKE | trivial | none | Hub01-interface CBM surgery now also needs `Rubik_exodized_u` (an existing var) |
| 49 | #89427 | Labyrinth Warlord | content (EOC-driven boss) | TAKE | trivial | Fork's EOC paths it uses behave as upstream: `u_run_monster_eocs`, global queueing from a monster alpha, safe `u_teleport` refusal. Small hit from EOC-P02, still open | Grenade offset bug; uid from `rand(1200)`. Needed by #89529 |
| 54 | #89291 | New Scenario: Experienced (part 1) | content | TAKE | trivial | Passes the fork's new start_location parameter check (it sets no parameters) | Starts about 2 months in at a stocked shelter. Unlocked by `achievement_survive_28_days` |
| 55 | #89458 | Initial commit (regen serum) | balance | TAKE | trivial | Wound healing path (`wound::get_wound_healing_modifier`) unchanged by the fork | Adds `REGEN_WOUND` ×50 to the regen serum enchantment |
| 88 | #89529 | the warlord arrives | content | TAKE | trivial in order (conflicts alone) | none beyond its parents | Depends on #89437 and #89427. Places the warlord in labyrinth2 four-way stair tunnels (1/50 per tile) |
| 104 | #89541 | Improvement of Fema Military Building | content / balance | TAKE | trivial | none | Fixes the `§` symbol clash (card reader vs fire extinguisher). Loot buff: guns 20→80 %, new ammo-can group at 90 % |
| 111 | #89195 | Marloss evangelist base camp (Collective Center) | content | TAKE | trivial | Its special-placed `"camp": "marloss"` goes through the fork's OMAP-B03 `find_camp` fix. `marloss` faction exists for MGEN-B03 checks | GLOBALLY_UNIQUE, 75 % chance. No NPCs placed yet |
| 118 | #89569 | Remove regional slur | text | TAKE | trivial | none | 3 strings |
| 121 | #89493 | Marloss Evangelist 1: Joth Luteus | content (NPC) | TAKE | trivial | none (no unknown string conditions, ids unique for NPC-J02's duplicate check) | Not placed by any mapgen yet (dormant). Day-80/160 boundaries duplicate responses |
| 152 | #89580 | Better questgiver dialogue lines | content (dialogue) | TAKE | trivial | Dynamic-line condition keys unchanged (EOC-B02-validated); fork's TALK_COMMON_OTHER fix is in another file | Replaces fixed job-offer lines with snippet pools |
| 153 | #89602 | Editing Isherwood NPC dialogue | text | TAKE | trivial | none (EOC-B05 changed Isherwood item-taking in C++, not these strings) | Text only (checked by structure diff); one `*rolls`→`* rolls` gives a double space |
| 160 | #89596 | Variantize lakeside cabin | content + save migration | TAKE-ADJUST | small | none (no fork test or mod uses the old ids) | **Migration bug:** 15 rotated roof ids map to the ground-floor cabin |
| 161 | #89494 | Marloss Evangelist 2: Sera Nameko | content (NPC) | TAKE | trivial | none | `npc_first_topic` names `TALK_MARLOSS_EVAN_Sera_1a`; the topic is `..._SERA_1a`. Dormant |
| 162 | #89495 | Marloss Evangelist 3: Amaides | content (NPC) | TAKE | trivial | none | 3 response topics don't exist. Dormant |
| 167 | #89648 | scribble-scrabble | content / balance | TAKE | trivial | none (monster groups use no conditions, so MONDT-J01 doesn't apply) | 2 CBM-dropping labyrinth critters; surgical bot weight 10→50 |
| 173 | #89650 | strange... wasn't there a doorway here? | content | TAKE | trivial in order (conflicts alone) | none | Depends on #89437 (so also #89409). Locked 2→1 stairwell becomes a sealed "mysterious staircase" |
| 180 | #89570 | Marloss Evangelist 4: Ameth Aria | content (NPC) | TAKE | trivial | none | Uses #89493's `TALK_MARLOSS_EVAN_JOTH_WAKE` 3 times (looks like copy-paste). Dormant |
| 183 | #89675 | rewrite my flatlander's description | text | TAKE | trivial | none | 1 string |

## Notes per PR

### #89409 Fixbots part 1 (row 23): TAKE, with one optional fix

What it does: adds the "Bot Printer" boss room to the Netherum labyrinth. It brings:
- rouge fixbots, spotbots and medbots, plus haywire variants;
- a death spell that upgrades every ally to its haywire form when the printer dies;
- a Friends Forever passcode obelisk, a bridge computer, fog teleporter traps into the boss room, and a slope to labyrinth2;
- two hardcoded special attacks, `FIXBOT_SPAWNER` and `FIXBOT_SPAWNER_HAYWIRE` (`src/monattack.cpp` `mattack::fixbot_spawner*`, registered in `src/monstergenerator.cpp` `MonsterGenerator::init_attack`).

The C++ against the fork:
- **No conflict.** In `monattack.cpp` the fork edited `shriek_stun` (MAP-J01 basher) and `absorb_meat` (MONAI-B03). In `monstergenerator.cpp` it added `find_json_attack` (MONAI-J01) and the grab `vehicle_pull_formula` check. The PR only adds new functions near `mon_leech_evolution` and two `add_hardcoded_attack` lines.
- **APIs exist on the fork.** It uses `place_critter_around`, `pos_bub( map )`, `get_player_view()` and `g->all_monsters()`; all are unchanged on the fork.
- **Not reachable from fork JSON features.** Hardcoded attacks are excluded from `find_json_attack`, so EOC `u_attack` (MONAI-J01) can't call them. That is consistent with the fork's design.

Bugs in the PR (upstream HEAD has them too):
- **The haywire cap never applies.** `fixbot_spawner_haywire` (monattack.cpp:3878 at the PR commit) counts `mon_LS_fixbot`/`spotbot`/`medbot` but spawns `mon_LS_fixbot_haywire`. Killing the printer upgrades every normal bot to haywire, so the count drops to 0 and the "50 max" cap never triggers.
  - Each haywire printer then spawns a suicide bot every 50 turns, with no limit.
  - Each bot casts a 26-damage explosion plus a 999 bash cone of radius 8 that ignores walls.
  - The fix is one line: count `mon_LS_fixbot_haywire` in that loop.
  - Minor: the comment says 10 allies, the code says 20.
- **Obelisk copy-paste condition.** `t_nl_obelisk_fixbot`'s `EOC_LS_Obelisk_fixbot_grant` checks `not u_has_trait LS_Passcode_1`; the other obelisks check their own passcode. A player who already has the fixbot passcode gets "The obelisk is inert" and the grant popup again.
- **Dead data.** The spell `LS_sparkgenerator_summon` is unused. Its own description says "Later should summon via an EOC to cap number spawned".

Audit angle:
- **Hardcoded C++ with a JSON alternative.** This is new behaviour keyed to hardcoded `mtype_id`s, the "hardcoded content ids" theme in Audit/README.md. Ch. 19's rejected-findings appendix already notes that `spell`/`summon` and `eoc` + `u_spawn_monster` attacks cover this. An EOC attack with a `u_monsters_nearby(...) < 20` condition would do the same in JSON.
  - This is optional follow-up work. The later Labyrinth PRs depend on this PR's data, not on its C++.
- **Docs drift.** The two attacks are not added to `doc/JSON/MONSTER_SPECIAL_ATTACKS.md` (MONAI-J13).

Load safety on the fork:
- **Spells.** `summon` names a valid monster; `attack` has an empty effect_str; `upgrade`, `bash` and `explosion` are unchecked. They pass `check_spell_effect_str` (MAGIC-J03).
- **Computer.** Its EOC `EOC_LS_Computer` and chat topic `EOC_LS_Computer_teleports_1` exist, so it passes `jmapgen_computer::check` (MGEN-B03).
- **Medbot grab.** It reuses `grab`, which on the fork carries `pull_from_vehicle` and a valid formula.
- **Other references.** No unknown string conditions, and no ids missing from the fork.

Performance: `all_monsters()` runs once per 50 turns per printer, which is negligible.

### #89437 Labyrinth expansion (row 44): TAKE, take after #89409

What it adds:
- labyrinth2 roofs and second roofs, raised walkways and utility tunnels (mutable specials);
- teleports between labyrinth 1 and the labyrinth2 bridges;
- a permanent fog field;
- a 24-hour exit-terminal lock (`ls_exit_control_locked`, set at entry and cleared by a queued EOC).

Conflicts and overlaps:
- **Conflicts only with #89409.** It conflicts alone in 6 files, all of which #89409 created or edited first. Nothing new appears incrementally.
- **Overlap.** The fork's only change in these labyrinth files is MAP-B05's `"delete": { "oxytorch": true }` on `t_nl_metal_wall_border`. The PR edits `t_nl_roof` text and appends terrains. None of the new terrains use `copy-from`, so MAP-B05's inheritance change doesn't touch them.
- **No fork-fixed labyrinth data.** `git diff a1a646d6 refs/fork/ours --stat` over `mapgen/netherum`, `traps.json`, `nether_eocs` and `npcs/exodii` is empty.

Fork checks:
- **Computer.** The new `chat_topics` `EOC_LS_Computer_teleport_bridge` is defined in the PR, so MGEN-B03 accepts it.
- **Duplicate update_mapgen ids.** None added (checked over all of upstream HEAD), so MGEN-B02's weighted pick changes nothing.
- **Field type.** `fd_labyrinth_fog_perm` passes the fork's stricter `field_type::check` (named levels, no effects).

One thing that may be an upstream bug (I couldn't confirm the intent): in `labyrinth_computers.json`, topic `EOC_LS_Computer_teleport_bridge`, the only "Extend the bridge." option requires `nl_been_to_fixbot != 1`. That chunk's comment says it "always connects to bridges in labyrinth2", but once you've been to the fixbot room this terminal can't extend.

### #89427 Labyrinth Warlord (row 49): TAKE

What it is:
- A 2400 HP tripod boss whose three legs are separate monsters, re-positioned every turn by a global 1-second recurring EOC.
- Attacks: a heat ray, gas grenades, smash and leap.
- The legs' death EOCs damage the body (two dead legs kill it), and the body's death kills the legs.

I checked each fork change that this EOC design relies on:
- **Monster EOCs.** `f_run_monster_eocs` (`src/npctalk.cpp`) is byte-identical to upstream, and it passes the context, so the `_wm_uid` matching works.
- **Queueing from a monster.** `process_eoc` queues a monster-alpha EOC with `global: true` on the avatar, as upstream does.
- **Leg teleports.** The leg "cascade" relies on `u_teleport` (safe = true) failing on an occupied tile. The fork's `teleport::teleport_to_point` still returns false there. SPAT-B07's telefrag fix only touches the unsafe branch, and the fork's "refused teleport changes nothing" keeps the leg in place.
- **Spell casting.** `u_cast_spell` by a monster talker still works; it only needs `get_creature()`.
- **Death EOC talkers.** The death EOC is built as `dialogue( killer, dying monster )`. With no killer, `u_run_monster_eocs` falls back to beta with a debugmsg (EOC-B13 territory, same as upstream).
- **Load checks.** `effect_robot_anchored` (`CANNOT_MOVE`) is honoured for monsters on the fork. The spells pass MAGIC-J03, and there are no unknown string conditions.

Things to flag (all upstream):
- **Grenade offset bug.** `eoc_wells_toxic_grenade` adds the random offset to `_tar` but spawns at `loc` (labyrinth_warlord.json:255-263 at HEAD), so the bomb always lands on the target.
- **Possible uid collision.** Warlord uids come from `rand(1200)`, so two warlords can share an id and swap legs. Unlikely.
- **Cost per turn.** Each warlord near the player costs, per turn, four `u_location_variable` calls plus two creature scans. EOC-P02 (`f_location_variable` allocates a full `map`, about 1.4 MB, per call) is still open on the fork (`npctalk.cpp:4914`), so that is roughly 4 map allocations per warlord per turn. It is acceptable for a rare boss, and another reason to fix EOC-P02 someday.

`effects.json` overlap: the PR inserts near line 68; the fork's edits are at lines 381, 696-750 and 5228. It merges cleanly.

### #89596 Variantize lakeside cabin (row 160): TAKE-ADJUST

What it does:
- Merges `cabin_lakeside1..5` into weighted variants of `cabin_lakeside`.
- Moves the roofs into nested chunks placed at z+1.
- Adds `data/json/obsoletion_and_migration_0.K/` with `overmap_special_migration` and `oter_id_migration` entries.

It is clean on the fork, and nothing in fork tests or mods uses the old ids (the TEST_DATA whitelist regex already covers them).

**Adjust before taking.** In `obsoletion_and_migration_0.K/overmap_terrain.json`, every `cabin_lakeside{1..5}_roof_{east,south,west}` maps to `cabin_lakeside_{east,south,west}`, the ground-floor cabin. Only the `_north` roofs map to `cabin_lakeside_roof_north`.
- In an existing save, an ungenerated rotated cabin's roof OMT would generate a second full cabin (canoe included) one level up.
- Generated ones would just show "lakeside cabin" on the overmap at z+1.
- **Fix:** map those 15 entries to `cabin_lakeside_roof_{east,south,west}`. Upstream HEAD still has the bug, so this is worth reporting upstream.

### Marloss Evangelist series: #89493, #89494, #89495, #89570 (rows 121, 161, 162, 180) and camp #89195 (row 111): TAKE all

These add four unique NPC templates and classes with time-gated dialogue: different lines before day 80, before day 160, and after day 160 (when you can "put him out of his misery" with `npc_die`).

**Dormant.** No upstream mapgen places `marloss_evan_1..4` yet. #89195 builds the Collective Center camp but places no NPCs, so all of this is unreachable in play until a later upstream PR places them.

Fork checks:
- no unknown string conditions (EOC-B02);
- `npc_die`'s `supress_message` spelling matches the loader;
- NPC and faction ids are unique (NPC-J02's same-source duplicate debugmsg won't fire);
- npc_class loading only gained the optional `ai_tuning`.

Upstream bugs (none breaks loading; EOC-J03 topic validation is still open on the fork):
- **#89494.** `npc_first_topic: TALK_MARLOSS_EVAN_Sera_1a` doesn't match the defined `TALK_MARLOSS_EVAN_SERA_1a`. Every conversation after the first would hit the "I don't know what to say" debugmsg.
- **#89495.** Responses go to `TALK_MARLOSS_EVAN_AMAIDES_SETTLE`, `_SITUATION` and `TALK_MARLOSS_EVAN_AMAIDES_Marloss`. None exist; the file defines `MARLOSS_EVAN_AMAIDES_Marloss`, without `TALK_`.
- **#89570.** Three post-day-160 responses send Ameth into Joth's `TALK_MARLOSS_EVAN_JOTH_WAKE`. That topic sets Ameth's first topic to Joth's and shows Joth's lines. It also makes #89570 depend on #89493.
- **All four files** redefine the same `MAR_EVAN_weapon`/`MAR_EVAN_carry` item groups, identically (the last one wins, so it's harmless).
- **Boundary overlap.** Conditions use `<= 80`/`>= 80` and `<= 160`/`>= 160`, so at an exact boundary both response sets show.

**#89195 and OMAP-B03.** The special's `"camp": "marloss"` entry is placed by `add_camp` then `find_camp`. The fork's OMAP-B03 fix makes that lookup exact, which is what this single-camp special needs.

### #89357 robofac intercom review (row 29): TAKE

- **Text.** Rewrites the riot-gear placeholder lines.
- **Trust gate.** "Buy now, pay later" is now gated on `u_has_faction_trust: 5`; below that you get a new `..._NEGOTIATE_NO` topic offering the 8 HGC deal. On the fork, `f_has_faction_trust` uses `require_faction` (the EOC-B13 null-talker fix) with the same semantics, and the intercom NPC has the `robofac` faction.
- **Spawns.** The `u_consume_item`/`u_spawn_item` effects copy the existing "Deal!" response exactly, so EOC-B05/EOC-B19 behave the same.

### #89541 FEMA building (row 104): TAKE

- **Symbol fix.** `§` was both a terrain key (`t_card_military`) and a nested fire-extinguisher key, so every extinguisher spot also became a card reader. The PR moves card readers to `1`.
- **Loot buff.** Racks now spawn `guns_milspec` at 80 % (was 20 %), and ammo comes from a new `milspec_arsenal_group_ammo_part` group at 90 %. This is vanilla tuning; I'm only noting it in case the owner minds the buff.

## Group summary

**Verdicts:** 21 TAKE, 1 TAKE-ADJUST (#89596), 0 OWNER, 0 SKIP.

**Series and dependency chains:**
- **Labyrinth (Tektolnes / John-Candlebury):**
  - #89409 → #89437 → #89650.
  - #89427 → #89529, and #89529 also needs #89437.
  - #89648 is independent.
  - Later upstream PRs in other groups touch the same files: #89649 (D_engine) edits the labyrinth EOC file after #89437; #89641 and #89642 (B_combat) edit `nether.json` and `traps.json` after #89427 and #89650.
  - Taken in upstream order, none of this group adds an incremental conflict ("INCREMENTAL NEW: none" for all 22).
- **Marloss evangelists:** #89493 → #89570 (topic reference). #89494 and #89495 stand alone. #89195 is the setting. All are dormant until upstream places the NPCs.
- Everything else is standalone.

**Riskiest items:**
1. **#89596** corrupts rotated roof OMTs in existing saves unless the migration is fixed.
2. **#89409's unbounded haywire spawner.** It is a runaway spawn plus terrain-bashing bombs after the boss dies, though limited to that room.
3. **#89427's per-turn EOC cost** (EOC-P02) and its two small logic bugs.

None of these interacts badly with a fork fix. Where they touch fork-changed code (teleport, `run_monster_eocs`, queued EOCs, faction trust, computers, spells, start locations, special camps), the fork either behaves exactly like upstream or is stricter in a way this content passes.

**Checks run** (scripts in `scratchpad/f_content/`), over every PR in the group:
- **Unknown string conditions** against the fork's `parsers_simple` (EOC-B02): none. The only hits were enchantment `"ALWAYS"` conditions, which are a different parser.
- **Ids referenced** but defined only by upstream PRs outside this group: none, except the in-group chains above.
- **Ids the fork removed:** none referenced.
- **New terrain/furniture copy-froms** whose parents have tool actions (MAP-B05): none.
- **Duplicate update_mapgen ids** (MGEN-B02): none new.
- **Dialogue topic references:** the three Marloss bugs above.

**Recommendation:**
- Take the whole group in upstream order.
- Fix the lakeside cabin roof migrations when taking #89596.
- Optional:
  - the one-line haywire count fix and the obelisk condition fix in #89409;
  - the grenade `_tar`→`loc` fix in #89427;
  - the three Marloss topic-id fixes.
- Longer term, the owner may want to port `FIXBOT_SPAWNER*` to an EOC attack, in line with the fork's hardcoded-ids work.
