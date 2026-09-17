# Decisions

Numbered newest first.

## D-093 Flying targets are flat sprites that face the camera

Flying targets are drawn as a flat quad that turns its face to the camera, not as a body of meshes. One profile view
carries the whole character; a target crossing the other way mirrors the same sheet in the material, so a character
never needs a second set of frames. The flight loop runs at a constant rate whatever the target's speed, size or
distance, a hit switches the quad to the crash sheet, and a second hit to the knockout cell. Size on screen comes from
distance alone, as it does for any object in the world. Reason: the art is drawn, not modelled, and a drawn bird at 10 m
reads better than a modelled one would at the budget this project has. The hit spheres, the leading and the score do
not know the difference (M01-B01).

## D-092 Every character ships a description and the pipeline reads it

Every character folder carries a DESCRIPTION.txt from the artist with the frame order, and the processing pipeline
reads it. The frame order is never guessed from file names and the files are never sorted: a set whose names run out of
order still animates correctly, and a set whose order changes is corrected in the description rather than by renaming
the artist's files (M01-B01).

## D-091 Processed sheets of the family friendly series live in the game repository

The packed sheets of the family friendly series live in the public game repository, as any other texture does. Only the
finished sheets go, never the source frames. A later series that is not family friendly keeps its sheets in the private
model repository and is loaded from outside the public history (M01-B01).

## D-090 Source art lives in a private repository of its own

Source art lives in THEMODELS, a private repository beside the game, never in the public game repository. The frames
are large, the game does not need them to run, and a public repository cannot take anything back out of its history.
The repository carries its own licence: the game's code is published under the Business Source License, the art is not.
The game repository receives only what the pipeline packs (M01-B01).

## D-086 Nanite runs in every preset with Virtual Shadow Maps

Nanite is on in High, Epic, Ultra and Cinematic, the presets that run Virtual Shadow Maps; Medium and Low keep it off. The High preset had Nanite off "until real production geometry exists" (G03-B03). That geometry exists now: the G05-B03 plants are Nanite meshes, and with Nanite off the bird range's trees drew their fallback meshes, whose simplification had dropped every leaf (4,151 triangles of 1,072,212 for island_tree_02, the leaf slot's UV density 0). D-027 stands unchanged: it concerns presets without Virtual Shadow Maps only, and its crash condition does not apply here. For Medium and Low the plant meshes build their fallback with area preservation, so the leaves survive the simplification (island_tree_02: 8,823 triangles). The settings version goes to 3, so saved settings take the new preset values. The measurement D-026 and D-030 ask for is still open: the benchmark of Nanite in High and Epic runs as a night run (G05-B03).

## D-085 Vegetation is generated, not placed by hand

Vegetation is generated, not placed by hand: the engine's procedural vegetation tooling where it is stable, otherwise instanced foliage. The bird range must read as a landscape, not as a green plane with spheres on sticks (G05-B03).

## D-084 Free sources only, in this order

Free sources only, in this order: Unreal engine content and Epic sample packs, Fab items marked free, Poly Haven models, ambientCG, Kenney. Every item is recorded in docs/credits.md with its source, licence and date. No purchases (G05-B03).

## D-083 Environments move from primitives to models

Environments move from primitives to real models. A textured box is still a box. From here on, anything the player looks at closely is a model with geometry, not a scaled cube. In G05-B03 the door range went first: door panel, frame and lanterns are models, the doors carry their grain on their own UVs instead of a world aligned projection, and the wet floor was dried from a mirror to a damp surface.

## D-082 Typography follows the website

Typography follows the website: small capitals with wide letter spacing for labels, large numerals for values, thin rules and corner brackets as framing. No default engine font anywhere in player facing user interface (G05-B03).

## D-081 The brand design system comes from the website

The brand design system comes from the website and is defined once in the project, in MPC_Brand and DA_BrandStyle. Colours: cyan 2BE3FF as the primary, danger red FF2A2A, near white F2FEFF and a deep black background; the older 009FE3 stays inside the logo artwork only. Fonts, all free for software use: the wordmark and the headings in Michroma, labels and technical readouts in Share Tech Mono, titles, buttons and values in Saira Condensed, body text in Source Sans 3. The briefing named Bruno Ace for the wordmark and Barlow for body text. The website's own code uses neither: its wordmark is Michroma with a gradient and its body text is Source Sans 3, and where briefing and website disagree the website wins, with the case reported. Reason: menu, HUD and website must read as one product, not as three designs (G05-B03).

