# Backlog

Index of everything known to be built. Items marked (G02) belong to season G02.

## Simulator Core

- [ ] Ballistics model, camera and scenario agnostic
- [x] Ballistics from weapon data: muzzle speed from energy and projectile mass, gravity drop on projectiles and by flight time on the rail hitscan, range falloff (G04)
- [x] Fire control (trigger modes, refire, magazine) as weapon data (G04)
- [ ] Hit scoring service shared by all scenarios (G02)
- [x] One screen space aiming path from the crosshair for mouse, light gun and hardware (G03)
- [x] Style meter as the moral system: clean shooting raises it, a hit on a hostage or bystander collapses it (G04)
- [x] Combo multiplier, controlled pairs, headshots that kill at once, accuracy in the run statistics (G04)
- [x] Hit stop on a kill, directional camera kick in three strengths, kill screen effect, all values in the style data asset (G04)
- [x] Per bone hit zones against the physics asset: head, torso, legs, both arms and the held weapon, each with a damage multiplier, a reaction and a score in DA_HitZones (G04)
- [x] Hit reactions as montages blended over the body, picked by the side of the shot, on a native anim instance without an Anim Blueprint (G04)
- [x] Disarm: a shot on the weapon or the weapon arm drops it with physics, removes the threat without a kill and scores above a kill (G04)
- [ ] Session statistics (hits, misses, reaction times)
- [ ] Scenario package format: GameMode, data assets, input context (G02)

## Scenarios

- [ ] Western: saloon doors, undead bandits, living customers must not be shot
- [ ] Zombie: first entertainment scenario, shared core for three sub modes
- [ ] Zombie first person sub mode
- [ ] Zombie top down sub mode
- [ ] Zombie target range sub mode
- [ ] Olympic: precision, real physics, no time dilation
- [ ] Target Shooting mode: precision, reaction time and target discrimination; the door range levels play it today
- [ ] Tactical Training mode: scenarios with offenders, hostages and bystanders; the hostage taker and the disarm exist on the door range and the rail
- [ ] Zombie mode as a finished scenario; Lvl_ZombieTest is a test arena with three composed waves
- [ ] Bird Shooting mode: leading a moving target on a ballistic path
- [ ] Laser Tag mode: match data, replays and rankings shared with real events
- [ ] Play Together: local and online multiplayer, cooperative and competitive
- [ ] Scenario selection screen in the style of arcade lightgun cabinets

## Modules

- [x] Door module: twelve door circle, three visible, friend or foe under time pressure (G02)
- [x] Door module: draw timing bonus, waiting for the draw scores higher (G02)
- [x] Door module: draw bonus decays with the closing window, a shot on a shutting door earns less (G02)
- [x] Door module: hostile telegraph, rises from a crouch before it is shootable (G02)
- [x] Door module: the telegraph is a pistol draw on a skeletal body, the crouch is gone (G03)
- [x] Door module: night version Lvl_DoorRange_Night with moonlight, warm lanterns, volumetric fog and a graded post process (G03)
- [x] Door module: rare hostage taker, a small exposed zone frees the hostage, a hit on the hostage is the full penalty (G04)
- [ ] Door module: Western dressing (saloon doors)
- [ ] Door module: Zombie dressing
- [x] Encounter module: data driven waves with time, kill and signal triggers run by a director (G02)
- [ ] Horde module: chain ignition crowds
- [ ] On rails module (open option): guided flight or drive with mass targets
- [x] Rail module: spline route driven by its own distance clock, beats, holds and segment hand over (G03)
- [x] Rail cover: hold to take cover, the ride waits, no firing, enemy hits do not land (G03)
- [x] Rail test lit for night with route lanterns, fog and the door range grade (G03)
- [x] Rail reload in cover: entering cover starts it, leaving early leaves it unfinished (G04)
- [x] Rail hostage taker set piece beat with rescue, hostage hit and escape outcomes (G04)
- [ ] Rail branch selection beyond the first next segment
- [ ] Flight range module: targets crossing on ballistic paths
- [ ] Rail fail state when the rider goes down
- [ ] Rail camera turns toward the action of a beat

## Weapons

