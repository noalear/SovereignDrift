# Sovereign Drift handoff (for Naval Sim and any new project)

Written 2026-11-16 from the "Stellar Frontier" Claude project's memory, for
Kyle's new **Naval Sim** project (naval combat: take islands, fight navies,
send out air combat patrols). A new Claude project cannot see the old
project's memory or settings, so everything a fresh thread would otherwise
have to ask Kyle again is collected here.

State of the code this describes: branch `remake` at `804d289` (PR #89,
2026-11-05). Code names below are real identifiers on that commit; check
they still exist before relying on one.

Each section marks how it carries over to a naval game:

- **[reuse]** works as is or with renames.
- **[adapt]** the structure carries over, the content or numbers are space-specific.
- **[space]** space-only; replace or drop.

---

## 1. Kyle's standing rules and working preferences

These were Kyle's instructions to every thread in the old project.

- **Model:** use Opus 5.5 at high effort for all threads (Kyle, 2026-10-31).
- **Merge your own PRs** into the main development branch once the tests pass,
  without waiting for Kyle. Rebase onto the latest branch first and rerun the
  tests. If tests fail or a merge conflict can't be resolved cleanly, don't
  merge; tell Kyle. (Kyle: "use the CLI command to merge the PRs instead of
  waiting for me".) Music exception: adding new tracks is fine, but replacing
  a track Kyle already knows waits for his listen.
- **Tutorial rule:** when adding or changing a player-facing feature, also
  update the voiced tutorial (`src/ui/Tutorial.gd`, `src/ui/TutorialLines.gd`)
  and regenerate the narrator's voice lines with `tools/voice/generate.py`.
- **Godot download:** threads may download Godot 4.5 into their session to run
  tests and builds without asking. (The auto-mode safety check sometimes still
  blocks it until Kyle says so in the thread; cite this rule or ask once.)
- **After every merge:** publish an update patch (section 4). Avoid touching the
  updater script, `export_presets.cfg`, or the `[autoload]`, `[display]` and
  `[rendering]` sections of `project.godot` unless necessary, since those force
  a full new installer that Kyle must upload to Google Drive by hand.
- **Kyle decides balance numbers.** Threads measure (benches, sims) and
  propose; Kyle picks. When he gives a number that measures badly, ship the
  measured value and tell him (example: mass driver energy 200 drained a Scout
  in 3 s, 100 shipped).
- **Ambition:** "keep going, very advanced and high quality"; "great art that
  represents race, ship type and loadout". He wants depth (logistics,
  capture, sensors, crews) over arcade simplicity, but fights that still feel
  good.
- **Visual taste:** no geometric shapes in effects (explosions, wrecks,
  sparks, beams use soft textures or painted art, never `draw_line` sticks or
  polygons). Shields faint (~20% opacity), soft-edged, one neutral colour.
  Projectiles that are metal rounds must not look like lasers. Painted art
  over procedural where it shows.
- **Fair, readable gameplay:** pirate/random enemy spawns "super rare, very
  rare", never right next to the player. Practice modes don't count toward
  the career. Campaign: "challenging but fair", player-timed signals, fully
  scripted missions (no sandbox AI wandering in).
- **Names:** American spelling in player-facing text (armor, defense). Kyle
  hates silly weapon names ("Barb gun" became "Gatling gun"; never
  "grenade" in space). Kyle worries about IP: names must be original (see
  section 10).
