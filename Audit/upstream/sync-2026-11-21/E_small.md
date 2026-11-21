# Group E_small: triage (46 PRs)

Every PR here is `INCREMENTAL NEW: none`. The only standalone conflicts (`data/changelog.txt` x3, `tests/catacharset_test.cpp`) come from ordering or a missing upstream prerequisite. None of them touch fork work.

| Row | PR | Title | Kind | Verdict | Effort | Meets fork work | Notes |
|---|---|---|---|---|---|---|---|
| 1 | #89369 | CO2 airguns, itemgroups | content | TAKE | trivial | Two-well air pistol uses `firing_requirements`, which goes through the fork's multi-magazine code (IINF-B05 area). No conflict | Has data bugs, see notes. Removes `ammo_varmint_airgun` and replaces all 3 refs. Widens `box_tiny` to 20 cm |
| 4 | #89354 | Caffeine gum addictive, stim 4->6 | balance | TAKE | trivial | none | `caff_gum` is in modify_morale_test, which only checks flags and morale |
| 5 | #89355 | Stockings encumbrance, shoe/sock mass | balance | TAKE | trivial | none | Wool sock recipe 40->60 knitting |
| 10 | #89397 | Avoid near/far #defines (npc_sleep_test) | build/test | TAKE | trivial | npc_sleep_test.cpp: fork edits are in other hunks, applies clean. **The fork's own new tests use the same names** | See notes |
| 11 | #89392 | Re-add quantity to lab supplies | bugfix (data) | TAKE | trivial | none | Defines all 13 new container groups itself |
| 14 | #89399 | More Exodii CBM dreams | content | TAKE | trivial | none | Snippets only |
| 16 | #89404 | Annealing steel, cementation recipe | content | TAKE-ADJUST | small | none | Drops the carburize-to-mc_steel route, which looks unintended. See notes |
| 17 | #89398 | Snowstorm required_weathers | bugfix | TAKE | trivial | weather_type.json: the fork changed `temperature_modifier` units in other entries. Clean | Snowstorm could never occur before. See notes |
| 18 | #89251 | Bottle gourd seed ratio | bugfix | TAKE | trivial | none | 3 gourds->1 seed is now 1 gourd->3 seeds |
| 21 | #89401 | Weekly changelog to 11-02 | docs | TAKE | trivial | none | First of the 4 changelogs. See notes |
| 26 | #89414 | Add username to comment-commands | upstream-only | SKIP | trivial | none | Would give an upstream user issue-label/close powers in the fork's repo. See notes |
| 32 | #89429 | Sky Island maintainer | mod metadata | TAKE | trivial | none | Text in modinfo.json |
| 33 | #89430 | Scarlet storm offset for Beach Day | content/bugfix | TAKE | trivial | none | Checked the order: the `game_start` EOC sets the storm before scenario EOCs run |
| 36 | #89433 | JSON_STYLE formatter wording | docs | TAKE | trivial | none | |
| 38 | #89372 | Fix language autonyms | bugfix (UI) | TAKE | trivial | src/options.cpp: fork hunks are elsewhere (mod settings). Clean | be/he/uk names only |
| 45 | #89441 | Small armband pouch enc 0->1 | balance | TAKE | trivial | none | |
| 50 | #89352 | More NPC epilogues | content | TAKE | trivial | none | Id gap (no `female_64`), harmless. Some grim text |
| 53 | #89462 | Salt in homes | content | TAKE | trivial | none | Gives salt a 2 ml volume |
| 58 | #89464 | Combat exoskeleton pockets, MOLLE | balance/content | TAKE | trivial | Upstream npc_attack_test uses the exo. NPC toggle uses `get_use("transform")`, so a multi-action `use_action` still works | |
| 61 | #89449 | Sword bayonet loses RAPID | balance | TAKE | trivial | none | |
| 62 | #89474 | Hardened Scavenger loses katana | balance | TAKE | trivial | none | `ch_katana` item stays (fork TEST_DATA uses it) |
| 64 | #89484 | Lore doc: drop blob healing | docs | TAKE | trivial | none | |
| 65 | #89485 | Lore doc: drop radiation mutation | docs | TAKE | trivial | none | |
| 70 | #89491 | npc_test `far` rename (Windows) | build/test | TAKE | trivial | npc_test.cpp: fork edits are elsewhere. Clean | Same issue as #89397 |
| 75 | #89488 | Weekly changelog to 11-09 | docs | TAKE | trivial | none | Needs #89401 first |
| 79 | #89443 | HESCO MIL 3 barriers | content | TAKE | trivial | none | **Soil duplication**: build uses 40 soil, deconstruct gives 100-160 |
| 81 | #89506 | Dynamite description "20 seconds" | docs (data) | TAKE | trivial | Matches `dynamite_act` `countdown_interval` 20 s | |
| 82 | #89507 | Fuse recipe accepts red phosphorus | content | TAKE | trivial | none | `red_phosphorous_small` exists |
| 89 | #89516 | Blanched acorns rework | bugfix/content | TAKE | trivial | Uses unattended recipe steps, which the fork supports (CRAFT-P01 fixed waiting steps) | Migrates `acorns_blanching_active` |
| 92 | #89533 | Fuse amounts in bomb recipes | balance | TAKE | trivial | none | |
| 93 | #89538 | Conversion kits ship stanag10 | balance | TAKE | trivial | Fork's eoc_errands_activity_test makes `stanag30` directly, so it is unaffected | |
| 98 | #89540 | Explosive materials/requirements cleanup | balance/refactor | TAKE | trivial | none | Merges ammonium nitrate pellets into one item. Some recipe ids change. See notes |
| 99 | #89539 | Antiseptic/peroxide descriptions | docs (data) | OWNER | trivial | Tied to the wounds work (A_wounds #89558) | Text now says they are weak disinfectants, but their stats are unchanged |
| 106 | #89521 | Remove "tty" test from #89339 | test | TAKE | trivial | Depends on #89339 (C_refactors) | Moot without #89339 |
| 107 | #89561 | Bump actions for node24 | build/CI | TAKE | trivial | release.yml: 3 checkout pins, clean. Also bumps the reusable workflows and action the release calls | See notes |
| 112 | #89563 | Weekly changelog to 11-14 | docs | TAKE | trivial | none | Needs #89488 |
| 113 | #89532 | Epoxy description typo | docs (data) | TAKE | trivial | none | |
| 116 | #89565 | Mil. explosive recipes add fuse | balance | TAKE | trivial | none | |
| 141 | #89615 | Update shipped Larwick_Overmap | content (gfx) | TAKE | trivial | Release builds already compose this tileset from the external repo | No effect on the release zip, only on local builds |
| 146 | #89613 | Fix discard_invalid_po.sh | build | TAKE | trivial | The fork's release runs this script (build-translations.yml) | Old script never discarded anything. See notes |
| 148 | #89619 | OSX dmg format ULFO | build | TAKE | trivial | none | The fork's macOS release is commented out, so no effect |
| 149 | #89632 | kuutekitty to wound reviewers | upstream-only | SKIP | trivial | none | request-review.yml only runs on CleverRaven, so no effect either way |
| 159 | #89614 | Windows: `make -C lang` | build | TAKE | trivial | Changes the fork's Windows release step, release.yml:467 | Same output, compiled in parallel. See notes |
| 170 | #89587 | Weekly changelog to 11-16 | docs | TAKE | trivial | none | Needs #89563 |
| 171 | #89592 | Lore: Marloss Evangelists | docs | TAKE | trivial | none | |
| 174 | #89647 | README: drop packaging status | docs | TAKE | trivial | none | Harmless. Distro packages are upstream CDDA anyway |