- [x] Pistol placeholder from the first person template hooked to scoring (G02)
- [x] Point blank shots register: a blocked muzzle path starts the projectile at the view origin (G03)
- [x] Muzzle flash and impact sparks with short lived lights on the projectile weapons and the rail aim, tunable under Shot Feedback (G03)
- [x] Pistol and scattergun with magazine, reload, empty click, switching and their own handling (G04)
- [x] Impact decals on world surfaces (G04)
- [x] Weapon data asset for damage and refire, the rail aim component still carries pistol values (G04); since G04-B03 the rail aim reads the same weapon definitions
- [x] 3R house mark printed on the pistol and the scattergun, beside the weapon name in the HUD and the pause menu loadout, and as stencil and neon dressing in the night range and on the rail crates (G04)
- [x] One weapon actor driven by weapon definitions: single shot with a cycling action or semi auto, draw, recoil with recovery, accuracy cone with bloom, falloff, ballistics from muzzle energy and projectile mass (G04)
- [x] Mount points as the contract for future models: grip, muzzle, magazine, optic and pressure gauge sockets with placeholder positions (G04)
- [x] PCP pressure model: reservoir pressure is the ammunition, energy, drop and spread follow the pressure through curves, timed refills from a limited supply, pressure on the logo arc gauge and a needle gauge on the weapon (G04)
- [x] AP JET I, AP JET II and 3R 9x19 with placeholder bodies; the 3R 9x19 replaces the template pistol as the standard sidearm (G04)
- [ ] Real models for the AP JET family and the 3R 9x19, carrying the mount point sockets
- [ ] Reserved fire mode slot: bursts or another mode once a weapon needs one
- [ ] Digital twin of the real CYB3RGUN weapon
- [ ] Improvised torch attachment: pulsed bursts, five metre flame cone
- [ ] Torch chain ignition between burning enemies
- [ ] Torch fire light attracts enemies
- [ ] Torch self destruction roll with heat bonus and hiss warning
- [ ] Torch radial damage on detonation
- [x] Overclock: charge from clean hits, held input, world dilation with a separate player scale, locked where precision counts (G04)
- [ ] Overclock time dilation, diegetic, coupled to the HOLD trigger

## Enemies

- [x] Placeholder occupants: red spike hostile, green snowman friendly, distinct silhouettes (G02)
- [x] Enemy base class with one damage entry point, data driven definitions and StateTree behaviour (G02)
- [x] Undead shambler and sprinter placeholders built from engine shapes (G02)
- [x] Skeletal bodies on the mannequin for door occupants, shambler and sprinter, friend and foe told apart by posture, scale and material (G03)
- [x] Leg hits stagger the undead and slow their approach (G04)
- [x] Player death and respawn on encounter levels: the zombie test respawns the player after 3 s with the loadout, the rail rider gets back up after 3 s (G04)
- [ ] Realistic character cast on MetaHuman (D-039 to D-042 reserved)
- [ ] Mass Entity switch for crowd sized enemy counts
- [ ] Undead standard horde
- [ ] Undead bear (tank)
- [ ] Undead chicken (swarm)
- [ ] Undead clown (disruptor, horn audio signature)
- [ ] Machine turrets
- [ ] Machine drones

## UI

- [x] Score, wave, hostiles remaining and end of range summary in WBP_DoorRangeHUD (G02)
- [x] Wave, alive, kills and encounter complete summary in WBP_EncounterHUD (G02)
- [x] Crosshair overlay WBP_RailCrosshair drawn at the aim position (G03)
- [x] Settings menu WBP_SettingsMenu: preset, every graphics option, reset, apply, live frame rate, F10 in every level (G03)
- [x] Measured cost per option and the GPU frame time in the settings menu readout (G03)
- [x] Main menu with level selection, startup flow and a pause menu (G03)
- [x] Style HUD: rank, meter, combo, style events, ammunition with the empty cue, Overclock charge (G04)
- [x] End of run summaries add style, accuracy, best combo, controlled pairs, headshots, rescues and penalties (G04)
- [x] Brand colours defined once in MPC_Brand and DA_BrandStyle: cyan brand, magenta counter, red danger, every UI colour read from them (G04)
- [x] Application icon, splash screen and window title from the CYB3RGUN marks (G04)
- [x] Logo crosshair: the ring is the reticle and tightens red while a hostile draws, the arc is the round gauge and the reload in cover, a thin arc holds Overclock, pulse on a hit, flash on a headshot, red collapse on a penalty (G04)
- [x] Holographic HUD: scanlines, a faint flicker and a glitch burst when the player is hit, legibility first (G04)
- [x] Animated menu logo: boots on entry, the ring breathes, the arc turns and spins as the loading indicator (G04)
- [x] Every in-game number formatted with the invariant culture (G04)
- [ ] Settings menu key as a rebindable Enhanced Input action
- [ ] Scenario selection menu
- [ ] Session statistics screen
- [ ] Ironic safety splash screen spoken by the arena AI (open option)

