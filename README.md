![CYB3RGUN](.github/assets/banner.png)

# CYB3RGUN

**Digital Shooting Cinema.**
A shooting simulator with exchangeable scenarios, built in Unreal Engine 5.8. Companion software to the CYB3RGUN laser tag hardware.

![Engine](https://img.shields.io/badge/engine-Unreal%205.8-lightgrey.svg)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey.svg)
![Language](https://img.shields.io/badge/code-C%2B%2B%20%2F%20Blueprint-blue.svg)
![Status](https://img.shields.io/badge/status-in%20development-blue.svg)
![Licence](https://img.shields.io/badge/licence-BSL%201.1-blue.svg)
![Website](https://img.shields.io/badge/web-cyb3rgun.com-009FE3.svg)

---

CYB3RGUN is a shooting simulator first and an arcade shooter second. The core is precise: real ballistics, per bone hit resolution, honest scoring and a fire control system modelled on real hardware. On top of that core sit exchangeable scenarios, each with its own rules, targets and look. Target practice and tactical training on one side, undead and flying targets on the other, all running the same engine underneath.

The software exists alongside real hardware. The same trigger modes, the same assist logic and the same law govern both, so what a player learns on screen transfers to the device in their hands. The long term goal is one product in two forms: a simulator that runs anywhere, and a physical weapon that plugs into it.

Built as a system. Every module, weapon, enemy and scenario is a data asset, so new content is content work, and a new game mode takes days instead of months.

Shoot. Train. Improve.

---

## The Law

**NO TRIGGER, NO SHOT.**

The machine calculates everything around the shot: wind, range, drop, the exact moment. The hand takes the shot, always. The same rule governs the hardware and the game, and in the game it is the scoring system itself.

Targets are armed offenders, machines or the undead. Bystanders and hostages are protected: the rule protects the innocent, which is why tactical scenarios have human offenders to face. Hitting a hostage or a bystander is the most expensive mistake in the game, and removing a threat without killing it is the most valuable achievement.

| Action | Score | Effect |
| --- | --- | --- |
| **Disarm** | +300 | Highest single award. A shot on the weapon removes the threat without killing. |
| **Hostage freed** | +250 | A precise shot on the exposed taker ends the standoff. |
| **Weapon arm hit** | +200 | The target drops the weapon and stands unarmed. |
| **Controlled pair** | +80 | Two hits on the same target inside 0.25 s, worth more than two separate hits. |
| **Head shot** | +60 | Ends the target immediately, on top of the kill award. |
| **Kill** | +100 | The baseline for a resolved threat. |
| **Body hit** | +20 | A hit that does not resolve the target. |
| **Leg or off arm hit** | +10 | Effective, but not the point. |
| **Miss** | 0 | Combo drops by two, meter loses ten. |
| **Hostile escaped** | -50 | A drawn hostile that closes without being answered. |
| **Bystander hit** | -300 | Full penalty, the same as a hostage hit. |
| **Hostage hit** | -300 | Meter collapses to zero, combo resets, Overclock empties. |

Every award is multiplied by the combo multiplier, which starts at x1.0 and rises by 0.5 for every three kills or disarms in the combo, up to x4.0. Misses and penalties count at face value. A clean run is worth several times a messy one with the same number of kills.

The door range also keeps its own range score beside the style score: a drawn hostile that escapes costs 50 there, and a hit on a friendly or a hostage costs 150.

---

## Game Modes

| Mode | What it trains | State |
| --- | --- | --- |
| **Target Shooting** | Precision, reaction time and target discrimination against static and appearing targets. | Playable |
| **Tactical Training** | Law enforcement scenarios with armed offenders, hostages and bystanders. Decisions under time pressure. | In progress |
| **Zombie Mode** | Crowd control, positioning and ammunition management against the undead. Cinematic licence. | In progress |
| **Bird Shooting** | Leading a moving target on a ballistic path. Clay discipline in the precision scenario, feathered nonsense elsewhere. | Planned |
| **Laser Tag** | Bridge to the physical product: match data, replays and rankings shared with real events. | Planned |
| **Play Together** | Local and online multiplayer, cooperative and competitive. | Planned |

---

## Modules

A module is a reusable game mechanic. A scenario is the dressing around it. The same module appears in several scenarios wearing different clothes, which is why content scales.

### Door Range

Twelve doors arranged in a circle around the player, two to four of them in play at a time depending on the preset and the wave. A door opens, an occupant appears, and the player has a fraction of a second to decide. An armed offender is a target. Friendlies and hostages are protected.

- **Draw bonus.** Waiting until the target actually commits scores higher than firing at the first sight of movement. The bonus decays as the door swings shut, so a late shot is worth less than a timed one.
- **Telegraph.** Hostiles announce themselves before they become shootable, so the bonus is earnable rather than a reflex lottery.
- **Hostage taker.** A rare occupant stands behind a hostage and exposes only a small target. The reward for the precise shot is high, the penalty for the wrong one is total.
- **Difficulty ramp.** Exposure window, opening cadence and hostile share change per wave, defined in a preset rather than in code.

| Preset | Waves | First wave exposure | Last wave exposure | Hostile share | Openings per wave |
| --- | --- | --- | --- | --- | --- |
| **Training** | 3 | 4.0 s | 3.0 s | 50 to 60 % | 8 to 12 |
| **Standard** | 3 | 2.5 s | 1.6 s | 55 to 65 % | 10 to 14 |
| **Frantic** | 4 | 1.5 s | 0.8 s | 60 to 75 % | 12 to 18 |

A person needs roughly 0.7 to 1.0 seconds to turn to a door and fire. Training leaves a comfortable margin, Standard tightens to about twice that budget, and Frantic ends below it, so a full clear is not expected.

### Rail

The camera follows a spline route. The player aims while the route carries them. This is the arcade light gun tradition, rebuilt with modern rendering and honest ballistics.

- **Beats.** Encounters trigger at distances along the route, not at hand placed volumes. A beat can hold the ride until it is cleared.
- **Cover.** Taking cover pauses the ride, blocks incoming fire, refuses outgoing fire and starts the reload. Leaving cover early leaves the magazine unfinished.
- **Set pieces.** Hostage standoffs and scripted moments sit on the same timeline as ordinary encounters.
- **Segments.** A route segment hands over to the next one when it ends. Choosing between several branches is planned.

### Encounter Arena

Enemies spawn from a data driven director. Waves are composed rather than repeated: build up, peak, then relief. A wave starts after a time, when few enough enemies are left alive, or on a director signal.

### Flight Range

Targets crossing the screen on ballistic paths. Leading a target instead of reacting to one, which is mechanically the opposite of everything above. Planned.

---

## The Simulator Core

The core knows nothing about cameras, scenarios or input devices. It resolves shots, damage and score, and every layer above it is exchangeable.

### Aiming

All aiming resolves through a single screen space path: the crosshair position is projected into the world, and that point is what every shot is aimed at. On the rail the shot is traced from the crosshair itself; the first person weapons fly their projectile toward the same point. The crosshair decides where a shot goes. This is a deliberate architectural decision, because it means mouse, gamepad, a light gun replacement and the planned CYB3RGUN device with gyro aiming all feed the same code.

### Hit Zones

A shot resolves against the bone it hits, not against a capsule. Every zone has its own damage multiplier, reaction and score.

| Zone | Bones | Damage | Reaction | Score |
| --- | --- | --- | --- | --- |
| **Head** | head, neck_02 | x1.0, always lethal | falls away from the shot | 60 |
| **Torso** | pelvis, spine_02 to 05, neck_01, clavicles | x1.0 | torso flinch | 20 |
| **Weapon arm** | upperarm_r, lowerarm_r, hand_r | x0.6 | arm flinch, weapon drops | 200 |
| **Off arm** | upperarm_l, lowerarm_l, hand_l | x0.6 | arm flinch | 10 |
| **Leg** | thighs, calves, feet | x0.75 | heavy stagger, then a limp | 10 |
| **Weapon** | the held weapon itself | x0 | knocked away, target stands unarmed | 300 |

The leg effect is measured, not cosmetic: a shambler's walking speed drops from 130 to zero for the 0.7 second stagger, runs at 55 per cent until three seconds after the hit, then recovers fully. A target on the door range falls instead.

A disarm takes a direct projectile or trace on the weapon or the weapon arm while the holder can still shoot; only direct hits disarm. In practice the weapon itself is the reachable target, because a two handed aiming stance puts the off hand in front of the gun arm.

### Style Scoring

Every resolved action feeds one system. The meter rises with clean work and collapses on a mistake, and the rank is visible at all times.

| Rank | Meter | Meaning |
| --- | --- | --- |
| **COLD** | 0 | The meter is empty. |
| **STEADY** | 20 | Clean work has started to count. |
| **SHARP** | 45 | Accurate work, kept up. |
| **CLEAN** | 70 | Consistent work, no mistakes. |
| **FLAWLESS** | 90 | High combo, no penalties. |

- Combo rises with each clean resolution and multiplies every award.
- A miss costs two combo steps, a penalty resets the combo entirely.
- The meter holds for four seconds after the last action, then decays at 1.5 per second.
- Run statistics are recorded and shown at the end: style points, best rank, accuracy, best combo, controlled pairs, head shots, disarms, leg and arm shots, rescues and penalties.

### Overclock

Diegetic time dilation, produced in the fiction by the weapon electronics rather than by magic. It is the only assist in the game. The precision style data already switches it off, ready for the precision scenario.

| Property | Value |
| --- | --- |
| Charge gain | +4 per hit, +10 per kill, +6 per head shot, +8 per controlled pair, +20 per disarm, +20 per rescue |
| Minimum to trigger | 25 of 100 |
| Drain | 20 per second, about five seconds from full |
| World time | 0.35 |
| Player time | 0.70 |
| Ease in and out | 0.15 s |
| On penalty | charge empties and the effect stops |

Weapons, reloads, refire timing and the crosshair run on the player clock, so aiming stays responsive while the world slows.

---

## Weapons

Weapons are definition assets, not classes. Name, subtitle, mesh, sockets, fire mode, ballistics, handling, audio and effects all live in data, so adding a weapon is content work.

| Weapon | Type | Feed | Refire | Notes |
| --- | --- | --- | --- | --- |
| **3R 9x19** | Service pistol | 17 rounds, magazine | 0.12 s | The standard sidearm. Accurate, fast, the natural partner for controlled pairs. |
| **AP JET I** | Big bore PCP, single shot | Reservoir pressure | 1.1 s cycle | 400 J, damage 140. Heavy, slow, rewards one placed shot. |
| **AP JET II** | Big bore PCP, semi automatic | Reservoir pressure | 0.35 s | 360 J. Faster follow up at the cost of pressure efficiency. |
| **Scattergun** | Short range spread | 6 shells | 0.7 s | Eight pellets in a 5 degree cone, full damage to 7 m. |

All weapon names, manufacturers and models in this game are fictional.

### The Pressure Model

Pre charged pneumatic weapons carry compressed air instead of a magazine, and the air is the ammunition. This is a real mechanic rather than a reskinned round counter, and it changes how the weapon is played.

| Property | Behaviour |
| --- | --- |
| **Consumption** | AP JET I uses 7 per cent of current pressure per shot, AP JET II uses 10 per cent. |
| **Energy curve** | 90 per cent of peak energy when full, 100 per cent around 200 bar, 55 per cent at the firing floor. Modelled on a real valve. |
| **Consequence** | Damage, projectile speed, drop and spread all follow the falling pressure. |
| **Usable shots** | Eleven from 250 bar before the weapon refuses to fire below 120 bar. |
| **Refill** | 3.5 seconds from a carried supply of three. |

A full reservoir is worth more than a topped up one, and the last shots in a fill are measurably weaker than the first. The gauge shows usable pressure rather than absolute pressure, and a needle on the weapon body turns with it.

---

## Feel

Feel is tuned through measured values in one data asset, so it changes without a rebuild.

| Element | Value |
| --- | --- |
| **Hit stop, kill** | 50 ms at world speed 0.02 |
| **Hit stop, head kill** | 80 ms |
| **Camera kick, light** | 0.25 degrees, 0.08 s decay, 18 Hz |
| **Camera kick, medium** | 0.6 degrees, 0.12 s decay, 16 Hz |
| **Camera kick, heavy** | 1.1 degrees, 0.16 s decay, 14 Hz |
| **Kill screen effect** | 0.12 s at strength 0.6 |

The kick turns the view toward the target on screen and upward when the target is dead ahead, then swings back. Impact decals, muzzle light and spark effects complete the picture.

---

## Interface

The crosshair is the brand. The ring is the reticle, the arc around it is the gauge, and the player looks at the mark for the entire session without a logo being pasted anywhere on screen.

| Element | Behaviour |
| --- | --- |
| **Ring** | Tightens and shifts from cyan to red while a hostile draws. Pulses on a hit, flashes on a head shot, collapses red on a penalty. |
| **Outer arc** | Rounds for magazine weapons, usable pressure for pneumatic weapons. Closes as reload progress while in cover. |
| **Thin arc** | Overclock charge, just outside the gauge. |
| **Treatment** | The HUD is projected by the weapon in the fiction, so it flickers, carries fine scanlines and glitches when the player takes damage. |

---

## Graphics

Six presets, and every heavy feature is individually switchable on top of them. Every feature is available, and the player decides what it is worth.

| Preset | Scalability | Global illumination | Shadows | Anti aliasing | Nanite |
| --- | --- | --- | --- | --- | --- |
| **Low** | 0 | off | off | TSR Performance | off |
| **Medium** | 1 | irradiance field | low | TSR Balanced | off |
| **High** | 2 | Lumen | high | TSR Quality | off |
| **Epic** | 3 | Lumen | epic | TSR Quality | off |
| **Ultra** | 3 | Lumen | epic | native | on |
| **Cinematic** | Cine | Lumen | epic | native | on |

Cinematic is labelled as a capture preset for screenshots and video, not for play. Resolution scale runs to 200 per cent for supersampling on Ultra and above.

Measured on an RTX 3090 at 5120 x 1440, each feature measured between two baseline laps inside one process. These are the preset definitions of the published benchmark pass; the current six presets are measured in the pending night run.

| Preset | Average frame time | Frames per second |
| --- | --- | --- |
| **Low** | 5.88 ms | 170 |
| **Medium** | 9.97 ms | 100 |
| **High** | 13.48 ms | 74 |
| **Epic** | 27.65 ms | 36 |

Findings worth recording: global illumination is nearly free here, because Lumen replaces two ambient occlusion passes that the off setting still pays for. Native anti aliasing costs 6.5 ms more than the quality upscaler at this resolution. Nanite is gated behind virtual shadow maps after a reproducible renderer crash without them.

Benchmarks run as a dedicated night pass.

---

## Controls

| Input | Action |
| --- | --- |
| Mouse | Aim |
| Left mouse | Fire |
| R or gamepad X | Reload, first person levels |
| Q or mouse wheel | Switch weapon |
| E or gamepad LB | Overclock, held |
| Right mouse, Space or left gamepad trigger | Cover and reload, on the rail |
| F10 or gamepad menu button | Settings |
| Escape | Pause, and back one level in menus |

Firing, reloading, switching, Overclock and cover are Enhanced Input actions in mapping contexts. Their default keys are still assigned in code, and the menu keys are read directly. Making every key rebindable is planned, and it is what makes the hardware controller a configuration change rather than a rewrite.

---

## Hardware

CYB3RGUN is a physical laser tag weapon system built around an ESP32-S3 with an inertial measurement unit. Software and device converge in four stages.

| Stage | What it means | State |
| --- | --- | --- |
| **1. Shared rules** | The game runs the device's trigger modes, environment profiles and law as game mechanics. | In progress |
| **2. Digital twin** | The virtual weapon behaves exactly like the planned device, so fire control is tested before a servo is fitted. | Planned |
| **3. Device as controller** | The real weapon acts as a USB HID controller. Gyro aiming from the on board sensor, trigger and shoulder contact as inputs, haptics as the return channel. | Planned |
| **4. Convergence** | Match data, replays and rankings shared between physical events and the game. | Planned |

Because all aiming already resolves to a screen position, existing light gun replacements that emulate a cursor are expected to work without special handling.

---

## Architecture

```
+---------------------------------------------------------------+
|                         SCENARIOS                             |
|   Target Shooting / Tactical Training / Zombie / Bird / ...   |
+---------------------------------------------------------------+
|                          MODULES                              |
|     Door Range  /  Rail  /  Encounter Arena  /  Flight        |
+---------------------------------------------------------------+
|                       SIMULATOR CORE                          |
|  Ballistics / Hit zones / Fire control / Style scoring / Feel |
+---------------------------------------------------------------+
|                      INPUT ABSTRACTION                        |
+---------------+---------------+-------------------------------+
|     Mouse     |    Gamepad    |   CYB3RGUN device (planned)   |
+---------------+---------------+-------------------------------+
```

Four rules hold the structure together:

1. **Data over code.** Weapons, enemies, encounters, hit zones, difficulty and feel are data assets. Code defines behaviour, data defines content.
2. **One aiming path.** Every input device resolves to a screen position and from there into the world.
3. **Modules are scenario agnostic.** A module works the same in every scenario that dresses it.
4. **Decisions are recorded.** Every architectural choice is written down with its reason, so it can be revisited rather than rediscovered.

---

## Project Structure

```
thegame/
+-- CYB3RGUN/
|   +-- Config/                 # Project configuration
|   +-- Content/CYB3RGUN/
|   |   +-- Core/               # Shared materials, combat, style, input, menu and level data
|   |   +-- Enemies/            # Enemy bodies, definitions and state trees
|   |   +-- Maps/               # Menu, playable, test and benchmark levels
|   |   +-- Scenarios/          # Game modes, presets and encounters per scenario
|   |   +-- UI/                 # HUD, menus, brand assets
|   |   +-- VFX/                # Niagara systems, decals, neon and the grade
|   |   +-- Weapons/            # Weapon definitions and materials
|   +-- Source/CYB3RGUN/
|       +-- Benchmark/          # Measurement harness
|       +-- Combat/             # Hit zones, reactions, disarm
|       +-- DoorRange/          # Door range module
|       +-- Encounters/         # Encounter director
|       +-- Enemies/            # Enemy base, definitions, StateTree tasks
|       +-- Feel/               # Hit stop, camera kick, Overclock
|       +-- Menu/               # Main menu and level selection
|       +-- Rail/               # Rail module
|       +-- Settings/           # Graphics settings and presets
|       +-- Style/              # Scoring, combos, ranks
|       +-- UI/                 # Menus and HUD widgets
|       +-- VFX/                # Shot feedback and the grade
|       +-- Weapons/            # Weapon framework, pressure model
|       +-- Variant_Shooter/    # First person template code, adapted
|       +-- Variant_Horror/     # Template code, not used by the game
+-- docs/                       # Concept, decisions, conventions
+-- media/                      # Brand source files
+-- tools/                      # Benchmark and maintenance scripts
+-- .github/assets/             # Repository presentation
```

---

## Status

| Component | Status |
| --- | --- |
| Door range module | Working |
| Rail module with cover and reload | Working |
| Encounter director with composed waves | Working |
| Style scoring, combos and ranks | Working |
| Per bone hit zones with reactions | Working |
| Disarm | Working |
| Hostage taker set pieces | Working |
| Data driven weapon framework | Working |
| Pressure model for pneumatic weapons | Working |
| Overclock time dilation | Working |
| Main menu, level selection, pause menu | Working |
| Graphics settings, six presets, benchmark | Working |
| Brand HUD and logo crosshair | Working |
| Holographic HUD treatment | Working |
| Night lighting and cyberpunk grade | Working |
| Impact decals and muzzle light | Working |
| Respawn in encounter levels | Working |
| Realistic character cast | Planned |
| Environments beyond blockout | Planned |
| Soundtrack | Planned |
| Flight range module | Planned |
| Multiplayer | Planned |
| Device as controller | Planned |
| Match data from physical events | Planned |

---

## Development

Built with Unreal Engine 5.8 on Windows. The repository uses Git LFS for binary assets.

**1. Clone**

```
git clone git@github.com:cyb3rgun/thegame.git
cd thegame
git lfs pull
```

**2. Generate project files**

```
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" -projectfiles -project="<path>\CYB3RGUN\CYB3RGUN.uproject" -game -progress
```

**3. Build the editor target**

```
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" CYB3RGUNEditor Win64 Development -project="<path>\CYB3RGUN\CYB3RGUN.uproject" -waitmutex
```

**4. Open**

Open `CYB3RGUN.uproject` directly by double clicking it, or pass its path to the editor. Started from the launcher without a project path, the engine resolves the project after computing its binary paths and reports a false compile error.

**Hard won notes**

- Live Coding crashes on newly reflected types. The working loop is: close the editor, build from the command line, relaunch.
- The editor's background CPU throttle must be off, or a session that is not in the foreground drops to a few frames per second and produces false measurements.
- Adaptive unity builds can hide name collisions until files are committed. Verify a clean tree build before pushing.

See [docs/dev-setup.md](docs/dev-setup.md) for the full local setup.

---

## How This Project Is Built

Development runs in seasons. A season has a goal that fits in one sentence, ends when the goal is met, and leaves its record in the repository: the decisions in the decision log, the state in the season table and the built items in the backlog. Within a season the work is cut into briefings, each a single closed piece with its decisions stated up front and its acceptance criteria written before the work starts.

Three rules have earned their place:

- **Decisions are recorded with their reason.** The decision log is numbered and only grows, so a choice can be revisited instead of rediscovered.
- **Bugs are fixed forward.**
- **Measurements beat opinions.** Every performance claim carries a number, taken on real hardware.

The editor is driven through the official Model Context Protocol plugin, which lets tooling place actors, inspect the running game and capture evidence directly. Verification happens in play.

---

## Documentation

| Resource | Link |
| --- | --- |
| Game concept, the living reference | [docs/game-concept.md](docs/game-concept.md) |
| Code and content licensing split | [docs/content-tiers.md](docs/content-tiers.md) |
| Code licence, Business Source License 1.1 | [LICENSE](LICENSE) |
| Game content licence | [LICENSE-CONTENT.md](LICENSE-CONTENT.md) |
| Third party material | [LICENSE-THIRD-PARTY.md](LICENSE-THIRD-PARTY.md) |
| Architectural decisions with rationale | [docs/decisions.md](docs/decisions.md) |
| Seasons and their goals | [docs/seasons.md](docs/seasons.md) |
| Build backlog | [docs/backlog.md](docs/backlog.md) |
| Naming and folder conventions | [docs/conventions.md](docs/conventions.md) |
| Measured graphics benchmark | [docs/benchmark.md](docs/benchmark.md) |
| Local development environment | [docs/dev-setup.md](docs/dev-setup.md) |

---

## Licensing

The code is published under the [Business Source License 1.1](LICENSE). You may install it, configure it, change it and run it, including in your own commercial operation and for your own guests at your own premises. Four years after each release, that version becomes available under GPL-3.0.

The source code is here. Models, content packs, plugins and updates are available in the shop.

Offering the software to third parties as a service, or reselling it, is what a commercial agreement covers. Game content, including models, environments and the soundtrack, is available through the shop and through service agreements under its own [content licence](LICENSE-CONTENT.md).

The folders that hold code and those that hold game content are listed in [docs/content-tiers.md](docs/content-tiers.md).

---

## Support and Services

IT and More Systems offers the services around running CYB3RGUN:

| Service | What it covers |
| --- | --- |
| **Installation** | Setting up the simulator on your hardware |
| **Configuration** | Scenarios, difficulty, graphics and input set up for your space |
| **Custom scenarios** | Scenarios built for your venue, your training or your event |
| **Content packs** | Characters, environments, enemies and soundtrack |
| **Updates** | New versions of the simulator and the content |
| **Operation** | Running the simulator for you |

Enquiries through [cyb3rgun.com](https://cyb3rgun.com).

---

## Legal Notice

All weapons and manufacturers in this game are fictional. CYB3RGUN is developed for an 18+ age classification. The code is licensed under [LICENSE](LICENSE), game content under [LICENSE-CONTENT.md](LICENSE-CONTENT.md), and third party material, including Epic's Unreal Engine content, under the terms listed in [LICENSE-THIRD-PARTY.md](LICENSE-THIRD-PARTY.md).

---

## Acknowledgments

[Epic Games](https://www.unrealengine.com/) for Unreal Engine, MetaHuman and the sample content that makes a project of this size possible for a small team.

---

*CYB3RGUN is a product of IT and More Systems, Recklinghausen, Germany.*
*Shoot. Train. Improve.*

**CYB3RGUN - No trigger, no shot.**
