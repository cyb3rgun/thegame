# Decisions

Numbered newest first.

## D-062 The README is kept current every season

The README is the project's front door. It is brought up to date at the end of every season, in the same structure as the initial version, so what a visitor reads matches what is built.

## D-061 Public text names no real weapon maker, weapon model, film or game

Public facing text never names a real weapon manufacturer, a real weapon model, a film or another game. Mechanics may be described freely; names are what turn an homage into a false origin claim. This covers the README, the store page, trailers and all marketing copy.

## D-060 The canonical remote is cyb3rgun/thegame over SSH

The canonical remote is git@github.com:cyb3rgun/thegame.git over SSH. The old repository saschadaemgen/CYB3RGUN-THEGAME stays as a backup and receives no further pushes. SSH avoids the token prompts the HTTPS remote required.

## D-059 Encounter levels respawn the player

Encounter levels respawn the player, so an idle player cannot end a test run. Verification must never depend on the tester staying alive. The zombie test puts a fallen player back at a player start with the starting loadout, and the rail rider gets back up where it went down and the ride goes on.

## D-058 The pistol is the disarm target, not the weapon arm

The held pistol is the disarm target. In the two handed aim pose the off hand covers the gun arm, as measured in G04-B02, so a shot at the gun arm lands on the off hand. This is accepted as designed, not a defect; the weapon arm stays a disarm zone wherever it shows.

## D-057 In game numbers use invariant formatting

All numbers shown in the game use invariant formatting. The editor's German locale rendered the style multiplier as "x1,0"; a number the player reads must never depend on the machine's locale.

## D-056 The 3R mark is the in game manufacturer

The 3R mark identifies the in game manufacturer of our own weapons. It appears on the weapon itself, in the weapon menu beside the name, and as world dressing. The CYB3RGUN mark is the product brand, the 3R mark is the fiction's brand, and neither ever substitutes for the other.

## D-055 The PCP model is a real mechanic

The pre charged pneumatic model is a real mechanic, not a reskinned magazine. Reservoir pressure is the ammunition currency, muzzle energy falls as the pressure drops, and refilling is an action with a cost. This is what separates this simulator from every magazine counter shooter.

## D-054 Placeholder bodies now, real models later

Weapons carry placeholder bodies until real models exist. The mount points are the contract: grip socket, muzzle socket, magazine socket, optic mount and pressure gauge location. Any future mesh carrying these sockets drops in without code changes.

## D-053 In game weapon names are our own

The weapon definition holds the name and the subtitle as text, and the names are our own. A future manufacturer licence is then a text and mesh swap, not a rebuild.

## D-052 Weapons are data, not classes

One weapon actor is driven by UWeaponDefinition: name, subtitle, mesh, sockets, fire mode, ballistics, magazine or pressure model, handling, audio and effects. Adding a weapon is content work, never code.

## D-051 The holographic HUD is diegetic

The holographic treatment is diegetic: the weapon projects the HUD, so it flickers, carries scanlines and glitches under damage. It is a property of the fiction, not a filter laid over the screen.

## D-050 Disarming is the highest skill shot

Hitting the weapon or the weapon arm removes the threat without killing, and it scores above a kill, because the product's rule is that force is the last option.

## D-049 Hit location matters

A shot resolves per bone, with a damage multiplier and a distinct reaction per zone. A simulator where every hit does the same thing is not a simulator.

## D-048 The crosshair is the logo

The ring of the logo is the reticle and the arc around it is the gauge, so the player looks at the brand for the whole session without a logo pasted anywhere. It keeps the arcade shrinking reticle: the ring tightens and shifts from cyan to red while a hostile draws.

## D-047 One brand colour, defined once

The brand colour is the logo cyan, hex 009FE3, defined once as a project wide parameter and never typed as a literal. Magenta is the counter colour, and red is reserved for danger and penalties. One source of truth makes a rebrand a single edit.

## D-046 Feel is tuned by measured values in a data asset

Hit stop, camera shake and slow motion are balanced through measured values in a data asset, never through constants scattered in code, so they can be tuned without a rebuild.

## D-045 Cover is the reload trigger

As in the arcade tradition, cover is where the weapon reloads. Reloading is a decision made under fire, not a button pressed in safety, and it gives the rail module its rhythm.

## D-044 The style meter is the moral system