- **Accents/voices:** plain English text for TTS; fix single words with
  phonemes. No hand-spelled accent phonemes ("sounds like Belter from the
  Expanse").
- **Music taste:** loves industrial techno; also liked EDM/trance, synthwave,
  orchestral industrial. Hates noisy snares, vowel "wipe" sweeps, long fades,
  drum-roll or "Disney" endings.
- **Communication:** Kyle skims; lead with the result, attach screenshots
  (before/after sheets in project files worked well), keep questions to one
  decision card with a recommendation.
- **Playtesting:** Kyle plays on his Windows PC at 3840x2160. Threads have
  played the game on his PC through computer use when he OKs it; check he
  isn't mid-game before closing it.

## 2. Repositories, branches and where things live

| What | Where |
|---|---|
| Game repo | https://github.com/noalear/SovereignDrift (private; renamed from Stellar-Frontier on 2026-10-31; old name still redirects) |
| Remake branch | `remake` (orphan branch: only the remake, no original code). All feature PRs go into `remake`. Default branch is still `master`. |
| Original editions | `master` (tag `original-1.0`) = the 1997 game running on a Godot 4 shell; `improved` = Enhanced Edition. Not relevant to a new game. |
| Update host | https://github.com/noalear/SFUpdater (public, branch `main`): `updates.json` + `patches/<build>.pck` |
| Installer for players | Google Drive file https://drive.google.com/file/d/1xAcmV3v4agDk25Q6vmtaA_xgXHyXHVy7/view (Kyle uploads new versions by hand: Manage versions > Upload new version) |
| Kyle's art/source folder | `C:\Users\noale\OneDrive\Documents\Stellar Frontier 2\` (ChatGPT reference sheets `ref-*.png`, caller paintings, planet maps); older sheets in `Documents\stellar_frontier_source\` |
| Installed game on Kyle's PC | `%LOCALAPPDATA%\Programs\Sovereign Drift` (from `SovereignDrift-Setup.exe`) |
| Project shared files (old project) | `/mnt/project-files/` in the Stellar Frontier project: screenshots, reference art copies, installer parts, base packs. Not visible from a new project. |

Git notes:
- Claude's git proxy refuses tag pushes, release creation and repo-settings
  writes (HTTP 403). Kyle makes tags/releases himself (he posted installers as
  releases tagged `remake-<base>`; never a tag named like a branch).
- Push big branches with `git -c http.postBuffer=524288000 push`.
- Several threads commit to `remake` in parallel: always rebase right before
  merging, and expect conflicts in `Ship.gd`, `World.gd`, `HUD.gd`,
  `project.godot` and `Net.PROTOCOL`.

## 3. Engine, build and tests

- **Godot 4.5 stable**, standard build (not .NET), GDScript only, renderer
  **GL Compatibility**, no 2D MSAA. Download the Linux editor and export
  templates from the godotengine GitHub releases (`4.5-stable`); templates go in
  `~/.local/share/godot/export_templates/4.5.stable/`.
- **Always run `godot --headless --path . --import` after pulling or
  rebasing.** New `class_name` files or assets leave a stale cache; tests then
  fail with "Identifier not found" or hang (exit 124), and an export can ship
  broken.
- Main scene `res://src/Main.tscn`. Autoloads, in order: `Updater` (must stay
  first), `Settings`, `Keybinds`, `GameData`, `Music`, `Sfx`, `Net`.
- **Tests** are scripts under `tools/` run headless:
  `godot --headless --path . -s tools/<name>.gd` (exit code 0 = pass; check the
  exit code, not just grep for FAIL, because a parse error never prints FAIL).
  - `tools/smoke.gd` plays 6000 frames incl. the shipyard; needs a timeout
    over 900 s.
  - About 60 test scripts: physics_test, capture_test, landing_test,
    economy_test, cargo_test, station_test, trade_test, invasion_test,
    governor_test, sensor_test, armor_test, deflector_test, pdc_test,
    seeker_test, wing_test, wing_outfit_test, warp_test, story_test,
    tutorial_test, shipyard_test, part_info_test, names_test, controls_test,
    death_penalty_test and more (`ls tools/*_test.gd`).
  - Two-process network tests: run the host (`-- --host`, `--host-duel`,
    `--host-story`, `--host-survival`...) and ~4-5 s later a joiner
    (`-- --join=127.0.0.1`); both must print `passed`. They share port 7777, so
    run them one at a time.
  - Sims for balance (run with `--fixed-fps 60`): battle_sim, economy_sim
    (`-- --minutes=30`), fuel_sim, warp_sim, fw_sim, weapon_bench,
    duel_timer, station_timer, starbase_defense.
- **Screenshots** need a virtual display:
  `xvfb-run -a -s "-screen 0 1920x1080x24" godot --path . --rendering-driver opengl3 -s tools/<shots>.gd`.
  Many `*_shots.gd` and gallery tools exist (hull_gallery, mount_gallery,
  menu_shots, munition_shots...). Use 3840x2160 to check how Kyle sees it.
- **Windows build:** `godot --headless --path . --export-release "Windows Desktop" build/windows/<Game>.exe`.
- Test-writing gotchas:
  - Scripts run with `-s` load before autoloads: reach them with
    `root.get_node("/root/GameData")`, never the bare identifier, and don't
    name `class_name` types that depend on autoloads.
  - A freed Node compares `== null`; track objects by `get_instance_id()`.
  - Never compare live sim floats exactly across the network.
  - Tests that change settings must back up and restore `user://settings.cfg`
    (and set both `Settings.race_id` and the single-player profile's race).
  - Running 4+ Godot tests at once on a 4-core box causes timing failures.
  - The headless window is 64x64: tests click with `root.push_input(ev, true)`.
- **Known flaky tests on `remake` itself** (rerun before blaming a change):
  net_smoke trade/shipyard/engine checks (~1 in 10), tutorial_test "the jam
  bites" under load, shipyard_test "R / right stick turns mount 4",
  governor_test raider checks, skirmish_test "own side of the arena",
  invasion_test (~3 in 12), pdc_test "two PDCs >= one", wing_test checks.

## 4. Release pipeline: installer + self-updating patches [reuse]

Kyle didn't want to redownload the whole game for each change, and online
players need matching builds. The pipeline:

- `src/autoload/Updater.gd` is the FIRST autoload. In `_init` it reads
  `res://build.json` (written at export, gitignored; absent in the editor and
  tests, so the updater is off there), loads `user://updates/<build>.pck` named
  in `user://updates/patch.json` when its base matches, deletes stale patches.
  At `_ready` it fetches `https://raw.githubusercontent.com/noalear/SFUpdater/main/updates.json`
  (env `SF_UPDATE_URL` overrides), downloads the patch for its base and checks
  sha256. The main menu shows "Build X" plus Restart to update / Get the
  installer.
- Patches are **cumulative against the installer's base** (`godot --export-patch --patches base.pck`),
  split into 90 MB parts (GitHub's 100 MB file cap); cap 500 MB. New scripts
  and new input actions work from a patch.
- `tools/publish_update.sh` with `GODOT=`, `UPDATES=<SFUpdater clone>`,
  `BASES=<folder keeping <base>.pck, <base>.fp and current>`. Then commit and
  push SFUpdater. It **exits 2** instead of cutting a new base when Updater.gd,
  `[autoload]`/`[display]`/`[rendering]` or the engine changed (fingerprint);
  pass `--new-base` only when Kyle should reinstall. It then prints
  `NEW_INSTALLER=build/SovereignDrift-Setup.exe` (NSIS script
  `tools/installer/StellarFrontier.nsi`, `apt install nsis`).
- A new installer goes to Kyle's PC as 24 MB parts + a `join-installer.bat`
  (`copy /b` of the parts; regenerate the list when the part count changes),
  because device file copies cap at ~25-30 MB per file and 100 MB per call.
  Kyle uploads it to Drive himself.
- Test installers under Wine first (fresh WINEPREFIX, `wineboot -i`,
  `wine Setup.exe /S`, check AppData/Local/Programs, Desktop, Start Menu). An
  `nsExec taskkill` line once crashed the installer on Kyle's PC.
- Current base: `7708371` (2026-10-31). SFUpdater's `.git` is large (every old
  patch in history); trimming needs a force-push Kyle hasn't approved.

For a new game: either a new update repo (recommended, so the two games'
manifests don't collide) or a subfolder; change `REPO_URL`/`INSTALLER_URL` in
`publish_update.sh` and the URL in `Updater.gd`, and cut a fresh base.

## 5. Code map

About 40,000 lines of GDScript. Largest files: `World.gd` 3.4k,
`Builder.gd` 3.2k, `Ship.gd` 2.5k, `AIPilot.gd` 2.0k, `Net.gd` 2.0k,
`HUD.gd` 1.4k, `ShipArt.gd` 1.4k, `GameData.gd` 1.3k.

| Path | What | Naval |
|---|---|---|
| `src/Main.gd` | boot, menus, starting rounds/modes, pause | reuse |
| `src/Names.gd` | player-facing names (game, empires) + TTS phonemes + `fill()` tokens | reuse |
| `src/autoload/GameData.gd` | all data: hulls, parts (MODULES), munitions (MISSILES), RACES, RANKS, SOL bodies, stock loadouts, `stats_for(design)` = the one source of ship numbers | adapt (contents) |
| `src/autoload/Settings.gd` | settings, single/online profiles, careers | reuse |
| `src/autoload/Keybinds.gd` | rebindable keys (2 kb/mouse + 1 pad slot per action, modifiers), saved diffs only | reuse |
| `src/autoload/Music.gd` | mood-driven music (cruise/tension/battle), hold times | reuse |
| `src/autoload/Sfx.gd` | positional SFX, voice caps, leveled families | reuse |
| `src/autoload/Updater.gd` | self-update (section 4) | reuse |
| `src/physics/` | custom deterministic rigid bodies (PhysicsBody, PhysicsSpace, CollisionHulls) | adapt |
| `src/ships/Ship.gd` | the ship: flight, weapons, energy, fuel, shields, cargo, crews, landing | adapt |
| `src/ships/ShipSystems.gd` | per-subsystem damage placed on the hull art | reuse |
| `src/ships/Sensors.gd` | signature channels, sensor families, contact quality | adapt |
| `src/ships/PointDefence.gd` | PDC turrets shooting down munitions (= CIWS) | reuse |
| `src/ships/AIPilot.gd` | AI states (attack, evade, escort, haul, ferry, board, flee, retreat/repair, raid) | adapt |
| `src/ships/ShipArt.gd`, `ShipPlaces.gd` | painted hull + sockets + turrets at live aim; hand-placed part spots per painting | reuse |
| `src/ships/ShieldBubble.gd`, `shield.gdshader` | shield bubble | space |
| `src/weapons/Missile.gd` | every projectile: guns, missiles, torpedoes, mines, drones; seeking, shoot-down | adapt |
| `src/world/World.gd` | the round: spawns, respawns, loot, refuel, colonies, cargo/trade commands, sensor links | adapt |
| `src/world/Planet.gd` | worlds: colonies, garrisons, buildings, planetary shields (shader-drawn) | adapt (islands) |
| `src/world/Economy.gd` | resources, richness, production, markets | adapt |
| `src/world/Ground.gd` | ground battles, troops, flak, sabotage | reuse for island assaults |
| `src/world/Governor.gd` | AI empire brain: targets, building, recruiting, stations, convoys, job board | reuse |
| `src/world/Wing.gd` | command wings: orders wheel, called wingmen, admirals | reuse (task forces, CAP) |
| `src/world/Capture.gd` | boarding/prize crews/station capture | reuse |
| `src/world/Round.gd`, `Duel.gd`, `Survival.gd`, `Skirmish.gd` | game modes (`World.mode_rules`, `Ship.team()`) | reuse |
| `src/world/SaveGame.gd` | sandbox campaign saves | reuse |
| `src/world/Starfield.gd`, Sun, belt, rings | backdrop | space |
| `src/campaign/` | Story director, StoryOp base, 14 ops, StoryLines | reuse framework, new content |
| `src/net/` | Net autoload (host-authoritative), NetClient prediction, NetCodec, LAN discovery | reuse |
| `src/ui/` | HUD, TacticalMap, Builder (shipyard), PartInfo tips, CargoPanel, Ledger, MainMenu, Tutorial, Comms/CallView, Briefing, OrdersWheel, Scoreboard, Trade | reuse/adapt |
| `tools/` | tests, sims, shot tools, art cutters, sfx/voice/music generators, installer, publish script | reuse |

## 6. Systems, how they work, and what they become at sea

### 6.1 Physics [adapt]
- Custom rigid-body space (`PhysicsBody.gd`, `PhysicsSpace.gd`), NOT Godot
  RigidBody2D, so it is deterministic for netcode prediction. SAT + sequential
  impulses, swept bullets, spatial hash. ~3.8 ms/step with ~130 bodies.
- Collision hulls traced from the painted sprites' alpha (`CollisionHulls.gd`).
- Space-specific: local gravity wells, Kepler rails for moons, sun well,
  asteroid belt force, orbit guard, speed cap as an energy cap in wells, warp.
- Physics interpolation is ON: anything that teleports a body must call
  `reset_physics_interpolation()` afterwards, and moving nodes from `_process`
  looks jerky. Camera follows at the end of `World._physics_process`.
- Impulse per damage point 180. Starbases are kinematic, held by `World._keep_station`.
- Naval: drop gravity/orbits/warp; add water drag (strong lateral, weaker
  forward), turning that depends on speed (rudder), momentum/inertia per
  tonnage, maybe wind and currents, shallow water/islands as static colliders.
  Aircraft can be a separate fast body class that ignores water drag.
- Test: `tools/physics_test.gd` (32 checks).

### 6.2 Ship systems, crews, capture [reuse]
- Systems (engines, reactor, shields, sensors, drive, jammer, one per weapon
  mount) sit where drawn on the hull. Hits spread by a bell curve around the
  impact; damaged <60% = weaker; 0 = offline until damage control (rate scales
  with crew). Engines out = adrift (keeps velocity, no thrust/turn).
- Crews: hits kill crew; disabled = engines or reactor out, or crew < 0.5.
- Boarding (key K): tractor holds the target, Lanchester crew fight, AI ships
  become prizes that fly with you (max 2 per pilot). Stations are boardable
  once breached (shield <= 10%, hull <= 50%).
- Kyle wanted fights to end in disabling and capture more often than
  explosions. Knobs at the top of `ShipSystems.gd` and `Capture.gd`.
- Naval: maps directly (engine room, boilers, rudder, magazines, radar mast;
  boarding parties and prize crews). Flooding could be a new system.

### 6.3 Weapons [adapt]
- Each weapon has a role and a damage type (kinetic, beam, explosive, plasma,
  ion; `GameData.DAMAGE_TYPES`), armor resists per type, `DAMAGE_SCALE`.
- Guns: mass driver (5,000 px range), Gatling, cannon (heavy, recoil, pierce),
  disruptor (free-aim, drains energy). Launchers: torpedo (slow turning, for
  big ships), seeker (heat track), missile rack (radar track), drones, mines.
  Guns can shoot down munitions; PDC part (bay-mounted, auto) downs single
  missiles reliably, so swarms/salvos are the counter (Kyle's balance).
- Guided shots use velocity steering toward an intercept (`Missile.intercept`),
  head cone, lock hold. Ammo + rearm at friendly bases.
- Weapon groups 1-4 (by type), alt-fire group, guns converge on the cursor,
  lasers/disruptors aim at the lock or the mouse.
- Tools: `weapon_bench.gd`, `duel_timer.gd`, `seeker_test.gd`, `pdc_test.gd`.
- Naval: guns, torpedoes, anti-ship missiles, depth charges, mines, CIWS and
  flak carry over almost one for one. Lasers/disruptors/shields are space.

### 6.4 Armor and shields [adapt]
- Armor coats: Plate, Reflective, Ablative, Reactive, Shroud (stealth),
  Bulkheads; resist per damage type, mass, signature, thermal lag. One coat
  per ship.
- Shields: front/rear halves with a focus key, three families (Standard,
  Barrier, Flicker) plus Drengin/Skethra Deflector that bounces gun rounds and
  beams. Shields are space; armor belts, compartments and bulkheads are naval.

### 6.5 Sensors and stealth [adapt]
- Channels "radar", "heat", "em", "temporal" (jumps). Sensor families: optics,
  Radar I-III, Pulse, Lidar (cone), Infrared, EM, Temporal; heat sink.
  Contact quality 3 track / 2 fuzzy / 1 bearing / ghost. Planets block line of
  sight. Targeted Jammer (C), ping (F), silent running/EMCON (Z).
- API: `Ship.signature(channel)`, `add_signature(channel, amount, seconds)`,
  `contact_quality(target)`, `has_track(target, channel)`, `visible_to()`.
- The map only shows ships inside your own sensor range (Kyle's ask).
- Naval: radar, passive sonar, active sonar ping, visual (weather/night),
  ESM (detecting radars). Islands block LOS like planets. Submarines are the
  natural stealth class. This system is one of the most reusable designs.

### 6.6 Command wings [reuse: task forces and air patrols]
- `Wing.gd`: orders wheel (form, attack, cover, hold, engage, home, call,
  dismiss) on key 5 / middle mouse; 6 attack, 7 form. Called wingmen by rank,
  AI admirals call their own, wingmen follow your warp jump, go home to
  repair, have their own purse you can top up (outfit them at the yard).
- Radio voice lines per wingman (WingLines.gd, 4 Kokoro voices).
- Naval: escort groups and **combat air patrols** (launch a flight from a
  carrier, hold/cover/attack orders, bingo fuel return = already there: 12%
  bingo sends a wingman home).

### 6.7 Landing, colonists, ground war [reuse: island assaults]
- Land on worlds (B), carry colonists (1/2). Planetary shield must fall before
  a landing; drop ships carry troops (infantry, armour, commandos); ground
  battles tick in `Ground.gd` with orbital support, flak, sabotage.
  Colonists only take an undefended world at 20:1.
- Planet buildings (slots by size), garrisons, recruiting, upkeep with
  shortages making units desert.
- Naval: islands = planets; shore batteries = colony guns; amphibious
  landings = drop ships; shore bombardment = orbital support.

### 6.8 Economy and logistics (the "Resources" update) [reuse]
- Resources Metals, Ice, Fuel, Food, Exotics, Alloys (refined). Richness per
  world from its data row. Local stockpiles hauled by ships (cargo key L).
- Supply ships, trade ships, a neutral free port, station kits deployed by
  supply ships (mining, skimmer, refinery, depot, sensor, defense, trade,
  shipyard). Bigger hulls need Alloys/Exotics at the yard.
- AI `Governor.gd` per empire: picks targets, builds, recruits, deploys
  stations, forms convoys with escorts, posts a job board players can take.
  Raiders hunt lone haulers. Haulers flee threats (`AIPilot` FLEE).
- Balance via `tools/economy_sim.gd -- --minutes=30` (compare 3+ runs each
  side; variance is big).
- Naval: maps almost exactly (oil, steel, food, munitions; convoys and
  convoy raiding are classic naval warfare).

### 6.9 Death, respawn, careers [reuse]
- Death costs half your credits (into the wreck's credit chip, captor gets it
  on capture); wrecks drop one part. No starbase = no launch until it's
  rebuilt (60 s). Starbases have defense turrets. AI relaunches after 15 s,
  >= 2600 px from the player. Pirates: one raider, first launch 6-12 min,
  returns 10-18 min after dying.
- Careers per profile (single/online), ranks unlock hulls; practice and free
  flight don't record.

### 6.10 Modes [reuse]
- Conquest round (host-authoritative rules, tactical map on Tab, scoreboard).
- Survival (waves vs your starbase), Skirmish (team arena), online Duels
  (free-for-all), Practice duel, Free flight, voiced Tutorial.
- Online: co-op or competitive (`Net.versus`); joiners take AI seats.
- Hooks: `World.mode`, `mode_rules`, `is_arena()`, `Ship.team()` decides
  hostility everywhere (never compare race ids directly).

### 6.11 Multiplayer [reuse]
- Direct IP + LAN discovery (UDP 7778), game port 7777. Godot ENet with
  hand-written RPCs on the `Net` autoload (not MultiplayerSynchronizer).
  Host-authoritative snapshots at 20 Hz, client prediction of own ship,
  100 ms interpolation for others. Single player = host with no clients.
- `Net.PROTOCOL` (31 now) must be bumped on any wire change; parallel threads
  bump it too, so expect rebase conflicts on that line.
- Sync audit tool: `tools/sync_audit.gd` + `tools/sync_audit.py` diff host vs
  client state. Known soft spots: client reconciliation is approximate
  (~15 px), clients don't simulate rocks.
- Join codes/relay, lobby, chat, UPnP, lag compensation are not built.

### 6.12 Campaign framework [reuse framework]
- `src/campaign/Story.gd` (director, OPS list, flags, choices, stardate
  interludes, co-op voting), `StoryOp.gd` (objectives, marks, waypoints,
  `spawn` with meta "story", `scaled(n)` for co-op, `pay()`, `hold_raiders`,
  signals the player triggers with the story key), `ops/*.gd` one file per
  mission, `StoryLines.gd` (speakers, places, lines).
- UI: Comms video-call window with painted callers (talk/blink frames),
  Briefing (key M), StoryChoice cards, StorySaves (unlimited slots with
  screenshots).
- Kyle's campaign answers (Terran): huge scaled-up map where travel takes
  time, each mission mostly scripted in its own corner, co-op only for
  multiplayer, retry most missions, fail-forward set pieces, 3 story choices,
  14 ops, grounded tone, video-call presentation, enemies on calls come
  through clean (no tint/static).
- Rules learned: mission sites away from armed places (`StoryOp.guns_near`),
  story raiders use `ai.duel = true` + `hold_raiders` (or they wander off and
  softlock the op), story mode spawns no sandbox fleets and governors don't
  tick.

## 7. Content pipelines

### 7.1 Art: ChatGPT on Kyle's PC + cutters [reuse]
- Kyle OK'd generating reference art in the ChatGPT desktop app on his PC via
  computer use. Prompts: sheets of 3 ships stacked vertically, nose right,
  black background, "NO weapons" (sockets instead); caller portraits on chroma
  green `#00FF00` as 3-frame sheets (neutral/talk/blink) plus a waist-up body
  with both arms inside the frame.
- Cutters in `tools/art/`: `cut_ship_sheet.py <sheet> <out> <names> <scale>`,
  `fill_hulls.py` (**run on every newly cut hull**: the cutter reads alpha from
  brightness, so dark paint goes see-through), `cut_part.py`, `cut_turrets.py`,
  `cut_systems.py`, `cut_sockets.py`, `cut_munitions.py`, `cut_containers.py`,
  `cut_callers.py`, `rebody_callers.py`, `cut_place.py`, `make_planet_map.py`,
  `make_nebula.py`, `frame_callers.py`.
- HD shipyard paintings: cut at native size then upscaled x4 with
  Real-ESRGAN x4plus through onnxruntime (torch is blocked in the container).
- Hand-placed part spots per painting in `ShipPlaces.gd` (fractions of the
  painting, x from the stern); check with `tools/mount_gallery.gd`.
- ChatGPT app quirks: download from the image viewer's top-bar arrow with the
  window RESTORED (not maximized); type the full path in the Save dialog;
  click download once; the image's corner icon is Share (never make a link);
  its window capture can freeze (toggle maximize); two parallel chats work
  well; "Textinputhost" can steal focus after a dialog.
- Naval art fits the same flow (top-down warships, islands as painted maps,
  aircraft sprites).

### 7.2 Voice: Kokoro local TTS [reuse]
- Kokoro-82M via `pip install kokoro-onnx soundfile scipy`; Apache 2.0,
  offline, 54 voices. Model files (`kokoro-v1.0.onnx`, `voices-v1.0.bin`) from
  the GitHub release `thewh1teagle/kokoro-onnx` `model-files-v1.0` (Hugging
  Face is blocked by the container proxy). Keep them outside the repo
  (`KOKORO_DIR`).
- `python tools/voice/generate.py [ids...]` (GODOT env for the binary;
  `--report`; `--sample <voice> "text"`). Reads lines via
  `tools/voice/export_lines.gd`.
- Chain: 48 kHz, trim, 90 Hz highpass, "clean" (+1.5 dB at 3.2 kHz) or "radio"
  (band-pass + tanh), compressor, -17 LUFS, Ogg q5 ->
  `assets/voice/<set>/<id>.ogg`.
- Narrator Commander Hale: `af_heart`, speed 0.96, clean. Good others:
  am_michael, am_fenrir, bm_george (male); af_bella, bf_emma (female);
  am_onyx for a villain; "radio" fx for comms.
- Kokoro only speaks words (no laughs, sighs, hums). Fix single words with
  `is_phonemes=True`. Spoken text never names keys (keys change).
- Music ducks while the narrator speaks; separate Voice bus.

### 7.3 Sound effects: generated in Python [reuse]
- `tools/sfx/generate.py` + `dsp.py` design every effect (numpy, scipy,
  soundfile, ffmpeg/libvorbis) -> `assets/sfx/<name>_<NN>.ogg`, 48 kHz,
  loudness-matched. Families per item/level, not per race (Kyle).
- Engine lesson ("sounds like a weed-eater"): no fast periodic amplitude
  flutter, no saw/square buzz, no short-delay stereo widening (comb filter);
  weight below 500 Hz, slow random swells only. Measure with spectrograms and
  an envelope-spectrum AM check since nobody can listen in the container.

### 7.4 Music [reuse process]
- Generated in Python, no samples except VSCO-2 CE (CC0) orchestral. Scripts
  in `tools/music/` (STATUS.md, BRIEF.md). Kyle's `midi-genre-remix` skill.
- 18 new tracks + 8 remixes of the original's songs. The 8 remixes must go
  for any paid release (original melodies).
- Usage lesson: 9 parallel render agents hit the account limit in ~20 min;
  2 at a time worked.

## 8. UI patterns worth keeping [reuse]

- Main menu (`MainMenu.gd`): painted key art with slow drift + parallax,
  chrome logo, Orbitron/Rajdhani fonts (OFL), glass panels;
  `MainMenu.make_theme()` restyles other panels.
- Shipyard (`Builder.gd`): pay on Accept with an itemised bill, part preview on
  the slip, mouse-over descriptions (`PartInfo.gd`, numbers pulled live;
  `part_info_test` fails if a part lacks text), keyboard/gamepad navigation,
  wingman tabs.
- HUD: target picture under the readout, radar darts pointing along travel,
  owner-coloured labels (green own / yellow friend / red enemy / grey none),
  empire-coloured pilot names, death screen notes.
- Godot gotchas: full-screen custom-drawn Controls under a CanvasLayer need
  `set_anchors_and_offsets_preset` after `add_child`; any full-screen Control
  with MOUSE_FILTER_STOP blocks mouse fire (use IGNORE for in-flight
  overlays); things drawn on a node with a shader material go through the
  shader (plain `draw_*` on Planet is invisible).

## 9. Lessons and gotchas from the environment

- Claude cloud sessions: files for Kyle's PC must be staged under
  `/mnt/project-files/` then copied with the device tools (<= ~25-30 MB per
  file, 100 MB per call): split builds into 24 MB parts + a `.bat` that joins
  them. Kyle wants a single zip kept, not loose files.
- Git on the network mount is slow: clone into a scratch dir.
- Don't `pkill` by a pattern containing your script name (kills your own
  shell); kill by process name.
- Sims run about 0.4x real time with 3 in parallel on the 4-core container.
- Computer use on Kyle's PC: grant the app and File Explorer; the Claude app
  and browser steal focus (re-run open_application before clicking); the
  orange-red border in screenshots is the computer-use indicator, not the game.
- Kyle declines Remote Control on the wrong folder; use the approved project
  folder.

## 10. IP and licensing (read before any release)

- Sovereign Drift began as a remake of Stellar Frontier (Stardock, 1997).
  Its data tables (`GameData`) were first ported from Stardock's shared-source
  release and later retuned; names were changed (Drengin -> Skethra spiders,
  Arceans -> Ephyri jellyfish, Terrans -> Solborn humans; game title Sovereign
  Drift). Code identifiers and some asset names still use the old names.
- The Stardock Shared Source license is **non-commercial** and forbids
  removing copyright notices. Renaming alone does not clear the IP: a
  commercial release needs Stardock's permission or a clean rewrite of
  anything derived. The 8 remixed original songs must go for a paid release.
- For Naval Sim: if it is ever meant to be sold, prefer writing the data
  tables fresh (hulls, parts, ranks, numbers) rather than editing
  `GameData.gd`, keep the engine-level code (physics, net, updater, UI
  framework, tools) which was written new, and don't carry the old names,
  remixed songs or Stellar Frontier text. Do a real trademark search on the
  new title.

## 11. Space -> naval mapping (starting ideas, not decisions)

| Sovereign Drift | Naval Sim candidate |
|---|---|
| Planets/moons (colonies, garrisons, buildings) | Islands (ports, airfields, shore batteries) |
| Planetary shield must fall before landing | Coastal defenses/air cover must be suppressed |
| Drop ships + ground troops (`Ground.gd`) | Landing craft + marines (amphibious assault) |
| Starbase (home, respawn, yard) | Home port / naval base |
| Stations built from kits | Forward bases, airstrips, fuel depots, radar sites |
| Command wings + called wingmen | Task forces; carrier air wings, CAP and strike flights |
| Drones / drone bay | Carrier aircraft / helicopters |
| PDC | CIWS, flak |
| Torpedoes, mines, missiles, guns | Same, plus depth charges |
| Sensors: radar/heat/EM/temporal; jammer; EMCON | Radar/sonar/visual/ESM; jamming; EMCON; submarines |
| Armor coats | Belt armor, compartments, damage control |
| Shields, warp drive, Sun heat, gravity wells | Drop (space-only) |
| Fuel + refuel at bases | Fuel/oil range, at-sea replenishment |
| Supply/trade ships, convoys, raiders | Merchant convoys and convoy raiding |
| Asteroid belt | Reefs, shallows, weather |

## 12. Open questions for Kyle (not decided here)

- **Repo:** fork `SovereignDrift` (keep history, pull fixes across) or start
  a new repo seeded with the reusable parts? Kyle's call.
- Update host: a new repo like `NavalSimUpdater` vs reusing SFUpdater, and a
  new Drive installer file.
- Camera and scale: top-down 2D like Sovereign Drift, or something else?
- Is air combat flown by the player, or only ordered (CAP/strike orders)?
- Setting: historical (WWII/modern) or fictional nations? This drives names,
  art briefs and IP care.
- Commercial intent: decides how much code/data must be written fresh
  (section 10).

## Appendix A: Project instructions text from the old project

```
When adding or changing a player-facing feature, also update the voiced Terran tutorial (src/ui/Tutorial.gd, TutorialLines.gd; regenerate Hale's voice lines with tools/voice/generate.py) so it covers the feature.

Kyle (2026-10-28): threads may download Godot 4.5 in their session to run tests and builds without asking.

Kyle (2026-10-28): threads merge their own PRs into `remake` once the tests pass (green), without waiting for Kyle. Rebase onto latest remake first and rerun tests; if tests fail or there's a merge conflict you can't resolve cleanly, don't merge, tell Kyle.

Updates (2026-10-28): after merging into remake, message the "Stellar Frontier remake branch" thread so it publishes an update patch to noalear/SFUpdater (tools/publish_update.sh; see memory stellar-frontier-updater). New input actions in project.godot are fine (patches carry them). Avoid touching src/autoload/Updater.gd, export_presets.cfg, or the [autoload], [display] and [rendering] sections of project.godot unless necessary: those force a full new installer, which Kyle must upload to Google Drive by hand (overwriting https://drive.google.com/file/d/1xAcmV3v4agDk25Q6vmtaA_xgXHyXHVy7/view).
```
