# Group A_wounds: upstream wound rework (28 PRs)

Method: read every PR diff. Simulated the cumulative merge with
`git merge-tree --write-tree --merge-base=a1a646d6 refs/fork/ours <C>` at each PR that adds a conflict and at the last PR (#89651), and read the conflict hunks. Checked the fork's side in `src/wound.cpp`, `src/bodypart.cpp`, `src/character_health.cpp`, `src/condition.cpp`, `src/npctalk.cpp`, the Audit and the `tests/audit_*` files. Nothing was built or run.

Corrections to the group notes:
- The fork never changed `src/medical_ui.cpp` (`git diff a1a646d6 refs/fork/ours -- src/medical_ui.cpp` is empty), so every medical_ui hunk in the series applies cleanly.
- The fork's `activity_actor.cpp` changes don't touch `fix_wound_activity_actor`, so it never conflicts cumulatively.
- In this group, the PRs that add the first new conflict are #89386 (condition.cpp), #89489 (doc/JSON/JSON_FLAGS.md) and #89453 (bodypart.cpp, npctalk.cpp, wound.cpp).
  - #89557 and #89573 add no new conflicting file. They only conflict when applied alone, because they depend on earlier PRs. #89557 enlarges the existing wound.cpp and npctalk.cpp hunks.
  - The character_health.cpp conflict comes from row 76 (exertion, another group), not from this series.

| Row | PR | Title | Kind | Verdict | Effort | Meets fork work | Notes |
|---|---|---|---|---|---|---|---|
| 6 | #89351 | stabs | content | TAKE | trivial | none | New stab wound family (bleeding → stab → clean → bandaged) and its fixes; stab damage now makes wounds. Each stab wound lists `block` twice. Base of the data chain |
| 9 | #89386 | (wound severity) | feature | TAKE-ADJUST | small | condition.cpp `f_bodypart_is_bleeding` vs EOC-B13 guard | Adds `severity`, `u_is_bleeding` object form; bleeding = BLEED + blood vitamin. **Bug:** registered `jarg::array`, so `{ "u_is_bleeding": "MAJOR" }` is always false |
| 20 | #89408 | Shrink wound fix options + some UI improvements | feature (UI) | TAKE | trivial | none (activity_actor.cpp overlap is elsewhere) | Merges identical fixes into "xN"; menu reopens after a fix. Nits: `int amount` uninitialised from old saves; reopens for NPC patients |
| 22 | #89415 | Account for stabs in the wound cleansing/bleeding/supernatural healing EoCs | content | TAKE | trivial | none | Needs #89351. Also retunes the cut thresholds of the healing pool |
| 30 | #89423 | Some types of anatomy can't get bruises | content | TAKE | trivial | flags.json overlap, merges clean | `BRUISE_IMMUNE` for bark/stone/latex/water skins and stoneskin |
| 31 | #89422 | small wound tweaks | balance | TAKE | trivial | none | Severity on bleeding wounds, drops the unused BLEED `amount`, medical UI sorted and coloured by severity. Needs #89351, #89386, #89408 |
| 34 | #89431 | Bleeding condition improvements | content (EOC/UI data) | TAKE | trivial | Xedra mutations.json overlap, merges clean | Switches `bleed`-effect checks to wound checks. Its MAJOR/CRITICAL clauses never fire until the #89386 bug is fixed |
| 37 | #89428 | Infection Immune medical mutants' self-cleaning wounds | content | TAKE | trivial | none | New recurring EOC for INFIMMUNE |
| 39 | #89439 | Add _bodypart_is_wounded condition | feature (EOC) | TAKE | trivial | condition.cpp (next to the #89386 hunk, no new hunk) | Returns false on non-characters without the EOC-B13 debugmsg |
| 52 | #89456 | Fix wound update when timer present | bugfix | TAKE | trivial | wound.cpp overlap, merges clean | Wounds with endless healing time but a `progress_timer` now advance. #89457/#89466/#89487/#89489 need it |
| 57 | #89457 | Initial commit (bruise timers) | balance | TAKE | trivial | none | Bruises stage down by timer. Without #89456 they would never heal |
| 59 | #89466 | Lacerations/Scratches stage down when healing | content/balance | TAKE | trivial | none | "Old dressing" stages and change-dressing fixes |
| 63 | #89487 | Stabs and cuts stage down | content/balance | TAKE | trivial | none | Same for stabs/cuts; renames a fix id that clashed |
| 67 | #89467 | Add _bodypart_is_wounded_and_fixable condition | feature (EOC/UI) | TAKE | trivial | none | Sidebar marks limbs with fixable wounds |
| 68 | #89489 | Add vine wounds | content | TAKE-ADJUST | small | JSON_FLAGS.md: fork's WINGS_2 text (CHAR-B05) | Bugs: lower vine limbs now inherit `LIMB_UPPER`; character-level `blacklist_flag: NON_FLESH_LIMB`; descriptions print `%s` |
| 71 | #89453 | wound pain is affected by other stuff | feature/refactor | TAKE-ADJUST | medium | 3 conflicts (CHAR-B01, HLTH-B09, EOC-B13); breaks fork `update_wounds` and 2 audit tests | **Bug:** `evaluate_pain` multiplies by the raw PAIN multiplier (0 by default), so new wounds have 0 pain. Also a per-wound `has_flag` in `get_pain` |
| 72 | #89481 | [Xedra Evolved] Update Arvore regen wounds | mod | TAKE | trivial | none | Adds REGEN_WOUND next to Arvore's REGEN_HP sources |
| 85 | #89492 | Rewrite wound text to be more colorful and immersive | content (text) | TAKE | trivial | none | Text only; needs the earlier data PRs |
| 108 | #89558 | Updated descriptions for wound cleaning agents | content (text) | TAKE | trivial | none | Three item descriptions |
| 109 | #89557 | having wound prevent body part regeneration | feature/balance | OWNER | medium | enlarges the wound.cpp/bodypart.cpp/npctalk.cpp hunks; no new file | Wounds hold back HP until healed; one wound per damage type per hit; `u_add_wound` no longer worsens by default. **Bugs:** a negative heal drains HP; possible null deref |
| 119 | #89566 | Display if a sub limb is wounded in the sidebar | feature (EOC/UI) | TAKE-ADJUST | small | condition.cpp: the sublimb branch uses `who`, which the fork's EOC-B13 version removed | Adds `check_sublimbs` to 3 conditions |
| 123 | #89573 | Expand wound fix skill, allow them to be mandatory or not | feature | TAKE | trivial | none | `skills` goes from object to array, so old-format mod JSON fails to load (no in-tree mod uses it) |
| 126 | #89577 | 48x-Locutus can heal improperly-set broken bones | content | TAKE | trivial | first vanilla wound with PAIN caps; fork's HLTH-B09 semantics apply | Locutus resets broken bones into a fast-healing, very painful wound |
| 130 | #89549 | Medical iuse menu redirects to wounds menu | feature (UI) | TAKE | trivial | none | When the player heals themself, offers wound fixes; body_window shows bleeding wounds |
| 135 | #89601 | More safety on wound healing progress updates | bugfix | TAKE | trivial | duplicates HLTH-B09's `healing_progress` init (they agree) | Clamps the lifestyle factor; healing never runs backwards |
| 147 | #89627 | Fix exposed conduit damaging the right leg twice | bugfix | TAKE | trivial | fixes TERR-B13 (open in fork) | Mark TERR-B13 fixed by upstream |
| 150 | #89617 | Fix wounds with > INT_MAX healing_progress | bugfix | TAKE | trivial | none | Drops garbage-progress wounds from old saves (with a debugmsg). Needs #89601 |
| 178 | #89651 | allow limb scores of wounds to be multiplicative | feature/balance | OWNER | small | fixes HLTH-B19 (open) as a side effect | New `mod`; the new formula hides wound penalties while the limb is above ~75% HP |

## Details

### #89386 (row 9): wound severity (TAKE-ADJUST, small)

**What it does:**
- Adds `wound_type::severity` (MINOR/MAJOR/CRITICAL) to `src/wound.h`/`wound.cpp`.
- Adds `"severity"` to `u_bodypart_is_bleeding`, and a new object form `f_is_bleeding( jo, member, is_npc )` for `u_is_bleeding`.
- `wound_type::assign_bleeding_wound` now needs a BLEED mod_value as well as a `blood`/`redcells` vitamin. I checked every wound in core and mods: none drains blood without BLEED, so nothing changes classification. That includes the fork's BLEEDING_RATE use in `update_wounds`.

**Fork side of the conflict:** `src/condition.cpp:f_bodypart_is_bleeding`.
- EOC-B13 replaced `*d.const_actor( is_npc )` and `who.get_bodypart( bid )->` with `require_character( d, is_npc, "bodypart_is_bleeding" )` plus a null-bodypart `return false`.
- It is tested by `eoc-b13_conditions_with_the_wrong_talker_are_false` (`tests/audit_eoc_dialogue_test.cpp:546-547`, a monster talker and a missing `test_tail`).

**Resolution:**
- Keep both guards, then add the `severity_var` capture and parse, and `&& wd.type->severity >= severity` in the loop.
- Also keep a `const const_talker &who = *d.const_actor( is_npc );` after the guards, because #89566 later reads `who` in this lambda.

**Upstream bug, still at HEAD:** `{"u_is_bleeding", "npc_is_bleeding", jarg::array, ...}`.
- `{ "u_is_bleeding": "MAJOR" }` has a string, not an array, so no parser matches.
- It then falls into the simple-string fallback of `conditional_t::conditional_t( const JsonObject & )`, which builds `conditional_t( "MAJOR" )`. That is always false, with no load error.
- #89431 and #89566 use `{ "u_is_bleeding": "MAJOR" | "CRITICAL" }`, so the sidebar's two upper bleeding tiers and "bleeding and bitten" never show.
- **Fix:** `jarg::member`.
- The fork's EOC-B02 load check covers `read_condition`, `parse_array` and `not`, but not this fallback. Extending it would have caught this. Apply the jarg fix first, or #89431's data stops loading.

**Minor:**
- The object form of `f_is_bleeding` has no `require_character`. It is safe, since non-character talkers return no body parts, but it does not give the EOC-B13 debugmsg.
- `io::string_to_enum<wound_severity>` runs at evaluation time, so a typo in a literal errors on every sidebar redraw instead of at load.

### #89408 (row 20): wound fix menu (TAKE, notes)

The fork never touched `fix_wound_activity_actor`. Two upstream nits, both trivial to fix while bringing it in:
- `int amount;` has no default, so an in-progress ACT_TREAT_WOUND loaded from an older save has an uninitialised count. Make it `int amount = 1;`.
- `finish()` now calls `who.pick_wound_fix( healed_bp )`. The player can start a friendly NPC's treatment from the body status UI (`game.cpp` examine_status → `body_status_ui.cpp` medical tab), so the menu can pop up when the NPC finishes. Guard it with `who.is_avatar()`.

### #89489 (row 68): vine wounds (TAKE-ADJUST, small)

**Fork side of the conflict:** `doc/JSON/JSON_FLAGS.md`. The fork rewrote the `WINGS_2` entry for the CHAR-B05 `can_fly` fix. Keep the fork's line and insert upstream's `PLANT_LIMB` line. `VINE_LIMB` is used but not documented; add it too.

Data bugs, all still at HEAD:
- **Vine limb flags:** `limbs_body_parts.json` deletes the `flags` of `vine_limb_upper_l`, `vine_limb_lower_r` and `vine_limb_lower_l`, so they inherit the list of `vine_limb_upper_r`.
  - The lower vine limbs lose `LIMB_LOWER` and gain `LIMB_UPPER`. Small monsters can no longer hit them; before, they had 3x hit size against small monsters.
  - `vine_limb_lower_l` also gains `IGNORE_TEMP`.
  - Fix: give the lower limbs their own `flags` list with `LIMB_LOWER`.
- **Mend-all bones:** `mend_all_bone_wounds.json` sets `"blacklist_flag": [ "NON_FLESH_LIMB" ]`. That is a character-flag test, and `Character::has_flag` includes body part flags.
  - A MEND_ALL character (Reptilian Healing) with any non-flesh limb gets no broken bones on any limb, because plain `bone_broken` already excludes MEND_ALL.
  - It is redundant with the `blacklist_bp_with_flag` inherited from `bone_broken`. Drop it.
- **Descriptions:** `plant_wounds.json` descriptions contain `%s`, but `wound_type::get_description()` is never formatted, so the medical UI prints a literal `%s`. Also "You %s is broken".

Side effect: every flesh wound now excludes `NON_FLESH_LIMB`, so Xedra's karma arms, devil tail, salamander wings and gossamer wings stop getting wounds.

### #89453 (row 71): wound pain (TAKE-ADJUST, medium)

**What it does:**
- Wound pain is rolled with the PAIN enchantment and the limb's `pain_mod`.
- `wound::get_pain( who )` returns 0 for PAIN_IMMUNE or narcosis.
- The API changes:
  - `wound( id, who, bp )`
  - `bodypart::add_wound( id, who )`
  - `add_or_worsen_wound( id, who )`
  - `get_pain()` becomes `get_pain( who )` / `get_pain_raw()`

**Upstream bug, still at HEAD:** `wound_type::evaluate_pain` does `ret *= who->enchantment_cache->get_value_multiply( PAIN )`.
- `enchant_cache::get_value_multiply` returns the summed `multiply` values, which is 0 when there are none (`src/magic_enchantment.cpp:get_value`).
- So every wound put on an ordinary character has 0 pain.
- Pain Resistant (-0.33) gives negative wound pain, which lowers total pain. Pain-sensitive traits get 25-50%.
- `Character::mod_pain` treats the multiplier as a delta.
- **Fix:** `ret = who->enchantment_cache->modify_value( enchant_vals::mod::PAIN, ret );`, clamped at 0.
- The fork's HLTH-B09 test (`REQUIRE( audit_wound_pain(...) == 2 )`) catches this once its calls are updated.

**Conflicts, which first appear here:**
- **`src/wound.cpp:wound::wound`:**
  - Fork side: HLTH-B09 (`867d4cb1`) added `healing_progress( 0.0f )` to the init list.
  - Upstream changes the signature and `pain( wd->evaluate_pain( who, bp ) )`.
  - Resolution: take upstream's lines and keep the init in each new constructor. #89557 adds a second constructor. #89601 later makes the init redundant with a default member initializer, and the values agree.
- **`src/bodypart.cpp:bodypart::add_or_worsen_wound`:**
  - Fork side: CHAR-B01 (`4c4457f7`) added `if( old_wound == nullptr ) { break; }` before `wound new_wound = wound( wdp.id );`.
  - Upstream changes that line to `wound( wdp.id, who, id )`, and to `..., ddi )` in #89557.
  - Resolution: keep the guard and take the new constructor line.
  - The fork's `remove_wound( wd.type )` and trailing `break` are on lines upstream doesn't touch, so they survive. Upstream HEAD still has the CHAR-B01 crash.
- **`src/npctalk.cpp:f_add_wound`:**
  - Fork side: EOC-B13 (`Character *guy = require_character( d, is_npc, "add_wound" )`, null-bodypart `return`).
  - Upstream has `Character &who = *d.actor( is_npc )->get_character();`.
  - Resolution: keep the guard and pass `guy`.

**Breakage the merge won't show:**
- `Character::update_wounds` (fork HLTH-B09 PAIN cap) calls `wd.get_pain()`, which no longer exists.
  - Use `wd.get_pain_raw()`, the value `mod_pain` changes.
  - Not `get_pain( this )`: it returns 0 under narcosis or PAIN_IMMUNE, so the cap would never stop the growth.
- The tests need the new signatures. After #89557 these are `add_wound( id, who, 0 )` and `add_or_worsen_wound( id, &who, ddi )`.
  - `tests/audit_wounds_blood_magic_test.cpp` (CHAR-B01): `bp.add_wound( id )`, `add_or_worsen_wound( id )`.
  - `tests/audit_health_test.cpp` (HLTH-B09): `add_wound( id )`, `wd.get_pain()`.

**Performance:** `wound::get_pain( who )` runs `who->has_flag( PAIN_IMMUNE )` and `has_effect( narcosis )` once per wound.
- `Character::get_pain` sums all wounds and runs every turn (perceived pain in `update_wounds`) and on UI redraws.
- `has_flag` rescans traits, bionics, effects and body parts (CHAR-P08, still open).
- Hoist the check into `Character::get_pain`.
- The hardcoded narcosis and PAIN_IMMUNE checks copy `Character::mod_pain`.

### #89557 (row 109): wounds hold back HP (OWNER, medium)

**What it does:**
- Each wound gets `hp_damage`, the damage that caused it. It shrinks as the wound heals.
- `Character::regen` caps natural and medicine healing at `hp_max - Σ get_hp_damage()`.
- `apply_random_wound_from_damage` now rolls one wound per nonzero damage type. Before, a second wound came only if the second damage type was at least 75% of the first.
- `u_add_wound` gains `damage_amount`, `damage_type` and `may_worsen`. By default it now adds plainly; before, it called `add_or_worsen_wound`.
- Wound fixes carry `hp_damage` over to the replacement wound.

**Owner choice:** take upstream's design, where wounds hold back HP until treated and mixed-damage hits give more wounds, or stop the C++ chain at #89453.
- Skipping it means hand-porting #89573, #89549 and #89651. They build on its text, and #89651 uses `wound::hp_damage`.
- #89601 and #89617 still apply.

**If taken, fix:**
1. **Negative heal:** in the medicine loop of `regen`, `healing_apply = std::min( roll_remainder( healing ), cap - hp_cur )` goes negative whenever HP is above the cap. That happens after bandage or first-aid HP, or after `u_add_wound` with `damage_amount` on a healthy limb.
   - `heal()` with a negative value removes HP.
   - It also sends `character_heals_damage` with a negative amount, lowers `healed_total`, and raises `damage_bandaged`/`damage_disinfected`.
   - Clamp it at 0. The natural loop already checks `> 0`.
2. **Null deref:** `apply_random_wound_from_damage` calls `populate_potential_wounds` twice and dereferences the second `pick()`.
   - `can_be_applied` rolls `WOUND_CHANCE`, so for a value strictly between 0 and 1 (an EXTERNAL_OPTION mods set) the second list can be empty.
   - Use `*list.pick()`.
3. **DISTRIBUTED_DAMAGE:** the fork's HLTH-B01 fix spreads a hit over the main parts, but the wound on the struck part still holds back the whole hit.
   - With bug 1, that part is even drained down to the cap.
   - Consider holding back only the HP that part lost.
4. **Multiple fixes at once:** in `fix_wound_activity_actor::finish` with `amount > 1`, every added wound copies the first matching wound's `hp_damage`, because removal now happens after adding.
5. **Docs:** `wound_id` is documented as mandatory but is still optional in the code, and `damage_type` is documented as an int.

**Conflicts:** no new file.
- The npctalk `f_add_wound` hunk grows. Keep the fork guard, then use `*guy` for `add_wound` and `guy` for `add_or_worsen_wound`.
- wound.cpp gets a second constructor (keep the init there too).
- bodypart.cpp gains the `ddi` argument.

**CHAR-B01:** still at most one progression per `add_or_worsen_wound` call, but a hit can now make one call per damage type.

### #89566 (row 119): sub-limbs (TAKE-ADJUST, small)

- In the resolved `f_bodypart_is_bleeding`, the new `check_sublimbs` branch reads `who`. Keep a `const const_talker &who = *d.const_actor( is_npc );` after the fork's guard (see #89386).
- Minor performance cost: the sidebar runs several `check_sublimbs` clauses per limb per redraw, and each builds a `get_all_body_parts( only_minor )` vector.

### #89573 (row 123): wound fix skills (TAKE)

- Wound fix `"skills"` changes from `{ "firstaid": 3 }` to `[ { "skill", "level", "time_save", "is_mandatory" } ]`. A third-party mod in the old format fails to load. No in-tree mod defines wound fixes.
- Accepting the object form as well would fit the fork's modding goal.
- The struct default `is_mandatory = false` differs from the loader default `true`; the loader wins.

### #89577 (row 126): Locutus bones (TAKE)

- `bone_broken_healing_locutus` is the first vanilla wound with PAIN `mod_values` caps (`cap_min 0, cap_max 350`).
- Upstream adds pain while the character's *total* pain is in (0, 350). The fork (HLTH-B09) grows *this wound's* pain up to 350. For a single wound the two are about the same.

### #89549 (row 130): first-aid redirect (TAKE)

- Applies only on the "player heals themself" branch of `heal_actor::use_healing_item`, and merges clean with the fork's iuse_actor work.
- `bodypart::has_bleeding_wound` / `Character::has_bleeding_wound` duplicate the fork-fixed `Character::is_bleeding( bp )`. Upstream's `is_bleeding` still returns true (HLTH-B09). The two can be unified.

### #89601, #89617, #89627 (bugfixes, TAKE)

- #89601 adds `float healing_progress = 0.0f;`, the same thing as HLTH-B09's constructor init. The fixes agree; keep either.
- #89617 removes wounds whose `healing_progress` is negative or above INT_MAX. It catches garbage left by the old uninitialised member.
- #89627 is TERR-B13, which is open in the fork. It changes the second `leg_r` to `leg_l` in both blocks of `trapfunc::dissector`. The Audit's broader suggestion, to iterate the main parts, is not done.

### #89651 (row 178): limb score formula (OWNER, small)

**What it does:**
- Adds `mod` to `wound_limb_score`. The mods are summed and applied as ×(1 − Σmod), on top of the flat `value`.
- Moves the medical UI debug view into ImGui tables.
- This fixes HLTH-B19: no more `pop_back` on a possibly empty damage-type string, and no more `ImGui::Separator` inside the string builder.

**Formula change in `bodypart::wound_adjusted_limb_value`:**
- Old: above 75% HP, `val × (1 − Σvalue)`; below 75%, `val × (hp%/0.75 − Σvalue)`.
- New: `(val × hp%/0.75 − Σvalue) × (1 − Σmod)`, clamped to `[0, val]`.
- So above 75% HP, the hp%/0.75 boost (up to 1.33) soaks up the flat penalty. At full HP, a limb with score 1.0 and a 0.1 penalty shows no penalty; the old code gave 0.9.
- With #89557, a fresh wound also lowers the limb's HP cap, which partly offsets this. Still, small wounds (bruises, scratches) on an otherwise healthy limb stop affecting limb scores.

**Owner chooses:** keep upstream's formula, or clamp `percent / 0.75` at 1.0 so wound penalties keep applying at high HP.

**Nits:**
- The header comment says a `mod` of 0.9 multiplies, but the doc and code treat `mod` as the fraction removed.
- The debug tooltip prints `mod` whenever it is `!= 1.f`, and the default is 0.
- HLTH-P12 (`get_limb_scores()` copied per wound per query) is unchanged.

### Data notes (TAKE rows)

- **#89351:** stab damage made no wounds before. Now stabs bleed (-3 to -23 blood/s, like cuts). Each stab wound lists `block` twice in `limb_scores`, so it counts double (still at HEAD).
- **#89431:** conditions that checked the `bleed` effect now check bleeding wounds. Some monster attacks and techniques still apply the `bleed` effect, which these conditions now miss: BombasticPerks `ewww_de_zombie` and the MindOverMatter pyrokinesis cauterize.
- **Ids:** checked every wound id referenced at HEAD in core wounds and the wound EOCs (progression, timers, fix add/remove, EOC `wound_id`s): all resolve. Only the pre-existing bionic `*_bad` wounds never heal and have no fix. HLTH-J04 (an empty `wound_type::check()`) stays open, and the series adds about 30 new ids that nothing validates at load.

## Group summary

**Verdicts:** TAKE 22, TAKE-ADJUST 4 (#89386, #89489, #89453, #89566), OWNER 2 (#89557, #89651), SKIP 0.

**Chains:**
- **C++ core:** #89386 → #89453 → #89557 → #89573 → #89549 → #89601 → #89617 → #89651. #89408 and #89422 provide the medical_ui and activity_actor context that #89453 and #89573 build on.
- **EOC/sidebar:** #89386 → #89431 → #89439 → #89467 → #89566.
- **Data:**
  - #89351 → #89415 → #89428.
  - #89351 + #89386 + #89423 → #89422 → #89457 → #89466 → #89487 → #89489 → #89492, then #89577.
  - #89422's `"severity"` field needs #89386. #89457, #89466 and #89487 need #89456, or their wounds never heal.
- **Stand-alone:** #89456, #89601 (+#89617), #89627, #89558, #89481, #89423, #89408.

**All or nothing?** The data PRs are chained by text and need #89386 and #89456. The C++ side from #89453 on is one unit, and #89557 is the only real decision point. Everything up to #89453 is coherent without #89557. Past it, #89573, #89549 and #89651 would need hand-porting. Taking the whole group in order is the cleanest.

**Fork fixes:** none are undone, and upstream fixes none of CHAR-B01, HLTH-B09 or EOC-B13 itself.
- **Text conflicts:** 5 hunks.
  - `condition.cpp` `f_bodypart_is_bleeding` (EOC-B13).
  - `bodypart.cpp` `add_or_worsen_wound` (CHAR-B01 guard).
  - `wound.cpp` `wound::wound` (HLTH-B09 init).
  - `npctalk.cpp` `f_add_wound` (EOC-B13).
  - `JSON_FLAGS.md` `WINGS_2` (CHAR-B05 doc).
- **Code to move:**
  - The fork's `update_wounds` PAIN cap: `wd.get_pain()` → `wd.get_pain_raw()`.
  - `tests/audit_wounds_blood_magic_test.cpp` and `tests/audit_health_test.cpp`: new API signatures.
- **Open findings this closes:** TERR-B13 (#89627) and HLTH-B19 (#89651).
- **New conditions without the EOC-B13 debugmsg:** `u_is_bleeding` (object form), `u_bodypart_is_wounded` and `u_bodypart_is_wounded_and_fixable`. They are safe, but adding the guard would be consistent.

**Riskiest:**
1. #89453 `evaluate_pain` gives wounds 0 or negative pain.
2. #89557's negative-heal HP drain, the null deref, and the design shift itself.
3. #89386's `jarg::array` makes the severity tiers dead.
4. #89489 turns lower vine limbs into upper limbs.

**Recommendation:** bring in the whole series in upstream order after the owner decides on #89557 and #89651. Apply the adjustments above: the conflict resolutions, the `get_pain_raw` move, the test updates, and the four one-line upstream bug fixes (`jarg::member`, `modify_value` for PAIN, clamping `healing_apply` at 0, reusing `list.pick()`). Overall effort: medium. If the owner declines #89557, take everything else except #89573, #89549 and #89651, or hand-port those three.
