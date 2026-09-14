# CYB3RGUN THEGAME - Game Concept

Living reference of the game concept. This file only grows, it never shrinks.

## Vision

- Digital Shooting Cinema: a shooting simulator with exchangeable scenarios, built in Unreal Engine 5.8, and companion software to the CYB3RGUN laser tag hardware. Shoot. Train. Improve.
- A shooting simulator first and an arcade shooter second. The core is precise: ballistics, per bone hit resolution, honest scoring and fire control modelled on real hardware. Exchangeable scenarios sit on top, each with its own rules, targets and look.
- One product in two forms: a simulator that runs anywhere, and a physical weapon that plugs into it. The same trigger modes, assist logic and law govern both, so what a player learns on screen transfers to the device.
- Built as a system, not as a single game: modules, weapons, enemies, encounters and scenarios are data, so new content is content work, not a rewrite (D-016, D-017, D-037, D-052).

## Product Identity

- This is a shooting simulator with selectable entertainment scenarios, not a game with a simulator mode. The simulator core (ballistics, fire control, hit scoring, statistics) is camera and scenario agnostic. Scenarios are exchangeable packages of GameMode, data assets and input context.
- Companion to the CYB3RGUN hardware. Four stage roadmap: 1 shared universe and rules, 2 digital twin of the real weapon, 3 the real CYB3RGUN as controller via USB HID and IMU, 4 fusion with real laser tag events.
- Scenario selection works like the classic arcade lightgun cabinets: pick a setting, not a realism slider. The Olympic scenario stays physically honest, entertainment scenarios take cinematic licence.
- Hard rule: targets are undead, machines or objects. Never living beings, never humans.
- The game boots into a main menu that is a real lit scene, never into a level (D-036, D-038), and level selection is data (D-037).
- Brand: the logo cyan 009FE3 is the one brand colour, magenta is the counter colour, red is danger and penalty (D-047). The CYB3RGUN mark is the product brand; the 3R mark is the in game manufacturer of our own weapons (D-056).
- Names are our own. Weapon names and subtitles are our own text (D-053). The gun fu style is built from mechanics only, never from another work's expression (D-043). Public facing text never names a real weapon manufacturer, a real weapon model, a film or another game (D-061).
- Target rating: an adult audience, 18+ as the target classification.

## The Law

Decided:

- NO TRIGGER, NO SHOT. The machine calculates everything around the shot: wind, range, drop, the exact moment. It never takes the shot; the hand decides, always. The same rule governs the hardware and the game, and in the game it is the scoring system itself, not a warning screen.
- Force is the last option. Removing a threat without a kill is the most valuable action: a disarm scores above a kill (D-050), and freeing a hostage scores just below it.
- Innocents are never targets. A hit on a hostage or a bystander is the most expensive mistake in the game: the style meter collapses, the combo resets and Overclock empties (D-044).
- Scoring values as built at the close of G04: disarm 300, hostage freed 250, weapon arm hit 200, kill 100, controlled pair 80 (two hits on one target within 0.25 s), head shot 60 on top of the kill, body hit 20, leg or off arm 10, miss 0 with two combo steps and ten meter points lost, penalty 300. Awards are multiplied by the combo multiplier, x1.0 rising by 0.5 per three kills or disarms up to x4.0; misses and penalties are never multiplied.
- The style meter is the moral system and carries five ranks: COLD, STEADY, SHARP, CLEAN and FLAWLESS. It holds for four seconds after the last action and then decays.

## Game Modes

Decided, as the player facing list:

- Target Shooting: precision, reaction time and target discrimination against static and appearing targets. The door range levels play it today.
- Tactical Training: scenarios with offenders, hostages and bystanders; decisions under time pressure.
- Zombie Mode: crowd control, positioning and ammunition management against the undead, with cinematic licence. Lvl_ZombieTest is its test arena.
- Bird Shooting: leading a moving target on a ballistic path. Clay discipline in the precision scenario, feathered nonsense elsewhere.
- Laser Tag: the bridge to the physical product, with match data, replays and rankings shared with real events.
- Play Together: local and online multiplayer, cooperative and competitive.

## Scenarios

Decided:

- Western: saloon doors, undead bandits, living customers must not be shot.
- Zombie: first entertainment scenario. Three sub modes sharing one core: first person, top down, target range.
- Olympic: precision, real physics, no time dilation.
- The precision scenario is the Olympic scenario, a separate scenario that does not exist yet. The day door range is not the precision scenario; it runs the standard style. DA_Style_Precision stays unused until the Olympic scenario is built.
- A module is a reusable mechanic and a scenario is the dressing around it. A module never knows which scenario it is dressed as, so the same module appears in several scenarios.

## Core Modules

Decided:

- Door module: circle of twelve doors, three visible at a time, friend or foe decision under time pressure, waiting for the draw scores higher. Appears in both Western and Zombie scenarios, same mechanic, different dressing. Genre reference: Bank Panic (1984), mechanic only, no assets, no name.
- Horde module: chain ignition crowds.
- On rails module (open option): guided flight or drive sequence with mass targets and sustained fire. Reference: L.A. Machineguns.
- Door range as built: the draw bonus decays with the closing window (D-014), hostiles telegraph with a pistol draw before they are shootable (D-013), a rare hostage taker exposes a small target, and friend or foe reads by posture, scale and material (D-035). Pacing is a per wave ramp in the settings asset (D-012), with Training, Standard and Frantic presets; the door count in play, the exposure window, the cadence and the hostile share change per wave.
- Rail module: a spline plus our own distance scalar (D-020). Beats trigger at distances along the route and can hold the ride; cover pauses the ride, blocks incoming fire, refuses outgoing fire and is the reload (D-045). Hostage standoffs are beats on the same timeline. A segment hands over to the next segment; branch selection is still to come.
- Encounter arena: a director runs encounter data (D-017). Waves are composed, build up, peak, relief, and start on time, on few enough enemies left alive, or on a director signal.
- Flight range: targets crossing on ballistic paths, so the player leads a target instead of reacting to one. Planned.

