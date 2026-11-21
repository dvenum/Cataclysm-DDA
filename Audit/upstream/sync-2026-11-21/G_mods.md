# G_mods: triage of 37 upstream mod PRs

Scope: in-repo mods. Xedra Evolved (XE) is the bulk: the paraclesian/Sylph rework series, March Lords, glamours, Arvore, essences. The rest are DinoMod, Aftershock (exoplanet), Magiclysm, MindOverMatter (MoM), Sky Island, innawood, Martial Mastery (Perk_melee) and alt_map_key.

How this was checked:
- Read every diff.
- Compared against `git diff a1a646d6 refs/fork/ours -- data/mods` and the fork's commits that touched these mods:
  - `9a4e959d` and `db21dfb9`: EOC-B14
  - `9c38b68a`, `12c7974f` and `e15db0f0`: MAGIC-J03
  - `0ce1d2ae`: PROG-B11
  - `a615c6bd`: USE-B07
  - `8c7b137f`: EVT-B03
  - `c47dbf59`: HLTH-J02
  - `2266c297`: EOC-B02
  - `827993a0`: INV-J01
- Built the full merge of upstream HEAD into `refs/fork/ours` in the scratch bare repo (tree `743a12b0`), only to check data.
- Ran a script that copies the fork's stricter load checks over the merged mod data. It ran over XE, Aftershock, MoM, Magiclysm, DinoMod, Sky Island, innawood and Perk_melee, each loaded with core. It checks:
  - spell `effect_str` against its effect type, `learn_spells`, `extra_effects`, `magic_type`, formula arity, and TOUCH_REQUIRED+NO_HANDS (MAGIC-J03)
  - unknown string conditions (EOC-B02)
  - EOC references
  - pocket keys, including `ammo_restriction` with size keys (INV-J01)
  - duplicate ids within a mod
- Also checked that every JSON key on a `+` line of these PRs is read somewhere in the fork's `src/`: all are, so no PR needs an engine feature the fork lacks. Enchantment values, spell effects and flags were checked the same way.
- The game was not built or run. Runtime behaviour below is from reading the code.

## Table