Clean, disciplined shooting scores highest, and hitting a hostage or bystander collapses the meter instantly. The rule that innocents are never targets stops being a penalty line and becomes the core scoring loop.

## D-043 Gun fu is built from mechanics only

The gun fu style is built from mechanics, never from another work's expression. Genre, mood, palette, a shooting stance and the real Center Axis Relock technique are unprotected and free to use. Names, logos, characters, dialogue, specific set designs and any named hotel from an existing film are not, and never enter code, assets, filenames or marketing text. This is a house rule, not a preference.

## D-039 to D-042 reserved

Reserved for the MetaHuman cast briefing, which has not been executed yet. They are recorded when the cast is built; the gap in the numbering is intentional.

## D-038 The menu is a real scene

The menu is a real scene, not a widget on a black screen: a lit 3D background with a slow camera move, the settings menu reachable from it, and the same post process treatment as the game.

## D-037 Level selection is data

Level selection runs through a data asset listing playable entries, not hard coded map names, so adding a scenario later is content work and not a code change.

## D-036 The game boots into a main menu

The game boots into a main menu, never into a level. A product that drops the player into a test map is not a product. The menu is the first thing anyone sees, so it carries the brand.

## D-035 Friend or foe by posture, scale and material

Silhouette still carries the friend or foe distinction (D-013). With bodies instead of shapes, the distinction moves to posture, scale and material, never colour alone.

## D-034 Mannequin bodies replace placeholder primitives

Placeholder primitives are replaced by the Epic mannequin skeletal mesh already present in the project. A real body with a skeleton reads as a game and a cone does not. It is the Epic standard skeleton, so any future animation source retargets onto it.

## D-033 No purchased assets

Everything comes from the engine, the project or free Epic content. The look must come from lighting, materials and post processing first, because that is where the biggest visual gain sits and it costs nothing.

## D-032 Long tests run at night

Benchmarks and other long running tests never run during a working session. They are collected and executed in a dedicated night run briefing. Any piece that would occupy the machine for more than about a minute must be split out and named as such.

## D-031 Hardware detection caps at High on large displays

Hardware detection caps at High for displays above 4 megapixels, because detection placed the 7.4 megapixel development display on the old Epic at 36 fps while High delivers 74. The player can raise it at any time.

## D-030 Defaults follow the measurements

Defaults follow the measurements, never documentation or opinion.

## D-029 Cinematic preset for capture

A separate Cinematic preset carries the engine's Cine scalability level, clearly labelled as intended for screenshots and video capture rather than play. Nothing is removed, it is only named honestly.

## D-028 Ultra is a game preset

Ultra is a game preset, not a film preset. Ultra means maximum game quality: everything on, native resolution, no upscaling, Nanite on, Virtual Shadow Maps Epic, Lumen, MegaLights, fog and effects at their highest playable setting. The engine's Cine scalability level does not belong in a preset a player selects.

## D-027 The Nanite gate stands

Nanite only runs while Virtual Shadow Maps are on. Two of three processes crashed in the D3D12 renderer and every surviving lap hitched above 200 ms. It is a fix for a reproducible engine crash, revisited after an engine update.

## D-026 Feature decisions come from measurements

Feature decisions are made from measurements taken on this machine, never from documentation alone. Every feature gets a benchmark number before it becomes a default.

## D-025 Heavy rendering features ship behind player options

Every heavy rendering feature ships behind a player facing option. Nothing is excluded from the build because it is Beta; it is exposed on the highest preset and can be switched off. Options are what settings menus are for, and a proper menu also reads as a finished product.

## D-024 Input asset prefixes join the conventions

The asset prefixes IA_ (Input Action) and IMC_ (Input Mapping Context) join the conventions table. They are the engine standard and were already followed in practice.

## D-023 Rendering baseline

Rendering baseline for later application: MegaLights (production ready in 5.8), Lumen Lite, TSR, Nanite for static geometry only. Nanite Skeletal Mesh and Nanite Foliage stay off, both are Beta with documented packaged build crashes. Not applied in this piece.

## D-022 Editor background CPU throttle stays off

Development environment: the editor's background CPU throttle must stay off, because MCP driven runs never have the editor in the foreground and the throttle drops PIE to 3 fps, which produces false hit registration bugs. Document this in docs/dev-setup.md as a required local setting.

## D-021 Fix the template point blank miss

