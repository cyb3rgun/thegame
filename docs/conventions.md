# Conventions

## Asset name prefixes

| Prefix | Asset type |
|--------|------------|
| BP_ | Blueprint |
| WBP_ | Widget Blueprint |
| GM_ | Game Mode |
| DA_ | Data Asset |
| IA_ | Input Action |
| IMC_ | Input Mapping Context |
| M_ | Material |
| MI_ | Material Instance |
| T_ | Texture |
| F_ | Font |
| FF_ | Font Face |
| SM_ | Static Mesh |
| SK_ | Skeletal Mesh |
| NS_ | Niagara System |
| S_ | Sound |

## Folder layout

All project-made content lives under `Content/CYB3RGUN` with these top folders: Core, Scenarios, Weapons, Enemies, UI, Audio, VFX, Maps.

Shared textures live in `Content/CYB3RGUN/Core/Textures`, beside the materials in `Core/Materials` that sample them. A texture set is named after its source set, `T_<Set>_BaseColor`, `T_<Set>_Normal` and `T_<Set>_ARM`, and every external set has an entry in docs/credits.md before it is committed.

Template content stays where the template put it. It is not moved or renamed.

C++ stays in `Source/CYB3RGUN`.

## Commits

Every commit builds on its own, not only the pushed head. When a planned split would leave a commit that cannot compile without the next one, the pieces go into one commit. Pushed history is not rewritten.