| Row | PR | Title | Kind | Verdict | Effort | Meets fork work | Notes |
|---|---|---|---|---|---|---|---|
| 2 | #89374 | [DinoMod] crazed dinos attack! | mod content | TAKE | trivial | none | 19 feral dinos added to core GROUP_FERAL, about 16% of feral picks. Bug: `mon_ceratosaurus_feral` copies `mon_dilophosaurus` |
| 12 | #89402 | [XE] One fixed glamour, one new glamour | mod content | TAKE | trivial | none | Devana's Balm; Veins of Ice now calls the core stop-bleeding wound EOC. Soft dep #89351 (stab wound ids) |
| 19 | #89400 | Aftershock: Ravine datavault | mod content | TAKE | trivial | none | All refs resolve; mapgen `computers` ids valid for the fork's new check |
| 24 | #89418 | [XE] Luliya won't talk to the Fair Folk either | content (core NPC) | TAKE | trivial | none | `snip_bye`, `npc_first_topic` exist in fork |
| 28 | #88457 | [XE] Feral dream magick users | mod content | TAKE | trivial | none | New feral eaters/inventors; spells pass MAGIC-J03 |
| 40 | #89440 | (XE) no dreamdross for EMPTY_DREAMS | mod bugfix | TAKE | trivial | Same file as fork's brownie-blink fix (EOC-B14), different EOC; clean | |
| 41 | #89442 | [XE] Rework Paraclesian healing draughts | mod balance | TAKE | trivial | none | Poultices become a 6–12 h wound-regen buff |
| 43 | #89393 | [DinoMod] more endgame skeletals | mod content | TAKE | trivial | none | All 899 `upgrades` targets resolve |
| 46 | #89445 | [MoM] focused light derived_from 0.8→0.25 | mod balance | TAKE | trivial | none | |
| 51 | #89455 | [DinoMod] Rubik won't talk if he dislikes you | mod | TAKE | trivial | none | |
| 60 | #89468 | (alt_map_key) evac shelter oter update | mod | TAKE with #89291 | trivial | none | Hard dep: without #89291, "No mapgen terrain exists for shelter_experienced" load error |
| 66 | #89472 | [innawood] remove canvas-less canvas bag recipes | mod | TAKE | trivial | none | |
| 69 | #88993 | [XE] March Lords can carry unique gear | mod content | TAKE | trivial | MAGIC.md typo fix near fork's MAGIC-J03 doc edits; clean | |
| 78 | #89438 | Aftershock: Combat Headsets | mod content | TAKE | small | `spells.json` next to fork's removed `protection_from_cold`; clean | Bug: `afs_recon_headset` Deactivate and `revert_to` point at itself |
| 83 | #89513 | [MoM] Fix the Knockdown power | mod bugfix | TAKE | trivial | none | |
| 86 | #89522 | [XE] Elemental samples to essence | mod content | TAKE | trivial | JSON_FLAGS.md one line; clean | |
| 87 | #89525 | (XE) Fist of Taranis magic_type/spell_class | mod bugfix | TAKE | trivial | Same file as fork's MAGIC-J03 effect_str fix, other spell; clean | |
| 94 | #89536 | (XE) Arvore/Salamander selectors need a known spell | mod bugfix | TAKE | trivial | Partial upstream fix of what fork fixed as EOC-B14; compatible, clean | Typo: Salamander gossamer `< 15` vs leveler `< 8` |
| 95 | #89510 | [XE] Ríastrad edits | mod balance | TAKE | trivial | Files fork edited (effect/spell id fixes); clean | |
| 100 | #89535 | [XE] Add Vengeance of the Trees to Arvore | mod content | TAKE-ADJUST | small | **Conflict:** arvore and salamander learning EOCs vs EOC-B14 | Re-apply math weights; new leveler without `false_effect` |
| 102 | #89537 | (XE) paraclesian MAX_MANA enchantments | mod balance | TAKE | trivial | none | |
| 103 | #89542 | [XE] Bloodthorne passive XP fix | mod bugfix | TAKE | trivial | arvore learning EOCs, other hunk; clean | |
| 105 | #89553 | (alt_map_key) new XE oters | mod | TAKE | trivial | none | All ids exist in XE |
| 110 | #89530 | [DinoMod] unattended boiled dino eggs | mod content | TAKE | trivial | none | Step-recipe form passes the fork's step validation |
| 114 | #89551 | Delete empty Aftershock folder | housekeeping | TAKE | trivial | none | Folder has only an orphan file, no modinfo |
| 120 | #89546 | [XE] Elemental magic rework: Sylph | mod rework | TAKE-ADJUST (series) | medium | **Conflict:** `sylph_mutation_spells.json` (MAGIC-J03 Ominous Wind); learning EOCs (EOC-B14) merge clean but wrong | EOC id collides with Dark Days of the Dead; failure chance always ≥ 1 outdoors until #89607 |
| 122 | #89544 | [XE] Rework elemental fae passive spell XP gain | mod rework | TAKE-ADJUST (series) | small | arvore/salamander learning EOCs (EOC-B14); fork's hidden EOC-B14 test | Typo `_xp_multipler`: Arvore/Salamander passive XP is 0 |
| 125 | #89583 | (Magiclysm) min difficulty 1 for spellcasting prof XP | mod bugfix | TAKE | trivial | none | |
| 131 | #89606 | [XE] Fix Sylph casting equation | mod bugfix | TAKE (series) | trivial | jmath.json: clean in sequence; fork's chronomancer fix kept | Leaves the +1 outdoors term; needs #89607 |
| 132 | #89595 | [Martial Mastery] PERK flag on all perks | mod | TAKE | trivial | none | `PERK` is a known character flag |
| 137 | #89608 | [XE] Sylph traits use `"mana": true` | mod | TAKE (series) | trivial | none | `mana`/`activation_msg` supported |
| 138 | #89609 | [XE] Fix Sylph missing _xp_multiplier | mod bugfix | TAKE-ADJUST (series) | small | **Conflict:** `sylph_spell_learning_eocs.json` vs EOC-B14 | |
| 143 | #89597 | Aftershock: Spawn several shells | mod | TAKE | trivial | none | |
| 154 | #89607 | [XE] Sylph Spell rework | mod rework | TAKE-ADJUST (series) | medium | learning EOCs (EOC-B14, 8 new levelers); `obsoletion/spells.json` (fork edits, clean) | Fixes #89546's failure bug; `VERBLE` typo |
| 155 | #89628 | [Sky Island] starter kit clothes fit | mod | TAKE | trivial | none | |
| 157 | #89637 | [Sky Island] no collective center in raids | mod | TAKE with #89195 | trivial | none | Hard dep: copy-from a special only #89195 adds, a load error alone |
| 184 | #89656 | [Magiclysm] Enchanted tempered steelshod quarterstaffs | mod content | TAKE | trivial | none | |

