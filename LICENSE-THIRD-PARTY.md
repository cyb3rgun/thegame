# Third Party Material

This repository contains and relies on material that belongs to others. It is included under its owners' terms and is not licensed under the Business Source License 1.1 in [LICENSE](LICENSE).

## Epic Games: Unreal Engine and template content

The project is built on Unreal Engine 5.8 by Epic Games, Inc. The following material in this repository is Epic's. It is included under the [Unreal Engine End User License Agreement](https://www.unrealengine.com/eula) and remains Epic's property. It is not licensed under the Business Source License. Anyone who builds, runs or changes this project needs an accepted Unreal Engine EULA.

**Template content**, from the Unreal Engine first person template and its variants:

| Path | Content |
| --- | --- |
| `CYB3RGUN/Content/Characters/` | Mannequin meshes, skeleton, rigs, physics asset, animations, materials and textures |
| `CYB3RGUN/Content/Weapons/` | Pistol, rifle and grenade launcher meshes, materials, textures and one weapon sound |
| `CYB3RGUN/Content/FirstPerson/` | The first person level, animations and Blueprints |
| `CYB3RGUN/Content/Variant_Shooter/` | The shooter variant level, animations, AI, pickups, projectiles, input and UI |
| `CYB3RGUN/Content/Variant_Horror/` | The horror variant level, Blueprints, input and UI |
| `CYB3RGUN/Content/LevelPrototyping/` | Prototyping meshes, materials, door, jump pad and target |
| `CYB3RGUN/Content/Input/` | Template input mapping contexts and actions |
| `CYB3RGUN/Content/__ExternalActors__/`, `CYB3RGUN/Content/__ExternalObjects__/` | Actor data of the three template levels |

**Template source code**, carrying Epic's copyright header:

| Path | Content |
| --- | --- |
| `CYB3RGUN/Source/CYB3RGUN/Variant_Shooter/` | Shooter variant code. Parts of it were changed and extended for this project; the files remain derived from Epic's template |
| `CYB3RGUN/Source/CYB3RGUN/Variant_Horror/` | Horror variant code, unchanged |
| `CYB3RGUN/Source/CYB3RGUN/CYB3RGUN.cpp`, `CYB3RGUN.h`, `CYB3RGUNCameraManager`, `CYB3RGUNCharacter`, `CYB3RGUNGameMode`, `CYB3RGUNPlayerController`, `CYB3RGUN.Build.cs` | The template's generated game module and base classes; `CYB3RGUN.Build.cs` was extended with this project's modules and include paths |
| `CYB3RGUN/Source/CYB3RGUN.Target.cs`, `CYB3RGUN/Source/CYB3RGUNEditor.Target.cs` | The template's build targets |

**Engine content and plugins used at runtime.** The project's own assets and code refer to content that ships with Unreal Engine and is not stored in this repository: the basic shapes, engine materials, sky and cloud content, engine and editor sounds, the default font and the Model Context Protocol, All Toolsets, Modeling Tools, StateTree and Gameplay StateTree plugins. They are part of Unreal Engine and covered by the same EULA.

## MariaDB: Business Source License text

The text of the Business Source License 1.1 in [LICENSE](LICENSE) is copyright (c) 2017 MariaDB Corporation Ab. "Business Source License" is a trademark of MariaDB Corporation Ab. The text is used under the permission granted in the licence itself.

## Fonts inside Unreal Engine

The interface uses the engine's default font, Roboto, which ships with Unreal Engine together with its licence at `Engine/Source/ThirdParty/Licenses/ROBOTO_License.txt`. It is not stored in this repository.