## Simulator Core

Decided:

- One screen space aiming path from the crosshair (D-019). Mouse, gamepad, light gun replacements that emulate a cursor and the planned device with gyro aiming all resolve to a screen point, and every shot is aimed at the world point under it.
- Hit location matters (D-049): a shot resolves against the bone it hits, with a damage multiplier, a reaction and a score per zone: head, torso, weapon arm, off arm, leg and the held weapon. A head shot kills. A leg hit staggers and slows. The held pistol is the disarm target in practice, because the two handed stance puts the off hand in front of the gun arm (D-058).
- Feel is tuned by measured values in a data asset (D-046): hit stop on a kill, a directional camera kick in three strengths and a kill screen effect.
- Overclock is the only assist: charged by clean hits, kills, head shots, pairs, disarms and rescues, started from a quarter charge, draining in about five seconds, world time at 0.35 and player time at 0.70. Weapons, reloads and the crosshair run on the player clock. A penalty empties it.
- The HUD is diegetic (D-051): the weapon projects it, so it flickers, carries scanlines and glitches under damage. The crosshair is the logo (D-048): the ring is the reticle, the arc is the gauge for rounds or pressure, a thin arc holds Overclock.
- Every number the player reads uses invariant formatting (D-057).

## Weapons

Decided:

- Improvised torch attachment: pulsed bursts in machine gun rhythm, five metre flame cone, chain ignition between burning enemies, fire light attracts enemies. Self destruction rolls a base chance plus a heat bonus that rises with sustained fire, announced by a short hiss before detonation. Radial damage takes nearby enemies with the player.
- Time dilation under our own name (working title Overclock): diegetic, produced by the CYB3RGUN electronics, coupled to the HOLD trigger. Entertainment scenarios only.
- Weapons are data with our own names (D-052, D-053). The first roster: 3R 9x19, the service pistol and standard sidearm; AP JET I, a big bore PCP single shot; AP JET II, the same family as a semi-auto with a faster follow-up and a thirstier valve.
- Pre charged pneumatics spend air, not rounds (D-055): the reservoir pressure sets the muzzle energy, the drop and the spread, and a refill from the carried supply is a timed action with a limited number of charges.
- Placeholder bodies now, real models later (D-054). The mount points are the contract: grip, muzzle, magazine, optic and pressure gauge. Our own weapons carry the 3R mark (D-056).
- Fire modes: single shot with a cycling action, semi auto, and a reserved slot for a later mode. Handling in the definition: draw time, recoil with recovery, an accuracy cone that blooms, range falloff, ballistics from muzzle energy and projectile mass.
- The pressure model as built: AP JET I uses 7 per cent of the current pressure per shot and AP JET II 10 per cent; the valve delivers 90 per cent of peak energy when full, peaks around 200 bar and gives 55 per cent at the 120 bar firing floor; eleven shots from 250 bar; a refill takes 3.5 seconds from a supply of three. A full reservoir is worth more than a topped up one.
- The scattergun stays in the loadout as the short range spread weapon.

## Look

Decided:

- No purchased assets (D-033). The look comes from lighting, materials and post processing first; bodies are the Epic mannequin (D-034).
- A shared cyberpunk grade with neon, wet floors and volumetric fog dresses the night door range, the rail level and the menu. The day door range stays neutral.
- Every heavy rendering feature ships behind a player option (D-025). Six presets from Low to Ultra, with Ultra as the maximum game preset (D-028) and Cinematic for capture only (D-029). Defaults follow measurements taken on this machine (D-026, D-030).
- A realistic character cast on MetaHuman is intended; its decisions are reserved as D-039 to D-042.

## Hardware Roadmap

Decided:

- The CYB3RGUN is a physical laser tag weapon system. Software and device converge in four stages:
  1. Shared rules: the game runs the device's trigger modes, environment profiles and law as game mechanics.
  2. Digital twin: the virtual weapon behaves exactly like the planned device, so fire control is tested before a servo is fitted.
  3. Device as controller: the real weapon acts as a USB HID controller, with gyro aiming from its sensor, trigger and shoulder contact as inputs and haptics as the return channel.
  4. Convergence: match data, replays and rankings shared between physical events and the game.
- Because all aiming already resolves to a screen position, light gun replacements that emulate a cursor are expected to work without special handling.
- Every input is meant to be a rebindable action, so the hardware controller becomes a configuration change rather than a rewrite.

## Bestiary

Decided:

- Undead: standard horde, bear (tank), chicken (swarm), clown (disruptor, horn audio signature).
- Machines: turrets and drones, second enemy family, no living targets.

## Open Options

Not decided:

- Throwing the weapon during the hiss window as an improvised grenade.
- Balloon bunch on the clown as secondary explosion.
- Wrong gas mechanic that ruins the weapon.
- Space scenario.
- Hechtsprung style dodge roll.
- Ironic safety splash screen spoken by the arena AI.

## Assets and Budget

- Asset purchases are deferred until the treasury allows. Free paths first.
- Git LFS budget: the free tier grants 1 GB of storage and 1 GB of bandwidth per month; growing beyond it is the architect's explicit call (D-007).