## Sections

### The paraclesian / Sylph series: #89536, #89535, #89542, #89546, #89544, #89606, #89608, #89609, #89607

**Merge order.** Rows 94, 100, 103, 120, 122, 131, 137, 138, 154. Each depends on the previous ones' text, so take them together and in order. #89537 (MAX_MANA) and #89442 (draughts) are independent.

**What the series does.**
- Moves Arvore, Salamander and Sylph passive spell XP to `paraclesian_gramarye_passive_spell_exp(_xp_multiplier)`: gramarye × a multiplier that the caller passes in as the `xp_multiplier` context variable.
- Adds a known-and-below-cap check to the selectors' outer conditions (#89536). In Salamander and Sylph that check sits behind `expects_vars: ["xp_multiplier"]`.
- Rebuilds every Sylph spell on gramarye and school-count math, all capped at level 15, with a new `xedra_sylph_magick` magic type and failure formula.
- Adds 8 Sylph spells and 1 Arvore spell, and obsoletes `lightning_field` and `sylph_crash_of_lightning`.

**The fork's fixes these PRs meet.**

1. **EOC-B14, `9a4e959d`: the selectors ran themselves again.**
   - The six `EOC_*_SPELL_EXPERIENCE_INCREASER_SELECTOR` picked by numeric weight. A leveler whose spell was unknown or capped ran the selector again through `false_effect`.
   - The fork made each weight `( u_spell_level('S') >= 0 ) * ( u_spell_level('S') < CAP ) * W` and deleted every leveler's `false_effect`.
   - Test: `tests/audit_eoc_dialogue_test.cpp` `eoc-b14_xedra_spell_selectors_dont_run_themselves_again`. It is hidden (`[.][xedra_evolved]`).

   **Upstream HEAD still has this bug** in Arvore (26 levelers), Salamander (20) and Sylph (27): numeric weights, and every leveler's `false_effect` reruns the selector. #89536 adds the outer check, which ends the "everything capped" endless loop for Arvore and Sylph, but two cases remain:
   - **Salamander can still loop forever.** Its outer check says `paraclesian_spell_make_gossamer < 15`, but the leveler checks `< 8` and the spell's max_level is 8.
   - **The random walk still goes deep.** With one eligible low-weight spell, each miss adds two levels. My estimate of the chance a run passes the fork's 250-level guard: about 13% (Salamander), 16% (Arvore), 22% (Sylph, total weight 166).

   In the fork that shows the "EOC … is nested 250 deep" error and stops. Upstream's only guard, a 5000-entry callstack prompt, misses `run_eocs` recursion (that is EOC-B14 itself), so upstream can overflow the stack.

2. **MAGIC-J03, `9c38b68a` and `12c7974f`: Ominous Wind.**
   - `effect_str: "morale"` is not an effect, and the fork's `check_spell_effect_str` now rejects it at load.
   - Its negative `min_damage`/`max_damage` made the attack heal the foes it scares.
   - The fork changed it to `"run"` and dropped the damage.

   **Upstream HEAD still has both bugs.** #89546 keeps `"morale"` and rewrites the damage as negative math (`-8 …`, `-18 …`).

