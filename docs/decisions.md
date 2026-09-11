# Decisions

Numbered newest first.

## D-046 Feel is tuned by measured values in a data asset

Hit stop, camera shake and slow motion are balanced through measured values in a data asset, never through constants scattered in code, so they can be tuned without a rebuild.

## D-045 Cover is the reload trigger

As in the arcade tradition, cover is where the weapon reloads. Reloading is a decision made under fire, not a button pressed in safety, and it gives the rail module its rhythm.

## D-044 The style meter is the moral system

Clean, disciplined shooting scores highest, and hitting a hostage or bystander collapses the meter instantly. The rule that innocents are never targets stops being a penalty line and becomes the core scoring loop.

## D-043 Gun fu is built from mechanics only

The gun fu style is built from mechanics, never from another work's expression. Genre, mood, palette, a shooting stance and the real Center Axis Relock technique are unprotected and free to use. Names, logos, characters, dialogue, specific set designs and any named hotel from an existing film are not, and never enter code, assets, filenames or marketing text. This is a house rule, not a preference.

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