## Notes on non-trivial items

### #89369: CO2 airguns (TAKE, with data bugs)
- Adds `air_pistol_beretta_9mm`. It has two `MAGAZINE_WELL` pockets (pellet mag and a CO2 cartridge) and fires through `firing_requirements`. This is the multi-magazine path the fork already fixed (IINF-B05, reload checks per pocket). It should work, but spot-check reloading the CO2 well in game.
- Data that looks wrong. None of it is fixed at upstream HEAD:
  - The gun has `"weight": "2400 g"` and `"longest_side": "85 mm"`. Its own magazine is 134 mm long.
  - Generic Guns: the migration id is `"air_pistol_beretta_9mm_mag "` with a trailing space, in gg_magazines_migration.json, so it never matches.
  - The GG `air_pistol` copies the vanilla pocket `item_restriction: [air_pistol_beretta_9mm_mag]`, so the GG `air_pistol_mag` cannot be loaded into it.
  - The two bugs cancel out today: vanilla mags stay vanilla and still fit.
  - `co2_12g_cartridge_gg` is unused.
- Fixing any of these is optional and small.

### #89397 and #89491: `near`/`far` in tests (TAKE both; follow-up in the fork)
- With MSVC, `pch/main-pch.hpp` includes `platform_win.h`, and so `<windows.h>`, which defines `near` and `far` as empty macros.
  - CMake: the PCH is a PUBLIC precompile header on `cataclysm-tiles-common`, and the test target links it.
  - The .sln: `Cataclysm-test-vcpkg-static` force-includes the PCH through Cataclysm-pch.props.
  - Upstream MSVC CI does not catch this because it builds with `CDDA_USE_CCACHE`, which turns the PCH off. The fork's release does not build tests.