## D-080 The scattergun is the bird range weapon

The scattergun is the intended weapon of the bird range, which comes up in hand; the 3R stays in the loadout. It is the only place in the game where a spread pattern beats a precise shot, which gives that weapon a home. Its weapon definition is shared and was not changed for the mode (G05-B02). In the bird range the flight range settings override magazine and reload per loadout weapon on a copy of the definition handed out for the round: the scattergun carries 10 shells and reloads in 1.0 s, the 3R keeps 17 rounds and reloads in 1.0 s, so reloading is a rhythm and not a pause (G05-B02 addendum).

## D-079 Flight targets are data, not classes

A flying target is a UFlightTargetDefinition: a single mesh or a body of primitive parts with flapping pivots, size, hit spheres, flight speed, the weighted paths it flies with their values, base score, hit behaviour (fall or vanish) and sounds. The range settings list the definitions and their weights. Birds, drones, clay or anything else fly the same module by swapping data assets, without touching code (G05-B02).

## D-078 The player aims from a gallery; targets cross on ballistic paths

The player stands on a fixed spot with no movement and aims through the one screen space aiming path. Since G05-B03 the view no longer turns: it looks along the field and keeps its pitch, the crosshair moves freely over the screen, and when it enters the edge zone at either side the player slides sideways along a rail across the scene, faster the further out the crosshair is. A/D, the arrow keys and the left stick slide the same way, and the reload sits on Space and the right mouse button next to its own binding. The scene is as wide as the settings' scene width in screen widths, measured at the gallery depth, with hard stops at both ends and the start in the middle; the spawn director spreads side entries over that width and keeps every target's distance on the player as it slides. Targets cross the field of view on four path types with their values in data: straight crossing, rising under its own gravity, diving and pulling out, and an erratic flutter. Leading a moving target is the skill this mode trains: projectiles meet the target where it has moved to, and for hitscan pellets the target's hit spheres sit ahead of its body by the distance it covers while a charge at the lead shot speed of the settings crosses the range, so both weapons ask for the same lead (G05-B02). Every side entry passes the player within the reach distance of the settings, 22 m, at its closest point, height and wobble included, so no target is out of reach for its whole flight; the score takes the speed along the path, not the flutter's jinks (G05-B02 addendum).

## D-077 Bird Shooting is a countdown mode, not a wave mode

The bird range runs one countdown, one run and one score: 90 seconds by default in the settings asset, continuous launches that hold a steady number of targets in flight, and a summary at the end. No waves, telegraphs, hostages or difficulty ramp. Fixed props in the scene score through the mode (G05-B03): a bonus prop pays its points once per round, so it cannot be farmed, and a penalty prop costs its points on every shot that lands on it; the sign that asks not to shoot the birds is the mode's only penalty. A hit scores by difficulty, BaseScore x clamp((distance / reference distance) x (speed / reference speed) x (reference size / size), 0.5, 4) rounded to steps of 5, with the references in the settings; the style meter, combo and controlled pairs run beside it unchanged, as in every scenario. It is the deliberate counterpoint to the other three modes (G05-B02). Overclock is locked in this mode through the style values' bOverclockAllowed, because slowed time would stretch a countdown on game time; the rest of the style system stays active (G05-B02 addendum).

## D-076 Door and occupant share one timeline

A door and its occupant run on one timeline: the occupant is visible only where the door leaves it visible. The panel swings out towards the player, so its path never crosses the alcove the occupant stands in and the panel itself hides the occupant while the opening gap grows. On closing, the slot computes from the occupant's bounds the panel angle that covers its far side, plus a small margin for views that are not frontal, and hides the occupant at that angle, before the swing ends. This holds for the hostile, the friendly and the hostage taker pair alike, and was verified frame by frame in slow motion (G05-B01).

## D-075 Tiling breaks up before the horizon

Every tiling surface carries a second texture layer at a different scale, rotated about the vertical and offset, blended with the first by a large scale noise mask, so a repeat never forms a visible grid toward the horizon. M_Surface samples its sets world aligned on three planes with one sampler per texture, keeps texture reading direction the same on every wall, and exposes tiling size and aspect, the second layer's scale and rotation, the macro mask's size, contrast and balance, tint, and scale and offset for roughness, metallic, normal and ambient occlusion to each instance (G05-B01).

## D-074 Seamless surfaces come from CC0 sets