Fix the template weapon point blank miss forward. The projectile spawns ten units ahead of the muzzle, which lands inside a hugging enemy's capsule and never sweeps. The template source is ours now; fix it rather than working around it.

## D-020 Rail is a spline plus our own distance scalar

The rail is a spline plus our own DistanceAlongSpline scalar, driven by gameplay code. Level Sequences are used for individual setpieces only, never as the motor of the ride, because cover pauses, tempo changes and branching are trivial against our own scalar and painful against a sequence timeline. Branch points are separate spline segments selected at runtime.

## D-019 One screen space aiming path

All aiming goes through one screen space path (GetHitResultAtScreenPosition from the crosshair), never a muzzle trace. Reason: light gun replacements (Sinden, GUN4IR, AimTrak) emulate a cursor position, and the planned CYB3RGUN gyro aiming resolves to a screen point too. One path serves mouse, light gun and hardware. Muzzle traces stay cosmetic only (tracers, muzzle flash alignment).

## D-018 Decision log lives in the repo

The decision log is maintained in the repo and every briefing's decisions are recorded there.

## D-017 Encounters are data

Encounters are data, not hand placed actors. A definition asset describes waves; a director consumes it.

## D-016 Shared, data driven enemy base

Enemies share a base class, are data driven, and use StateTree for behaviour. Kept Mass Entity ready so a later switch is a replacement, not a rewrite.

## D-015 Presets store explicit values

Presets store explicit values, never inherited class defaults.

## D-014 Draw bonus decays with the closing window

Draw bonus decays with the closing window. A shot landing while the door shuts must not score the maximum.

## D-013 Silhouettes and telegraph

Occupants must read by silhouette, not colour alone, and hostiles telegraph before becoming shootable so the draw bonus is earnable.

## D-012 Pacing lives in the settings asset

Pacing lives in the settings asset as a per-wave difficulty ramp, because the same module must serve a calm training scenario and a frantic entertainment one.

## D-011 Real HUD widget for the door range

The door range gets a real HUD widget. Debug text cannot be verified in a capture and is not shippable.

## D-010 Ignore the editor-written MCP config

The editor-written copy at CYB3RGUN\.mcp.json is git-ignored. The editor regenerates it; the repo-root copy is authoritative.

## D-009 Version the MCP client config

The unreal-mcp client config .mcp.json is versioned at the repo root. Loopback URL only, no secrets, so every clone gets the MCP wiring for free.

## D-008 Ignore *.slnx

Ignore *.slnx. Visual Studio's new XML solution format is generated from the .uproject exactly like *.sln and never enters version control. Decided on CC's recommendation from the B02 report.

## D-007 LFS budget awareness

LFS budget awareness. GitHub's free tier grants 1 GB LFS storage and 1 GB bandwidth per month. Content size is measured BEFORE staging; hard stop above 800 MB total Content or any single non-LFS file above 95 MB. Growing beyond the budget is the architect's explicit call, never a silent push.

## D-006 Commit authorship stays as is

Commit authorship stays as is. Existing history is never rewritten. The co-author trailer question is with Sascha; keep the trailer until told otherwise.

## D-005 Enforce LF via .gitattributes

Enforce LF via .gitattributes. The machine's core.autocrlf would otherwise create CRLF drift noise on every checkout and for every future contributor. "* text=auto" becomes the FIRST line of .gitattributes. The existing LFS binary patterns keep their explicit -text flags and win as the more specific rule. This lands now, before any Unreal files enter the repo.

## D-004 Game repo boundary

The game and its repository live exclusively in C:\Projects\CYB3RGUN\THEGAME. CC may create this folder and work inside it. Nothing else under C:\Projects\CYB3RGUN is created, modified or committed. Whether the parent folder is itself a git repo is checked read-only and only reported; any action on it is deferred to the architect.

## D-003 Docs trio

The docs trio mirrors the proven framework of the main project: game-concept.md as the living reference that only grows, decisions.md numbered newest first, seasons.md with the G-track table.

## D-002 Standard Unreal .gitignore

Standard Unreal .gitignore. Binaries, DerivedDataCache, Intermediate, Saved and IDE folders never enter version control.

## D-001 Git LFS from the very first commit

Git LFS from the very first commit. Unreal assets are binaries and GitHub hard-fails single files over 100 MB. Retrofitting LFS later rewrites history, so it starts now.