**The three text conflicts, and how to re-apply the fork fixes.**

- **#89535: `arvore_spell_learning_eocs.json` and `salamander_spell_learning_eocs.json`.** The hunks are 26 and 20 at the full merge; they first appear at #89535.
  - Each hunk is one leveler: upstream changed its XP line, and the fork deleted the `false_effect` line right after it. Take upstream's XP line and keep the deletion.
  - The weighted list merges cleanly with the fork's math weights. Add the new entry by hand:
    ```
    [ "EOC_LEVELER_ARVORE_THORN_SPRAY_SPELL", { "math": [ "( u_spell_level('arvore_thorn_spray_spell') >= 0 ) * ( u_spell_level('arvore_thorn_spray_spell') < 15 ) * 6" ] } ]
    ```
  - Create `EOC_LEVELER_ARVORE_THORN_SPRAY_SPELL` without its `false_effect`.
  - Keep #89536's outer condition. It is redundant with the weights and harmless.
  - #89544 later changes these same XP lines again (see below).

- **#89546: `sylph_mutation_spells.json`, one hunk in `ominous_wind`.**
  - The merge keeps the fork's `"effect_str": "run"` on its own, because upstream did not touch that line.
  - In the hunk, take upstream's `max_level: 15` and the math `min_aoe`/`max_aoe`, and **drop** upstream's `min_damage`/`max_damage`.
  - Do not replace the file wholesale with upstream's: that brings back `"morale"` and XE fails to load in the fork.