Seamless surface textures come from free CC0 PBR sets: Poly Haven, ambientCG, CraftPBR or AITextured. Every set is recorded with source, authors, licence, download date and size in docs/credits.md before it is committed, and lives in Core/Textures. Colour and ARM maps are taken as JPG and normal maps as PNG, which keeps the six G05-B01 sets at 340 MB instead of 801 MB for PNG throughout, under the raised budget of 400 MB (G05-B01).

## D-073 The age classification is a product fact

The age classification is a product fact, stated once in the legal section of the README.

## D-072 The README is a product page

The README is a product page. Restrictions, warnings and legal statements appear exactly once, at the bottom, in a short legal section. They never appear in the banner area, in the badges, in the introduction or as a warning box, and no red or critical colouring is used anywhere.

## D-071 Game content is licensed separately from the code

Game content is licensed separately from the code, in its own file, LICENSE-CONTENT.md. Code and content are different copyrights, so the split needs no restriction inside the code licence.

## D-070 The code is licensed under the Business Source License 1.1

The code is licensed under the Business Source License 1.1, with a Change Date of four years and GPL-3.0 as the Change License. This supersedes D-066 and the source available positioning. The business model is service, content and shop, not the code. Anyone may install, configure, modify and operate the software for their own business, including commercially and including paying guests at their own premises; only offering it to third parties as a service or reselling it requires an agreement. AGPL was considered and rejected because it permits resale, which is exactly what must not be permitted. Apache was considered and rejected because it permits a competitor to offer the same service. GPL-3.0 is the Change License because the BSL covenants require compatibility with GPL-2.0 or later, which Apache-2.0 does not satisfy.

## D-069 Closed content never enters git history

Closed tier content never enters git history. The ignore rule is in place before the assets arrive, and it is enforced by a check script, tools/check_tiers.ps1, not by discipline. An ignore rule never removes what is already committed, so the split has to exist first.

## D-068 The open tier builds and runs on its own

The open tier must build and run on its own. Missing closed content degrades to primitives and logs a clear notice; it never crashes and never blocks startup. A public repository that cannot run is worse than none.

## D-067 Content is split into two tiers

The open tier is code, documentation, configuration, brand files and primitive placeholder geometry. The closed tier is characters, finished environments, enemy models, AI personalities, soundtrack and any purchased or licensed asset, available only under a rental and server agreement. The paths of each tier are in docs/content-tiers.md.

**Partly superseded by D-071 (G04-B06).** The folder split stands. Game content is licensed under LICENSE-CONTENT.md and is available through the shop and through service agreements.

## D-066 Source available, not open source

The correct term is source available, not open source. Open source implies a right to redistribute, which is not granted, and the wrong term invites criticism the project does not need.

**Superseded by D-070 (G04-B06).** The code is licensed under the Business Source License 1.1.

## D-065 No binary release is ever published

No binary release will ever be published. Anyone who wants to run the product compiles, configures, sets up and administers it themselves. This is a deliberate distribution decision, not a limitation.

**Superseded by D-070 and D-071 (G04-B06).** The code is published under the Business Source License 1.1, and models, content packs, plugins and updates are available in the shop.

## D-064 The product is for adults

The product is for adults: not suitable for anyone under 18, developed with an 18+ classification as its target. This is visible at the top of the README, not buried in a legal section.

**Partly superseded by D-072 and D-073 (G04-B06).** The 18+ classification stands; it is stated once, in the legal section at the bottom of the README.

## D-063 Targets are armed offenders, machines or the undead

Targets are armed offenders, machines or the undead. Never bystanders, never hostages. This supersedes the older concept line "undead, machines or objects, never humans". A law enforcement scenario without human offenders is not a law enforcement scenario. The rule protects the innocent, not the species.

## D-062 The README is kept current every season

The README is the project's front door. It is brought up to date at the end of every season, in the same structure as the initial version, so what a visitor reads matches what is built.

## D-061 Public text names no real weapon maker, weapon model, film or game

Public facing text never names a real weapon manufacturer, a real weapon model, a film or another game. Mechanics may be described freely; names are what turn an homage into a false origin claim. This covers the README, the store page, trailers and all marketing copy.

Clarified in G04-B05: D-061 applies only to outward facing text, meaning the README, the store page, trailers and marketing. Internal development documents such as the game concept and the decision log may name genre references, games and products, because that is what they are for.

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
