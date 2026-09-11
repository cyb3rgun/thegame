# Backlog

Index of everything known to be built. Items marked (G02) belong to season G02.

## Simulator Core

- [ ] Ballistics model, camera and scenario agnostic
- [ ] Fire control (trigger modes, refire, magazine)
- [ ] Hit scoring service shared by all scenarios (G02)
- [x] One screen space aiming path from the crosshair for mouse, light gun and hardware (G03)
- [ ] Session statistics (hits, misses, reaction times)
- [ ] Scenario package format: GameMode, data assets, input context (G02)

## Scenarios

- [ ] Western: saloon doors, undead bandits, living customers must not be shot
- [ ] Zombie: first entertainment scenario, shared core for three sub modes
- [ ] Zombie first person sub mode
- [ ] Zombie top down sub mode
- [ ] Zombie target range sub mode
- [ ] Olympic: precision, real physics, no time dilation
- [ ] Scenario selection screen in the style of arcade lightgun cabinets

## Modules

- [x] Door module: twelve door circle, three visible, friend or foe under time pressure (G02)
- [x] Door module: draw timing bonus, waiting for the draw scores higher (G02)
- [x] Door module: draw bonus decays with the closing window, a shot on a shutting door earns less (G02)
- [x] Door module: hostile telegraph, rises from a crouch before it is shootable (G02)
- [ ] Door module: Western dressing (saloon doors)
- [ ] Door module: Zombie dressing
- [x] Encounter module: data driven waves with time, kill and signal triggers run by a director (G02)
- [ ] Horde module: chain ignition crowds
- [ ] On rails module (open option): guided flight or drive with mass targets
- [x] Rail module: spline route driven by its own distance clock, beats, holds and segment hand over (G03)
- [x] Rail cover: hold to take cover, the ride waits, no firing, enemy hits do not land (G03)
- [ ] Rail branch selection beyond the first next segment
- [ ] Rail fail state when the rider goes down
- [ ] Rail camera turns toward the action of a beat

## Weapons

- [x] Pistol placeholder from the first person template hooked to scoring (G02)
- [x] Point blank shots register: a blocked muzzle path starts the projectile at the view origin (G03)
- [ ] Weapon data asset for damage and refire, the rail aim component still carries pistol values
- [ ] Digital twin of the real CYB3RGUN weapon
- [ ] Improvised torch attachment: pulsed bursts, five metre flame cone
- [ ] Torch chain ignition between burning enemies
- [ ] Torch fire light attracts enemies
- [ ] Torch self destruction roll with heat bonus and hiss warning
- [ ] Torch radial damage on detonation
- [ ] Overclock time dilation, diegetic, coupled to the HOLD trigger

## Enemies

- [x] Placeholder occupants: red spike hostile, green snowman friendly, distinct silhouettes (G02)
- [x] Enemy base class with one damage entry point, data driven definitions and StateTree behaviour (G02)
- [x] Undead shambler and sprinter placeholders built from engine shapes (G02)
- [ ] Player death and respawn on encounter levels
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
- [ ] Settings menu key as a rebindable Enhanced Input action
- [ ] Scenario selection menu
- [ ] Session statistics screen
- [ ] Ironic safety splash screen spoken by the arena AI (open option)

## Audio

- [x] Hit, friendly hit and escape feedback with placeholder engine sounds (G02)
- [ ] Clown horn signature
- [ ] Torch hiss warning before detonation
- [x] Door open and close sounds, placeholder engine noise (G02)

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
- [ ] Night run: measure the six presets and Ultra at 150 and 200 percent resolution scale, then refresh the menu cost figures
- [ ] Real vegetation content so Nanite Foliage can be measured on foliage it was built for
- [ ] Nanite on conventional shadow maps: recheck the D3D12 crash and 200 ms hitches after an engine update, until then Nanite runs only with Virtual Shadow Maps
- [ ] USB HID controller input from the real CYB3RGUN
- [ ] IMU aiming input from the real CYB3RGUN
- [ ] Laser tag event fusion
- [ ] Free asset sourcing before any purchase
