# Credits

External assets used in the project, with source, licence and download date (D-074). An asset enters the repository only with an entry here.

## Textures

The first six texture sets come from Poly Haven under CC0 1.0 Universal (https://polyhaven.com/license). CC0 asks for no attribution; the authors are named here because the project wants to credit them. The sets stay CC0 inside the project: the project licences do not add conditions to them.

Each set is imported as three textures in `CYB3RGUN/Content/CYB3RGUN/Core/Textures`: `T_<Set>_BaseColor` (sRGB colour), `T_<Set>_Normal` (DirectX normal map) and `T_<Set>_ARM` (ambient occlusion, roughness and metallic in red, green and blue). Colour and ARM maps were downloaded as JPG, normal maps as PNG. Sizes are the downloaded files and the imported assets.

| Set | Source | Authors | Resolution | Used by | Download | Imported | Downloaded |
|-----|--------|---------|------------|---------|----------|----------|------------|
| CobblestoneFloor07 | https://polyhaven.com/a/cobblestone_floor_07 | Rob Tuytel | 4K | MI_Floor_Stone, MI_Floor_Stone_Wet | 22.3 MB | 23.6 MB | 2026-09-14 |
| PlasterGrey04 | https://polyhaven.com/a/plaster_grey_04 | Rob Tuytel | 4K | MI_Wall_Alcove, MI_Wall_Plaster, MI_Bench_Concrete | 110.7 MB | 105.3 MB | 2026-09-14 |
| PlasteredStoneWall | https://polyhaven.com/a/plastered_stone_wall | Rob Tuytel | 4K | MI_Wall_Dark | 122.0 MB | 116.6 MB | 2026-09-14 |
| WoodCabinetWornLong | https://polyhaven.com/a/wood_cabinet_worn_long | Dimitrios Savva, Rico Cilliers | 4K (4096 by 2048) | MI_Door_Panel | 63.3 MB | 59.7 MB | 2026-09-14 |
| RustyMetal02 | https://polyhaven.com/a/rusty_metal_02 | Rob Tuytel | 2K | MI_Metal_Dark | 6.6 MB | 7.5 MB | 2026-09-14 |
| WeatheredPlanks | https://polyhaven.com/a/weathered_planks | Dario Barresi, Dimitrios Savva | 2K | MI_Door_Frame, MI_Crate, MI_Bench_Bark | 26.8 MB | 27.6 MB | 2026-09-14 |
| Total | | | | | 351.7 MB | 340.4 MB | |

### ambientCG

The grass and foliage sets come from ambientCG under CC0 1.0 Universal (https://docs.ambientcg.com/license/). Colour and the DirectX normal map were taken from the JPG download; the ARM texture was packed from the set's ambient occlusion and roughness maps with metallic at zero. Sizes are the downloaded zip and the three imported assets.

| Set | Source | Authors | Resolution | Used by | Download | Imported | Downloaded |
|-----|--------|---------|------------|---------|----------|----------|------------|
| Grass004 | https://ambientcg.com/view?id=Grass004 | ambientCG | 2K | MI_BirdRange_Meadow | 39.9 MB | 21.1 MB | 2026-09-14 |
| Moss004 | https://ambientcg.com/view?id=Moss004 | ambientCG | 1K | MI_BirdRange_Foliage (tree canopies and hedges) | 9.8 MB | 4.6 MB | 2026-09-14 |
| Foliage001 | https://ambientcg.com/view?id=Foliage001 | ambientCG | 1K | M_Foliage, the default blade atlas of the foliage material | 3.9 MB | 1.4 MB | 2026-09-15 |
| Total | | | | | 53.6 MB | 27.1 MB | |

Foliage001 is a cut out atlas of single grass blades: its colour, DirectX normal and opacity maps were imported as `T_Foliage001_BaseColor`, `T_Foliage001_Normal` and `T_Foliage001_Opacity`, without an ARM texture.

## Models

The models come from Poly Haven under CC0 1.0 Universal (https://polyhaven.com/license) and stay CC0 inside the project. They were downloaded as glTF with 1K JPG textures, converted to FBX in Blender without changing their geometry (base pivot, split variants) and imported into `CYB3RGUN/Content/CYB3RGUN/Core/Models`, their textures into `Core/Textures` as `T_<Set>_BaseColor`, `_Normal` (the OpenGL map, imported with its green channel flipped) and `_ARM`. The JPG glTF files carry no alpha, so the cut out masks were downloaded separately as the sets' 1K alpha PNG and imported as `T_<Set>_Opacity`; these 16 bit masks read correctly only with Masks compression (G05-B03). The plant meshes are Nanite meshes whose fallback keeps its leaves through area preservation (D-086). Sizes are the downloaded files and the imported meshes, material instances and textures.

| Model | Source | Authors | Resolution | Used by | Download | Imported | Downloaded |
|-------|--------|---------|------------|---------|----------|----------|------------|
| Street Lamp 02 | https://polyhaven.com/a/street_lamp_02 | Josh Dean | 1K | SM_Lantern (the lamp head and wall bracket, on a post made for the project), MI_StreetLamp02 | 1.9 MB | 2.3 MB | 2026-09-15 |
| Island Tree 02 | https://polyhaven.com/a/island_tree_02 | Rob Tuytel, Rico Cilliers | 1K | SM_IslandTree02 and its bark, branch and leaf materials | 46.2 MB | 36.3 MB | 2026-09-15 |
| Pine Sapling Small | https://polyhaven.com/a/pine_sapling_small | Rob Tuytel, Rico Cilliers | 1K | SM_PineSapling_A, _B, _C (the set's three saplings) and their bark and twig materials | 22.1 MB | 18.7 MB | 2026-09-15 |
| Shrub 04 | https://polyhaven.com/a/shrub_04 | Rico Cilliers | 1K | SM_Shrub04, MI_Shrub04 | 2.0 MB | 2.1 MB | 2026-09-15 |
| Total | | | | | 72.1 MB | 59.3 MB | |

## Fonts

The brand fonts follow the website (D-081) and are licensed under the SIL Open Font License 1.1. The licence texts with each font's copyright notice are in [docs/licenses](licenses). The font files are stored inside the font face assets in `CYB3RGUN/Content/CYB3RGUN/UI/Fonts` (`FF_<Font>_<Face>`), which the fonts `F_<Font>` group; the brand style picks them per text role. Sizes are the downloaded font files and the imported font face assets.

| Font | Source | Copyright | Faces | Used for | Download | Imported | Downloaded |
|------|--------|-----------|-------|----------|----------|----------|------------|
| Michroma | https://github.com/google/fonts/tree/main/ofl/michroma | The Michroma Project Authors | Regular | Wordmark and headings | 64 KB | 66 KB | 2026-09-15 |
| Saira Condensed | https://github.com/google/fonts/tree/main/ofl/sairacondensed | The Saira Project Authors | Medium, Bold | Titles, buttons and values (Bold; Medium is imported but no text role uses it) | 193 KB | 196 KB | 2026-09-15 |
| Share Tech Mono | https://github.com/google/fonts/tree/main/ofl/sharetechmono | Carrois Type Design, Ralph du Carrois | Regular | Labels and technical readouts | 43 KB | 45 KB | 2026-09-15 |
| Source Sans 3 | https://github.com/adobe-fonts/source-sans | Adobe | Regular, Semibold | Body text | 857 KB | 861 KB | 2026-09-15 |
| Total | | | | | 1.16 MB | 1.18 MB | |

## Made for the project

These were built for the project in Blender from code and carry no third party material: the door frame `SM_DoorFrame` and the door panel `SM_DoorPanel` with its hinges and handles, and the cast iron post of `SM_Lantern`. They are textured with the CC0 sets above through `M_ModelSurface`.