- **#89609: `sylph_spell_learning_eocs.json`.** 19 hunks at the full merge; the first conflict is at #89609, but #89546 and #89607 change the same file.
  - **#89546 merges cleanly here but is semantically wrong.** It changes every Sylph leveler cap from `dex_to_level(1)`, `int_to_level(1)` and `per_to_level(1)` to `15`. The fork's weights still carry the old caps, along with 20 for Ominous Wind and 35 for Summon Air Sprite and Cloud of Steam. A low-DEX Sylph then never gets passive XP between level 12 and 15.
  - Rewrite all 27 weights with cap 15.
  - Add math weights for `EOC_LEVELER_SYLPH_ELECTRICAL_FIELD` (#89609) and #89607's seven new levelers: INNER_STORM, EPHEMERAL_GRASP, HAMMER_OF_THE_WINDS, LET_ALLIES_FLY, HINDERING_WINDS, DODGE_RANGED_ATTACKS, REDUCE_ENCUMBRANCE.
  - Delete every `false_effect`, and take upstream's `…(_xp_multiplier)` XP lines.
  - The obsolete CRASH_OF_LIGHTNING leveler then weighs 0 by itself.

**Practical route.** Take upstream HEAD's three learning files and re-run the EOC-B14 transformation by script:
- For each weighted entry, read the spell and cap from its leveler's condition, and write the math weight.
- Strip `false_effect` from every `EOC_LEVELER_*`.

That is quicker and safer than resolving about 65 hunks by hand. Then fix the Ominous Wind hunk as above.

**Also fix while adjusting.** These are upstream bugs in the series:
- **`_xp_multipler` typo (#89544).** All 26 Arvore and 20 Salamander levelers read `_xp_multipler`, but the caller sets `_xp_multiplier`. An undefined context variable reads as 0, so Arvore and Salamander get **0 passive XP**. Sylph is spelled right (#89609).
- **Leveler caps above the spell's max_level:**
  - ominous_wind: 20 vs 15
  - cloud_of_steam_sylph: 35 vs 15
  - arvore_summon_wood_spirit: 35 vs 15
  - salamander_summon_fire_spirit: 35 vs 15
  - create_light_salamander: 20 vs 15

  Maxed spells keep winning picks and wasting the XP. Use the spell's max_level as the cap, in both the weight and the leveler condition.
- **`paraclesian_spell_dodge_bonus_sylph` (#89546)** has no `max_level`, so it defaults to 0 and can never level. It also lists `SPAWN_GROUP` twice, a copy-paste slip.
- **The Sylph selector's outer condition** tests `u_spell_level('effect_sylph_levitation')`, an effect id; it should be `sylph_let_allies_fly`. It also omits pick_up_ranged_objects and hammer_of_winds. This is cosmetic once the weights are math.
- **The new Sylph EVENT EOC (#89546) has the id `EOC_DDOTD_REMOVE_PILE_ONE`.** That is the id of Dark Days of the Dead's zombie pile-on EOC (`data/mods/classic_zombies/EoCs.json`). The two mods do not conflict, so in a world with both, the later one silently replaces the other. Rename it, for example to `EOC_SYLPH_TRACK_INDOORS`.
- **`sylph_inner_storm` (#89607)** lists the flag `"VERBLE"`. Unknown spell flags are silently kept as strings, so the spell is not VERBAL. This is a JSON-contract case, not a load error.
- **The Sylph failure formula from #89546** adds `(u_sylph_indoors == 1 ? 0.15 : 1)`, so outdoors the failure chance is always at least 1. #89606 does not fix this; #89607 changes `: 1` to `: 0`. Never stop the series between #89546 and #89607.

**Fork test to update.** `eoc-b14_xedra_spell_selectors_dont_run_themselves_again` would fail on the new data, for three reasons:
- Salamander and Sylph selectors now carry `expects_vars: ["xp_multiplier"]`. `runs_quietly()` uses an empty context, so it logs "Missing required variables".
- XP is now gramarye × multiplier, so the cleared avatar's gramarye 0 gives 0 XP.
- Ominous Wind's cap is now 15, not 20.

Give the test dialogue `xp_multiplier`, set gramarye to at least 1, and change SYLPH to `{ "ominous_wind", 15 }`. The Arvore check also needs the typo fixed.

**Loading, after re-applying the fixes.** The script found nothing new in XE:
- the new spells' `effect_str` (`zapped`, `downed`, the effect ids, the EOCs) resolve
- `xedra_sylph_magick`'s formulas take 0 arguments
- `learn_spells` entries resolve

### #89468 (alt_map_key fortified evac shelter) and #89637 (Sky Island collective center): hard dependencies

- **#89468** adds the `shelter_experienced`, `shelter_experienced_under` and `shelter_experienced_roof` oters, copied from `shelter`. Those oters and their mapgen come from #89291 (row 54, "New Scenario: Experienced", in F_content).
  - Without #89291, `overmap_terrains::check_consistency` reports "No mapgen terrain exists for shelter_experienced" whenever alt_map_key is loaded, which breaks the fork's every-mod-loads check.
- **#89637** does `copy-from: "collective center"`. That overmap special comes from #89195 (row 111, the Collective Center base camp, in F_content). Alone, it fails to load.
- Take each with its dependency or not at all.

### #89402 (XE glamours): soft dependency on #89351

Devana's Balm removes `stab_small`, `clean_stab_small` and `bleeding_stab_small`, which come from #89351 ("stabs", row 6, A_wounds). Without it, `u_remove_wound` with those ids silently does nothing (`bodypart::remove_all_wounds_of_type`), and nothing errors. Veins of Ice now calls the core `EOC_REGEN_MUTATIONS_STOP_BLEEDING_WOUNDS_EFFECTS` with `bodypart` passed, which the fork's version expects.

### #89438 (Aftershock combat headsets): one upstream bug

`afs_recon_headset` (the active OmniWare screen) has `"revert_to": "afs_recon_headset"`, and its "Deactivate" transform targets `afs_recon_headset` too. It is still wrong at upstream HEAD. The screen can never be switched off, and stays "active" when its battery dies. The two-word fix is `afs_recon_headset_off`; take it with the PR.

Everything else resolves:
- pockets (no `ammo_restriction` with size keys)
- the `special_vision` conditions
- the `character_ranged_attacks_*` EVENT EOCs (the fork sends them with the victim as beta)
- `beta_talker`/`alpha_loc`
- the flags it defines

The two 5-second recurring EOCs only do real work while the headset is worn.

### #89374 (DinoMod crazed dinos)

- `GROUP_FERAL` is extended, not overridden, by `mongroup.cpp` `LoadMonsterGroup`. The 19 weight-1 dinos join core's 100 total weight, so in DinoMod worlds about 1 in 6 feral spawns is a crazed dinosaur. That is the mod's intent.
- `mon_ceratosaurus_feral` has `copy-from: "mon_dilophosaurus"`, still so at HEAD. The "crazed ceratosaurus" gets dilophosaurus stats and looks; the copy-from should be `mon_ceratosaurus`.

### #89536 (selector "must know the spell")

Upstream's own partial answer to the EOC-B14 problem: the outer condition now requires a spell that is known and below its cap. It merges cleanly onto the fork, and the fork's math weights stay in place, so nothing breaks; the check just saves work when nothing can improve. It is listed TAKE, not as a duplicate: it does not fix the random walk, and its Salamander gossamer cap (`< 15` against max 8) is wrong. With the fork's weights in place that slip is harmless.

### #88993, #89522, #89525, #89510, #89440, #89542: same files as fork fixes, no conflict

The fork's edits in these files sit in other objects or other lines, and each merges cleanly:
- the Snow-Fox's Stride effect rename and the Summer protection `effect_str`, both MAGIC-J03
- the brownie-blink bounded loop (EOC-B14)
- the MAGIC.md and JSON_FLAGS.md doc edits

#88993's technique conditions (`npc_is_bleeding`, `npc_has_flag`) are known conditions, and the fork evaluates technique conditions with the target as beta (`melee.cpp`).

## Group summary

**Verdict counts:** 37 PRs.
- **TAKE: 32.**
  - 2 only together with a dependency from another group: #89468 needs #89291, #89637 needs #89195.
  - 3 only as part of the XE series: #89536, #89606, #89608.
- **TAKE-ADJUST: 5:** #89535, #89546, #89544, #89609, #89607.
- **OWNER: 0. SKIP: 0.**

**Series and dependencies:**
- **XE paraclesian series:** #89536 → #89535 → #89542 → #89546 → #89544 → #89606 → #89608 → #89609 → #89607. Take all of it, in order. Stopping between #89546 and #89607 leaves Sylph magic always failing outdoors.
- **Outside this group:** #89468 needs #89291 and #89637 needs #89195 (both F_content); #89402 benefits from #89351 (A_wounds).
- Everything else is independent.

**Riskiest items:**
1. **The XE series undoes EOC-B14** unless the fork's selector fix is re-applied to the new versions. Upstream still recurses, and Salamander can still loop forever. The cleanly merging #89546 already leaves the fork's Sylph weight caps out of step with the new leveler caps.
2. **Ominous Wind:** a careless resolution of the #89546 conflict brings back `"morale"`, a load error in the fork, or the negative damage that heals foes.
3. **The `_xp_multipler` typo** zeroes Arvore and Salamander passive XP. It is upstream's bug, and it is still there at HEAD.
4. **The fork's hidden EOC-B14 test** needs updating for `xp_multiplier`, gramarye-scaled XP and the new caps.
5. **The id collision `EOC_DDOTD_REMOVE_PILE_ONE`** between XE and Dark Days of the Dead.

**Recommendation.**
- Take the whole group. The non-XE PRs and the small XE content PRs are clean, load under the fork's checks, and need nothing.
- Bring the XE series in as one unit:
  - take upstream's versions of the three learning-EOC files and re-run the EOC-B14 weight transformation, using the spell's max_level as the cap
  - resolve the Ominous Wind hunk without the damage lines
  - fix `_xp_multipler`, the DDotD id clash, `VERBLE` and the Sylph dodge spell's missing max_level
  - update the hidden test, then run it with `--mods=no_npc_food,xedra_evolved,mindovermatter,magiclysm "[xedra_evolved]"`
- Fold in the one-line upstream bug fixes: the recon headset revert target and the feral ceratosaurus copy-from.

Overall effort is medium, nearly all of it in the XE series.