- **The fork's own new tests use the same names and will fail to compile in a local Windows test build:**
  - `tests/audit_math_talkers_test.cpp:509`
  - `tests/audit_perf_combat_test.cpp:867`
  - `tests/eoc_errands_ui_test.cpp:180`
  - `tests/grapple_counter_test.cpp:240,531,532`
  - `tests/grapple_eoc_test.cpp:331`
  - `tests/npc_errand_test.cpp:1021`
  - `tests/npc_json_goal_test.cpp:844`
- Rename them (to `far_pt` and similar) in the same pass. #89397 also covers the second `near`/`far` pair in npc_sleep_test.cpp (lines 994-995).

### #89404: steel annealing (TAKE-ADJUST)
- Adds annealing recipes that turn a case-hardened (`ch_`) lump back into low-carbon (`lc_`) steel, and a quenched-and-tempered (`qt_`) lump back into medium-carbon (`mc_`) steel.
- It also moves the `lc_steel_lump` carburizing recipe to a 72 h unattended step with a new `cementation` requirement.
- But it **replaced** the old `mc_steel_lump` recipe instead of adding a second one. Before, that recipe carburized budget or lc steel with coal. Now `mc_steel_lump` comes only from annealing qt steel, or from salvage.
  - lc and hc steel can still be made by carburizing; only mc lost that route.
  - The new lc recipe has `id_suffix: "from_qt_steel"` but takes `ch_steel` lumps, so the name is wrong.
  - This looks like an oversight. I could not confirm intent: the merge is a squash.
- Adjust: re-add the old carburizing `mc_steel_lump` recipe under an `id_suffix` and list it in the nested.json group. Optionally rename the suffix to `from_ch_steel`.
- If the owner prefers upstream's exact design, take it as is.

### #89398: snowstorm (TAKE, real bugfix)
- In `weather_generator::get_weather_conditions` (src/weather_gen.cpp:393), types are tried in priority order, and each needs the current winner to be in its `required_weathers`.
- In cold weather, `snowing` (priority 100) always replaces thunder and lightning first. `snowstorm` (110) required `thunder` or `lightning`, so it could never be chosen.
- With `required_weathers: ["snowing"]`, the snowstorm can now happen. That also turns on its `u_add_wet` EOC (weather_eocs.json), which touches the LOOP-J03 area.
- No fork test depends on snowstorm never occurring.

### #89414 (SKIP) and #89632 (SKIP)
- `.github/workflows/comment-commands.yml` has no repository guard. Listing an upstream user in `.github/comment-commands.yml` lets them relabel, close or reopen the fork's issues by commenting.
- `request-review.yml` is guarded to `CleverRaven/Cataclysm-DDA`, so #89632 does nothing in the fork.
- Both lists name upstream people. A side note for the owner: consider emptying the comment-commands user list or disabling that workflow in the fork.

### #89443: HESCO barriers (TAKE, flag)
- `material_soil` is 5 L per unit. Building the barrier uses 40 soil (200 L, or 40,000 sand).
- Deconstructing `f_hesco_barrier` returns 100-160 soil, and bashing returns 70-110.
- The wall uses 80 in total and deconstructs to 200-320.
- So each build and deconstruct cycle creates 300-800 L of soil. Soil is cheap, so the impact is low. A fix is to make the deconstruct counts match the inputs.

### #89540: explosives cleanup (TAKE)
- Merges `chem_ammonium_nitrate_pellets` into `chem_ammonium_nitrate` as a `powder` variant with spawn weight 0, and adds a migration for the old id.
- Rescales `volatile_explosive`/`stable_explosive`/`military_explosive` to 10 g TNT equivalent, and adds `volatile_explosive_potent`.
- Recipe ids change:
  - The synthesis recipe becomes `chem_ammonium_nitrate_powder`, because the fork's recipe.cpp:384 appends the variant to the id.
  - `chem_ammonium_nitrate` now names the from-fertilizer recipe.
  - `chem_ammonium_nitrate_from_pellets` is gone.
  - Characters in old saves may lose those learned recipes. Minor.
- MindOverMatter still uses `volatile_explosive`, which still exists.
- TEST_DATA `item_demographics.json` still lists the pellets id, as upstream's does.

### #89539: antiseptic text (OWNER)
- It is part of upstream's wound-cleaning work; its sibling #89558 is in group A_wounds.
- The new text calls antiseptic and hydrogen peroxide poor disinfectants. In the fork, and at upstream HEAD, both still have `disinfectant_power: 4` and `bite: 0.95`.
- Choice: take it with the wounds series, or skip it if the fork keeps the old infection model.