## Audio

- [x] Hit, friendly hit and escape feedback with placeholder engine sounds (G02)
- [ ] Clown horn signature
- [ ] Torch hiss warning before detonation
- [x] Door open and close sounds, placeholder engine noise (G02)
- [ ] Material dependent impact sounds
- [ ] Soundtrack

## Infrastructure

- [x] Door range level Lvl_DoorRange built via MCP (G02)
- [x] Door range settings with per wave difficulty ramp and Training, Standard, Frantic presets (G02)
- [x] Zombie test level Lvl_ZombieTest with the three wave preset DA_Encounter_Test (G02)
- [x] Runtime nav mesh generated around a navigation invoker on the player (G02)
- [x] Rail test level Lvl_RailTest, about 207 m with two held encounter beats (G03)
- [x] Graphics settings: Low to Ultra presets, every heavy feature behind its own option, first run hardware detection (G03)
- [x] Benchmark level Lvl_Benchmark and Bench.Suite, results in docs/benchmark.md (G03)
- [x] Six presets: Ultra as the maximum game preset, Cinematic on the Cine level for capture, measured defaults, detection capped at High above 4 megapixels (G03)
- [x] Resolution scale up to 200 percent with supersampling, Ultra and Cinematic (G03)
- [x] GPU, driver and resolution in every benchmark result line (G03)
- [x] Night benchmark prepared: Bench.Suite night and tools/night_benchmark.ps1, not run (G03)
- [x] Blockout materials: M_Surface and its instances in Core/Materials on floors, walls, doors, crates and posts (G03)
- [x] Shared cyberpunk grade DA_Grade_Cyberpunk placed by ACyberGrade in the night door range, the rail level and the menu, with neon tubes, wet floors and volumetric fog; the day door range stays neutral (G04)
- [ ] Night run: measure the six presets and Ultra at 150 and 200 percent resolution scale, then refresh the menu cost figures
- [ ] Real vegetation content so Nanite Foliage can be measured on foliage it was built for
- [ ] Environment art beyond blockout
- [x] Canonical remote git@github.com:cyb3rgun/thegame.git over SSH, the old repository kept as a fetch only backup (G04)
- [x] README as the project front page with banner, kept current every season (G04)
- [x] README positioning: adults only and no release above the fold, source available and no binary release badges, what the repository is, content tiers, licence and legal notice (G04), replaced in G04-B06 by the product page below
- [x] Source available licence in LICENSE, pending a lawyer's review (G04), replaced in G04-B06 by the Business Source License 1.1
- [x] Business Source License 1.1 in LICENSE: own business use including commercial use at the operator's premises, agreements for hosted services and resale, GPL-3.0 after four years per version (G04)
- [x] LICENSE-CONTENT.md for game content and LICENSE-THIRD-PARTY.md documenting Epic's engine, template content and template source, and the MariaDB licence text (G04)
- [x] README as a product page: neutral badge row with the BSL 1.1 licence, Licensing and Support and Services sections after the technical content, one short legal notice at the bottom (G04)
- [x] docs/content-tiers.md describes the code and content licensing split (G04)
- [x] Content tiers defined in docs/content-tiers.md, closed paths ignored by git, tools/check_tiers.ps1 fails on tracked or staged closed content (G04)
- [x] Open tier runs without the closed tier: door range occupants and the rail hostage taker stand in with primitives, one LogClosedContent line per kind (G04)
- [x] Body look materials moved from Enemies/Bodies to Enemies/Materials, leaving Bodies to the closed tier (G04)
- [ ] Content repository: private repository with the game content folders, laid over a code checkout
- [ ] Lawyer's review of LICENSE, LICENSE-CONTENT.md and LICENSE-THIRD-PARTY.md
- [ ] Contact address for licensing enquiries in LICENSE-CONTENT.md and the README, which point to cyb3rgun.com for now
- [ ] Shop listing for models, content packs, plugins and updates, which the README refers to
- [ ] Epic template content: check the redistribution terms of the committed template folders
- [ ] Body look materials without template parents, so the stand ins keep their colours without the template mannequin materials
- [ ] Nanite on conventional shadow maps: recheck the D3D12 crash and 200 ms hitches after an engine update, until then Nanite runs only with Virtual Shadow Maps
- [ ] USB HID controller input from the real CYB3RGUN
- [ ] IMU aiming input from the real CYB3RGUN
- [ ] Laser tag event fusion
- [ ] Free asset sourcing before any purchase
