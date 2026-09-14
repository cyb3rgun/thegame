# Content Tiers

The project's content is split into two tiers (D-067). The open tier is this public repository. The closed tier is licensed separately and never enters its git history (D-069). The open tier builds and runs on its own: wherever closed content is missing, the game stands in with primitives, logs one clear line and carries on (D-068).

## Open and closed paths

Paths are relative to the repository root.

### Closed

| Path | Holds | Reason |
|------|-------|--------|
| `CYB3RGUN/Content/CYB3RGUN/Characters/` | The character cast: player, offenders, hostages, bystanders | Characters are the closed tier's core content (D-067), and the MetaHuman cast (D-039 to D-042 reserved) lands here |
| `CYB3RGUN/Content/CYB3RGUN/Enemies/Bodies/` | Enemy models and their body materials | Enemy models are closed (D-067); enemy definitions, behaviour and placeholder materials stay open beside this folder |
| `CYB3RGUN/Content/CYB3RGUN/Environments/` | Finished environment art and the finished levels built from it | Finished environments are closed (D-067); primitive blockout levels stay open under `Maps` |
| `CYB3RGUN/Content/CYB3RGUN/Audio/` | Soundtrack, music and produced sound | The soundtrack is closed (D-067) |
| `CYB3RGUN/Content/CYB3RGUN/Personalities/` | AI personalities: voices, dialogue, character specific behaviour | AI personalities are closed (D-067) |
| `CYB3RGUN/Content/CYB3RGUN/Licensed/` | Any purchased or licensed asset brought in by hand | Purchased and licensed assets are never ours to publish (D-067) |
| `CYB3RGUN/Content/Fab/`, `CYB3RGUN/Content/Megascans/`, `CYB3RGUN/Content/MetaHumans/` | Where the engine's marketplace, scan library and MetaHuman importers place assets by default | An import that is not moved by hand would otherwise land in the open tier unnoticed |

### Open

| Path | Holds | Reason |
|------|-------|--------|
| `CYB3RGUN/Source/` | All C++ | Code is open (D-067) |
| `CYB3RGUN/Config/`, `CYB3RGUN/CYB3RGUN.uproject`, `CYB3RGUN/Build/` icons | Project configuration and application icons | Configuration and brand files are open (D-067) |
| `docs/`, `tools/`, `media/`, `.github/`, `README.md`, `LICENSE` | Documentation, scripts, brand source files, repository presentation, licence | Documentation and brand files are open (D-067) |
| `CYB3RGUN/Content/CYB3RGUN/Core/` | Shared materials, combat, style, input, menu, rail and level data | Data assets and blockout materials the open tier needs to run (D-068) |
| `CYB3RGUN/Content/CYB3RGUN/UI/` | HUD, menus, brand assets | Brand files and interface are open (D-067) |
| `CYB3RGUN/Content/CYB3RGUN/VFX/` | Niagara systems, decals, neon materials, the grade | Effects built from engine features, needed to run |
| `CYB3RGUN/Content/CYB3RGUN/Weapons/` | Weapon definitions, placeholder body materials | Weapons are data with placeholder bodies (D-052, D-054) |
| `CYB3RGUN/Content/CYB3RGUN/Enemies/` except `Bodies/` | Enemy definitions, state trees, placeholder and body look materials | Definitions and behaviour are code-like data; the models are closed |
| `CYB3RGUN/Content/CYB3RGUN/Scenarios/` | Game modes, presets and encounter data per scenario | Data assets the open tier needs to run |
| `CYB3RGUN/Content/CYB3RGUN/Maps/` | Menu, playable, test and benchmark levels built from primitives | Primitive placeholder geometry is open (D-067); a level dressed with finished art moves to `Environments` |
| `CYB3RGUN/Content/Splash/` | The splash screen bitmap | Brand file |
| Epic template content: `CYB3RGUN/Content/Characters/`, `Weapons/`, `FirstPerson/`, `Variant_Shooter/`, `Variant_Horror/`, `LevelPrototyping/`, `Input/`, `__ExternalActors__/`, `__ExternalObjects__/` | The first person template and its mannequins, weapons, animations and levels | Committed since G01. Its redistribution terms are with the architect for review; until then it stays where the template put it (conventions) |

A new top level content folder is open until this table says otherwise. Anything that is closed by nature goes into one of the closed folders from the start, because an ignore rule never removes what is already committed.

## Enforcement

- `.gitignore` lists every closed path, so git never offers closed content for a commit.
- `tools/check_tiers.ps1` fails when a closed path is tracked or staged and names every file it found. Run it before every push:

```
powershell -ExecutionPolicy Bypass -File tools/check_tiers.ps1
```

The script also fails when a closed path is missing from `.gitignore`, so the two lists cannot drift apart.

## Running without the closed tier

- Door range occupants, the menu's ambient doors and the rail hostage taker and hostage stand in with a primitive body when their character model is absent: a cylinder torso and a sphere head. They show, hide, take their look material and switch shootable together with the body. A shot on the head sphere counts as a head hit and a shot on the torso as a torso hit, so kills, head shots, rescues and penalties score as before. A held pistol moves to the stand in's hand and can still be shot for a disarm. Animations have nothing to play and are skipped.
- Enemies without a body model use the primitive parts their definition already carries.
- The character models and their animations load through an optional load that raises no error when the package is absent; the stand in then writes one line per kind to the log under `LogClosedContent`, for example `Closed tier content absent: the door range hostile has no character model, standing in with a primitive torso and head (D-068)`.
- Verified in G04-B05 with the closed folders and the template `Characters` folder moved aside: every level starts and plays, nothing crashes and nothing blocks startup. The door range shows its stand ins; on the rail the stand in hostage taker was freed with a head shot and the primitive enemies were killed. Shots on door range stand ins could not be scored in that run, because the first person player rig depends on the template folder that was moved aside with them (see below). With everything present no `LogClosedContent` line appears and the door range scores as before.

Dependencies on the Epic template content in `CYB3RGUN/Content/Characters` are not closed tier and are not covered by the stand ins. Without that folder the first person player rig has no arms, camera socket or weapon sockets and its shots do not land, the template animation Blueprints fail to compile (the editor then asks before Play unless it runs with `-unattended`), the body look materials lose their parent materials, and the benchmark's mannequin copy loses its skeleton.

## How a licensee receives the closed tier

Two ways were considered.

- **A separate private repository.** The closed tier lives in its own private repository with Git LFS, laid out with the same paths, and is checked out on top of an open tier checkout. The closed paths are ignored by the open repository, so the two never mix. Access is granted per licensee and revoked by removing it.
- **A delivered archive.** The closed tier is packed per release and handed over as a file.

**Recommended: the separate private repository.** It keeps the closed tier versioned against the open tier, so a licensee pulls updates instead of unpacking archives and every open tier commit can name the closed tier version it was tested with. Access is per person and can be withdrawn, which an archive that has already been copied cannot. It uses the same tools the open tier already uses. The cost to watch is Git LFS storage and bandwidth for a second repository holding the heaviest content (D-007); an archive becomes the fallback for a licensee who cannot use git.

The closed repository is never added to the open one as a submodule, because a submodule reference would put the closed tier's location and versions into the public history.