### #89521: tty test (TAKE with #89339)
- It only removes the `[!mayfail]` `tty` test that #89339 (group C_refactors) added. That test fails without a terminal and prints to stdout.
- Take it right after #89339, or skip both.
- It leaves an unused `#include <io.h>` under `_WIN32`.

### #89561: actions bump (TAKE)
- In the fork's release.yml it changes exactly 3 lines: `actions/checkout` v4 to v7.0.1 at lines 46, 101 and 211. They are identical to upstream, so the patch applies clean.
- It also bumps `upload-artifact` v4 to v7 in three files the fork's Windows release uses:
  - `compose-tilesets.yml`
  - `build-translations.yml`
  - the `build-sdl3-shaders` action
- `actions/download-artifact` stays at v4. Upstream HEAD's release.yml runs the same v7-upload/v4-download mix.
- Not bumped by this PR, and still on their old pins in release.yml: `actions/cache` v4, `setup-msbuild` v2, `lukka/run-vcpkg` v11.6, `msys2/setup-msys2` v2, `setup-java` v4. I could not check their Node versions offline.
- The other 25 files are upstream CI. The fork never changed them, so they are harmless.

### #89614, #89613, #89619: build scripts
- **#89614** changes the fork's Windows release step at release.yml:467 from `lang/compile_mo.sh all` to `make -C lang -j$NUMBER_OF_PROCESSORS`.
  - `lang/Makefile` writes the same `lang/mo/<lang>/LC_MESSAGES/cataclysm-dda.mo` that `build-scripts/windist.ps1:56` copies. `make` is already installed by the msys2 step (release.yml:260).
  - Result: faster, same output. If `NUMBER_OF_PROCESSORS` were unset, `-j` alone still works.
- **#89613** fixes the script that the fork's release runs after `tx pull`.
  - The old loop used `git diff --name-only lang/po/*.po`, which never lists the untracked `.po` files that `tx pull` downloads, so it discarded nothing.
  - The new loop checks every file with `msgfmt -f` and deletes the bad ones.
  - With no TX_TOKEN, only the tracked empty `lang/po/placeholder.po` exists. I checked that it passes `msgfmt -f`, so that case still exits 0.
  - It matters only if the fork has a TX_TOKEN.
- **#89619** only affects `make dmgdist` (macOS), which the fork does not build.

### #89615: overmap tileset
- release.yml downloads the composed `tileset-*` artifacts into `gfx/`. `compose-tilesets.yml` builds Larwick_Overmap from I-am-Erk/CDDA-Tilesets master.
- So the Windows release already ships the external version, and this PR only changes local builds.
- Unknown tile ids are dropped at load, a path the fork made safe with REND-B02.

### Changelogs: #89401, #89488, #89563, #89587 (TAKE, in order)
- The fork never changed `data/changelog.txt`.
- The standalone conflicts are only ordering: each weekly entry's context needs the previous one. Applied in sequence they are clean (INCREMENTAL NEW: none).
- No code in the fork reads the file (no `changelog` reference in src/ or the build scripts). It ships in the data folder.
- Keeping it identical to upstream keeps future weekly changelogs, or a later full merge, conflict-free.
- Some lines describe features from other groups the owner may skip: wounds, Labyrinth expansion, game metrics, Marloss evangelists. If that matters, delete those lines after taking the four.

### #89647: README
- The fork's README.md is still upstream's text, with the CleverRaven badges and download links. Removing the distro package list is harmless.
- A nickclysm README is a separate job.

## Group summary

**Verdicts:** 42 TAKE, 1 TAKE-ADJUST (#89404), 1 OWNER (#89539), 2 SKIP (#89414, #89632).

**Chains and dependencies:**
- Weekly changelogs, in order: #89401 → #89488 → #89563 → #89587.
- #89521 needs #89339 (C_refactors).
- #89539 goes with the wounds decision (A_wounds #89558).
- #89397 and #89491 are the same Windows fix in two files.
- #89561 and #89614 both touch release.yml, without conflict.
- The explosive-recipe PRs (#89506, #89507, #89533, #89540, #89565) are one author's series, and each is independent.

**Riskiest items:**
- #89404 silently removes the mc-steel carburizing recipe.
- #89561 changes the pins in the fork's only release workflow. It is low risk because upstream runs the same pins.
- #89443 soil duplication.
- #89369's Generic Guns migration typo.
- Nothing here touches engine code the fork fixed. The only src/ change is the language-name strings in options.cpp.

**Recommendation:**
- Take the whole group in upstream order. Fix #89404 as described, decide #89539 with the wounds group, and skip the two people-list edits.
- Separately, rename the `near`/`far` locals in the fork's 7 new test files listed above, or Windows test builds that use the PCH will break.
