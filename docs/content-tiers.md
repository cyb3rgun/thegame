# Code and Content Licensing

CYB3RGUN is published as two works under two licences (D-070, D-071). The code, documentation, configuration, brand files and primitive placeholder geometry form the code repository, licensed under the Business Source License 1.1 in [LICENSE](../LICENSE). The game content, meaning characters, environments, enemy models, AI personalities, the soundtrack and purchased or licensed assets, is licensed separately under [LICENSE-CONTENT.md](../LICENSE-CONTENT.md) and reaches customers through the shop and through service agreements. Epic's engine and template material is listed in [LICENSE-THIRD-PARTY.md](../LICENSE-THIRD-PARTY.md).

Code and content are different copyrights, so each has its own folders. This page lists them.

## Folders

Paths are relative to the repository root.

### Game content, under LICENSE-CONTENT.md

These folders hold game content. They are kept in their own content repository and stay out of the code repository's history (D-069).

| Path | Holds | Why it is content |
|------|-------|--------|
| `CYB3RGUN/Content/CYB3RGUN/Characters/` | The character cast: player, offenders, hostages, bystanders | Characters are game content (D-067), and the MetaHuman cast (D-039 to D-042 reserved) lands here |
| `CYB3RGUN/Content/CYB3RGUN/Enemies/Bodies/` | Enemy models and their body materials | Enemy models are game content; enemy definitions, behaviour and placeholder materials are code beside this folder |
| `CYB3RGUN/Content/CYB3RGUN/Environments/` | Finished environment art and the finished levels built from it | Finished environments are game content; primitive blockout levels are code under `Maps` |
| `CYB3RGUN/Content/CYB3RGUN/Audio/` | Soundtrack, music and produced sound | The soundtrack is game content |
| `CYB3RGUN/Content/CYB3RGUN/Personalities/` | AI personalities: voices, dialogue, character specific behaviour | AI personalities are game content |
| `CYB3RGUN/Content/CYB3RGUN/Licensed/` | Purchased or licensed assets brought in by hand | Third party assets come with their own licence terms |
| `CYB3RGUN/Content/Fab/`, `CYB3RGUN/Content/Megascans/`, `CYB3RGUN/Content/MetaHumans/` | Where the engine's marketplace, scan library and MetaHuman importers place assets by default | An import lands in the content folders even before it is sorted by hand |

### Code, under LICENSE

| Path | Holds | Why it is code |
|------|-------|--------|
| `CYB3RGUN/Source/` | All C++ written for the project | Code (D-070) |
| `CYB3RGUN/Config/`, `CYB3RGUN/CYB3RGUN.uproject`, `CYB3RGUN/Build/` icons | Project configuration and application icons | Configuration and brand files |
| `docs/`, `tools/`, `media/`, `.github/`, `README.md`, the licence files | Documentation, scripts, brand source files, repository presentation | Documentation and brand files |
| `CYB3RGUN/Content/CYB3RGUN/Core/` | Shared materials and their CC0 texture sets, combat, style, input, menu, rail and level data | Data assets and surface materials the simulator needs to run (D-068); the textures stay CC0, see docs/credits.md |
| `CYB3RGUN/Content/CYB3RGUN/UI/` | HUD, menus, brand assets | Interface and brand files |
| `CYB3RGUN/Content/CYB3RGUN/VFX/` | Niagara systems, decals, neon materials, the grade | Effects built from engine features |
| `CYB3RGUN/Content/CYB3RGUN/Weapons/` | Weapon definitions, placeholder body materials | Weapons are data with placeholder bodies (D-052, D-054) |
| `CYB3RGUN/Content/CYB3RGUN/Enemies/` beside `Bodies/` | Enemy definitions, state trees, placeholder and body look materials | Definitions and behaviour are code-like data |
| `CYB3RGUN/Content/CYB3RGUN/Enemies/Birds/` | Packed sprite sheets of the family friendly characters and the material that draws them | The finished sheets of that series belong to the game, the source frames never leave the private model repository (D-090, D-091) |
| `CYB3RGUN/Content/CYB3RGUN/Scenarios/` | Game modes, presets and encounter data per scenario | Data assets the simulator needs to run |
| `CYB3RGUN/Content/CYB3RGUN/Maps/` | Menu, playable, test and benchmark levels built from primitives | Primitive placeholder geometry (D-067); a level dressed with finished art moves to `Environments` |
| `CYB3RGUN/Content/Splash/` | The splash screen bitmap | Brand file |

### Epic material, under LICENSE-THIRD-PARTY.md

| Path | Holds |
|------|-------|
| `CYB3RGUN/Content/Characters/`, `Weapons/`, `FirstPerson/`, `Variant_Shooter/`, `Variant_Horror/`, `LevelPrototyping/`, `Input/`, `__ExternalActors__/`, `__ExternalObjects__/` | The first person template and its mannequins, weapons, animations and levels |
| `CYB3RGUN/Source/CYB3RGUN/Variant_Shooter/`, `Variant_Horror/` and the template's base classes | Template source code, partly adapted |

A new top level content folder belongs to the code until this page lists it as game content. Game content goes into one of the content folders from the start.

## Keeping the two apart

- `.gitignore` lists every content folder, so git keeps game content out of the code repository.
- `tools/check_tiers.ps1` confirms that no content folder is tracked or staged, names any file it finds, and checks that every content folder is listed in `.gitignore`. It runs before every push:

```
powershell -ExecutionPolicy Bypass -File tools/check_tiers.ps1
```

## Running the code on its own

The code repository builds and runs by itself (D-068). Where game content is absent, the simulator uses primitives:

- Door range occupants, the menu's ambient doors and the rail hostage taker and hostage use a primitive body: a cylinder torso and a sphere head. They show, hide, take their look material and switch shootable together with the body. A shot on the head sphere counts as a head hit and a shot on the torso as a torso hit, so kills, head shots, disarms, rescues and penalties score as they do with the characters. A held pistol sits at the primitive body's hand. Animations resume once the characters are installed.
- Enemies use the primitive parts their definition carries.
- Character models and their animations load optionally. For each kind of body that uses primitives, the log shows one line under `LogClosedContent`, for example `Closed tier content absent: the door range hostile has no character model, standing in with a primitive torso and head (D-068)`.
- Verified in G04-B05: with the content folders moved aside every level starts and plays. The rail hostage taker was freed with a head shot on its primitive body and the primitive enemies were killed; at the door range a shot on a primitive friendly cost the penalty, and hostage taker doors with primitive hostages were resolved with rescues. With everything installed the log shows no `LogClosedContent` line.

The first person player rig, the template animation Blueprints, the body look materials and the benchmark's mannequin copy use Epic's template content in `CYB3RGUN/Content/Characters`, which ships in the code repository. With that folder in place they work as intended.

## How game content reaches a customer

Game content is available through the shop and through service agreements. Two ways of delivering it were considered:

- **A separate private content repository** with Git LFS, laid out with the same folders and checked out on top of the code repository. Access is granted per customer.
- **A delivered archive**, packed per release.

**Recommended: the private content repository.** It keeps content versions in step with code versions, so a customer pulls updates and every code version can name the content version it was tested with. Access is managed per customer, and it uses the same tools as the code repository. The cost to plan for is Git LFS storage and bandwidth for the heaviest assets (D-007); an archive serves customers who prefer to work without git.

The content repository stays separate from the code repository rather than being linked as a submodule, so the code history carries no reference to it.
