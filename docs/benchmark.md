# Benchmark

Measured results for the graphics features and presets, taken on the development machine (D-026). Every feature ships behind a player facing option (D-025); this page is where the defaults come from.

## Machine

| Part | Value |
|------|-------|
| GPU | NVIDIA GeForce RTX 3090 |
| CPU | Intel Core i9-11900K, 8 cores, 16 threads |
| Memory | 128 GB |
| Display | 5120 x 1440 at 239 Hz, one monitor |
| Engine | Unreal Engine 5.8.2, standalone game from the editor build (`-game`) |

## Method

`Lvl_Benchmark` is a 194 m route flown by the rail pawn at 3 m eye height, in four sections:

- **City.** 24 concrete blocks along a street lit by 60 movable, shadow casting point lights in five colours.
- **Vegetation.** Instanced placeholder plants: 30,000 grass blades, 4,000 bushes, 900 trees of trunk and canopy. The meshes are Nanite enabled copies of the engine's basic shapes.
- **Enemies.** 20 enemies from the encounter director that walk toward the camera, plus 20 animated mannequins whose mesh copy has Nanite enabled.
- **Effects.** 20 looping Niagara fountains.

Volumetric fog is enabled on the level's height fog, so the fog option has something to render.

Each configuration is applied through `UCyberGameUserSettings`, never through raw console variables. The console command `Bench.Suite` then runs, for each configuration:

1. A warm up lap at three times the route speed.
2. A wait until shader compilation has been idle for 2 s.
3. A 1.5 s settle.
4. The measured lap at 600 cm/s, 32.4 s long.

Every lap starts the encounter again, which removes the enemies the previous lap left standing, so each lap meets the same 20.

**Measuring in one process.** Four passes were needed to get numbers that repeat. The RTX 3090 here boosts to about 1900 MHz while cool and settles at about 1450 to 1750 MHz under load, with the driver reporting a thermal slowdown. That alone did not explain the spread: the same configuration measured up to about 25 percent apart in different game processes, and the slower process sometimes had the higher clock. Inside one process the same configuration repeats within about 2 percent, and a heavy configuration such as Epic leaves nothing behind that slows the next one (an A/B probe measured the baseline at 12.38, 12.53 and 12.66 ms, then Epic, then 12.58 and 12.57 ms).

The published pass (run 4) is therefore one process. Two unmeasured baseline laps bring the GPU to its sustained clock. Every feature lap then sits between two baseline laps, and its cost is the difference to their mean. The presets follow from light to heavy; as absolute frame times they carry the process spread, so the tables give their range across all passes as well. `nvidia-smi` logged the GPU clock every 5 s, and the tables show the mean clock of each lap.

Frame rate cap and VSync are off for every run. The measured values are:

| Column | Meaning |
|--------|---------|
| avg ms | mean frame time over the measured lap |
| avg fps | 1000 divided by avg ms |
| 1% low ms | mean of the slowest one percent of frames |
| 1% low fps | 1000 divided by 1% low ms |
| GPU ms | mean GPU frame time reported by the RHI |
| game ms | mean game thread time |

The render thread time is not reported. UE 5.8 no longer writes the global the benchmark would read, it stays zero.

**Baseline** means every heavy feature off: MegaLights off, no dynamic global illumination, conventional shadow maps instead of Virtual Shadow Maps, no volumetric fog, no anti aliasing, Nanite off, lowest effects density and view distance, no motion blur. The remaining scalability groups stay at High. Each single feature run switches exactly one option on top of the baseline. Nanite is the one exception: it only runs together with Virtual Shadow Maps (see the observations), so its run adds Virtual Shadow Maps at Low, and the Virtual Shadow Maps Low cost is subtracted to get the cost of Nanite alone.

The two experimental options are read only engine switches. They run in their own processes with the switch set at startup, and are compared against the baseline, Nanite and Ultra runs of the main process.

Reproduce with the editor closed:

```bat
UnrealEditor.exe CYB3RGUN.uproject /Game/CYB3RGUN/Maps/Lvl_Benchmark -game -ResX=5120 -ResY=1440 -windowed -log -ExecCmds="Settings.Set WindowMode Borderless, Settings.Set Resolution 5120x1440, Bench.Suite all quit"
```

That runs every configuration once in order. Run 4, the published pass, used the interleaved order instead: `Bench.Suite` accepts `config:label` entries, so it was `baseline:r4_soak1 baseline:r4_soak2 baseline:r4_b01 megalights:r4_megalights baseline:r4_b02 gi_lumenlite:r4_gi_lumenlite ...` with a baseline lap before and after each of the 16 single features, then `preset_low` to `preset_ultra`.

Results land in the log as `BENCH|...` lines and in `Saved/Benchmark/bench_results.csv`. Since G03-B03 every result line also names the GPU (`gpu=`), its driver version (`driver=`) and the resolution (`res=`), so a result file shows which hardware it came from.

### Night run

Long measurements only run in a dedicated night run (D-032). The next one is prepared and has not been run. It measures the six presets as they are now, plus Ultra at 150 and 200 percent resolution scale, with the run 4 method: one process, two unmeasured baseline laps first, then every configuration between two baseline laps. That is 19 measured laps, about 20 minutes. Start it from the repository root with the editor closed:

```bat
powershell -ExecutionPolicy Bypass -File tools/night_benchmark.ps1
```

The script checks that no editor is running, reads the physical display resolution, logs the GPU clock with `nvidia-smi` when it is present, and runs `Bench.Suite night quit`. The suite is defined in `UBenchmarkSubsystem::GetNightSuite`.

## Options and the console variables they drive

| Option | Values | Console variables |
|--------|--------|-------------------|
| Quality preset | Low, Medium, High, Epic, Ultra, Cinematic | `sg.*` scalability groups at level 0 to 3, Ultra stays at 3, Cinematic uses Cine (4), plus the feature defaults of the preset |
| MegaLights | Off, On | `r.MegaLights.EnableForProject` |
| Global illumination | Off, Lumen Lite, Lumen | `r.DynamicGlobalIlluminationMethod`, `r.Lumen.DiffuseIndirect.Allow`, `r.Lumen.FinalGatherMethod`, `r.ReflectionMethod`, `r.Lumen.Reflections.Allow` |
| Virtual shadow maps | Off, Low, Medium, High, Epic | `r.Shadow.Virtual.Enable`, `r.Shadow.Virtual.ResolutionLodBiasDirectional` and `...Moving`, `r.Shadow.Virtual.ResolutionLodBiasLocal` and `...Moving`, `r.Shadow.Virtual.SMRT.RayCountDirectional`, `r.Shadow.Virtual.SMRT.RayCountLocal`, `r.Shadow.Virtual.MaxPhysicalPages` |
| Volumetric fog | Off, Low, Medium, High | `r.VolumetricFog`, `r.VolumetricFog.GridPixelSize`, `r.VolumetricFog.GridSizeZ` |
| Anti aliasing and upscaling | Off, TSR Native, TSR Quality, TSR Balanced, TSR Performance | `r.AntiAliasingMethod` (0 or 4), `r.ScreenPercentage` (100, 100, 66.7, 58, 50) times the resolution scale |
| Resolution scale | 100, 125, 150, 175, 200 percent, Ultra and Cinematic only | `r.ScreenPercentage`, the anti aliasing percentage times the scale, at most 200. Above 100 the engine renders more pixels than the display and scales down, which is supersampling; 200 is the TSR maximum |
| Nanite static meshes | Off, On | `r.Nanite`, set to 1 only while Virtual Shadow Maps are on, see the observations |
| Effects and particle density | Low, Medium, High, Epic | `sg.EffectsQuality` |
| View distance | Near, Medium, Far, Epic | `sg.ViewDistanceQuality` |
| Motion blur | Off, On | `r.MotionBlurQuality` (0 or 4) |
| Frame rate cap | 30, 60, 120, 144, 240, Unlimited | `t.MaxFPS` |
| VSync | Off, On | `r.VSync` |
| Resolution | supported modes plus the desktop resolution | `r.SetRes`, through `UGameUserSettings` |
| Window mode | Fullscreen, Borderless, Windowed | `r.SetRes` window mode suffix, through `UGameUserSettings` |
| Field of view | 60 to 120 degrees in steps of 5 | none, the settings subsystem sets the field of view of the camera the player looks through |
| Experimental Nanite skeletal meshes | Off, On, Ultra and Cinematic only | `r.Nanite.AllowSkinnedMeshes`, read only, staged in the engine config for the next start |
| Experimental Nanite foliage | Off, On, Ultra and Cinematic only | `r.Nanite.Foliage` and `r.Nanite.AllowAssemblies`, read only, staged in the engine config for the next start |

**The experimental switches** are read only, so the settings object writes them to the `[ConsoleVariables]` section of the saved Engine.ini, which the engine reads at the next start. The engine only saves the Engine.ini sections listed under `[SectionsToSave]`, so `DefaultEngine.ini` adds that section to the list, and it sets `r.Nanite.AllowSkinnedMeshes=0` because the engine default is on. Checked over four starts: both switches on at Ultra stages 1, 1, 1 with a restart pending; the next start runs with 1, 1, 1 and nothing pending; a reset stages the defaults; the start after that runs with 0, 0, 0.

**Lumen Lite** is not an engine name. UE 5.8 has no mode called that. The option uses Lumen with the Irradiance Field Gather (`r.Lumen.FinalGatherMethod 0`), which the engine describes as faster, lower quality GI targeted at mid range PC. Reflections are screen space in this mode.

## Results

All frame times are at 5120 x 1440, the full display, with frame rate cap and VSync off. Run 4 is the published pass; the other runs appear only in the last table. Costs below 0.3 ms are inside the noise of run 4, whose 17 baseline laps stayed between 12.39 and 12.74 ms.

Run 4, resolution 5120 x 1440, 2572 frames in the first baseline lap.

### Each feature against its neighbouring baseline laps

| Configuration | avg ms | avg fps | 1% low ms | GPU ms | game ms | GPU MHz | baselines before and after, ms | cost ms | 1% low change ms |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| MegaLights on | 13.51 | 74 | 18.75 | 12.59 | 3.06 | 1495 | 12.59, 12.62 | +0.90 | -3.52 |
| GI Lumen Lite | 11.39 | 88 | 21.40 | 10.43 | 3.11 | 1573 | 12.62, 12.68 | -1.26 | -0.80 |
| GI Lumen | 12.81 | 78 | 21.65 | 11.78 | 3.22 | 1519 | 12.68, 12.74 | +0.10 | -0.62 |
| Virtual shadow maps Low | 13.08 | 76 | 21.27 | 12.32 | 2.96 | 1490 | 12.74, 12.61 | +0.40 | -0.82 |
| Virtual shadow maps High | 13.29 | 75 | 23.87 | 12.53 | 3.04 | 1511 | 12.61, 12.57 | +0.70 | +1.78 |
| Virtual shadow maps Epic | 13.66 | 73 | 24.08 | 12.89 | 2.98 | 1470 | 12.57, 12.46 | +1.14 | +1.64 |
| Volumetric fog Low | 12.90 | 78 | 22.70 | 12.07 | 3.00 | 1490 | 12.46, 12.39 | +0.47 | +0.16 |
| Volumetric fog High | 14.80 | 68 | 24.81 | 13.95 | 3.06 | 1436 | 12.39, 12.45 | +2.38 | +2.65 |
| TSR Native (100 %) | 17.20 | 58 | 27.04 | 16.13 | 3.14 | 1455 | 12.45, 12.59 | +4.68 | +4.87 |
| TSR Quality (66.7 %) | 10.79 | 93 | 16.93 | 9.86 | 3.01 | 1545 | 12.59, 12.63 | -1.82 | -5.30 |
| TSR Balanced (58 %) | 9.49 | 105 | 15.47 | 8.58 | 3.00 | 1592 | 12.63, 12.64 | -3.15 | -6.73 |
| TSR Performance (50 %) | 8.43 | 119 | 13.24 | 7.55 | 3.01 | 1645 | 12.64, 12.68 | -4.23 | -8.88 |
| Nanite static meshes with VSM Low | 14.12 | 71 | 22.13 | 13.27 | 3.19 | 1488 | 12.68, 12.71 | +1.42 | -2.08 |
| Effects density Epic | 12.67 | 79 | 22.22 | 11.84 | 2.99 | 1524 | 12.71, 12.54 | +0.04 | -2.11 |
| View distance Epic | 12.58 | 80 | 22.59 | 11.75 | 3.02 | 1480 | 12.54, 12.42 | +0.10 | +0.42 |
| Motion blur on | 13.06 | 77 | 23.35 | 12.20 | 3.11 | 1452 | 12.42, 12.46 | +0.62 | +1.28 |

Nanite alone, the Nanite lap cost minus the Virtual Shadow Maps Low cost: +1.02 ms.

Baseline laps through the process: 12.59, 12.62, 12.68, 12.74, 12.61, 12.57, 12.46, 12.39, 12.45, 12.59, 12.63, 12.64, 12.68, 12.71, 12.54, 12.42, 12.46 ms, range 12.39 to 12.74 ms.
Their GPU clocks: 1524, 1474, 1488, 1502, 1487, 1492, 1506, 1488, 1488, 1506, 1470, 1490, 1504, 1518, 1498, 1481, 1474 MHz.

### Presets end to end

These are the presets as they were during run 4. Ultra then used the engine's Cine level; that preset is now called Cinematic, and Ultra became the maximum game preset on the Epic level (D-028, D-029).

| Preset | avg ms | avg fps | 1% low ms | 1% low fps | GPU ms | game ms | GPU MHz |
|---|---:|---:|---:|---:|---:|---:|---:|
| Low | 5.88 | 170 | 7.88 | 127 | 5.13 | 2.76 | 1680 |
| Medium | 9.97 | 100 | 13.62 | 73 | 8.63 | 3.33 | 1665 |
| High | 13.48 | 74 | 19.14 | 52 | 12.26 | 3.46 | 1535 |
| Epic | 27.65 | 36 | 33.74 | 30 | 26.31 | 3.52 | 1378 |
| Ultra | 56.49 | 18 | 79.34 | 13 | 55.01 | 3.66 | 1348 |

### Experimental options

Each experimental switch is read only, so it runs in its own process and is compared with the main process of the same run. The same configuration differs by up to about 25 percent between processes on its own, so these comparisons cannot resolve anything smaller.

| Run | Switch | Configuration | avg ms with switch | avg ms main process | difference ms | 1% low with switch | 1% low main |
|---|---|---|---:|---:|---:|---:|---:|
| run 2 | Nanite skeletal meshes on | baseline | 13.98 | 11.99 | +1.99 | 23.04 | 21.06 |
| run 2 | Nanite skeletal meshes on | nanite | 16.56 | 15.71 | +0.85 | 25.66 | 29.19 |
| run 2 | Nanite skeletal meshes on | preset_ultra | 61.30 | 64.23 | -2.93 | 91.64 | 95.14 |
| run 2 | Nanite foliage on | baseline | 13.40 | 11.99 | +1.41 | 22.87 | 21.06 |
| run 2 | Nanite foliage on | nanite | 14.79 | 15.71 | -0.92 | 22.72 | 29.19 |
| run 2 | Nanite foliage on | preset_ultra | 56.53 | 64.23 | -7.70 | 90.12 | 95.14 |
| run 3 | Nanite skeletal meshes on | baseline | 13.32 | 15.87 | -2.55 | 23.77 | 30.13 |
| run 3 | Nanite skeletal meshes on | nanite | 14.65 | 16.27 | -1.62 | 23.38 | 25.81 |
| run 3 | Nanite skeletal meshes on | preset_ultra | 55.34 | 61.54 | -6.20 | 86.97 | 87.85 |
| run 3 | Nanite foliage on | baseline | 13.41 | 15.87 | -2.46 | 23.38 | 30.13 |
| run 3 | Nanite foliage on | nanite | 14.76 | 16.27 | -1.51 | 23.47 | 25.81 |
| run 3 | Nanite foliage on | preset_ultra | 55.93 | 61.54 | -5.61 | 83.74 | 87.85 |

### The same configuration across runs

Average frame time in ms, with the mean GPU clock of the lap in MHz where it was logged. Runs 1 and 2 started each process cool, run 3 flew two Epic laps first, run 4 two baseline laps.

| Configuration | run 1 | run 2 | run 3 | run 3 repeat | run 4 |
|---|---:|---:|---:|---:|---:|
| baseline | 11.82 | 11.99 | 15.87 @ 1714 | 13.26 @ 1565 | 12.59 @ 1524 |
| megalights | 12.92 | 13.04 | 15.85 @ 1668 |  | 13.51 @ 1495 |
| gi_lumenlite | 11.04 | 11.13 | 13.87 @ 1730 |  | 11.39 @ 1573 |
| gi_lumen | 12.36 | 12.39 | 15.83 @ 1691 |  | 12.81 @ 1519 |
| vsm_low | 12.61 | 12.67 | 15.08 @ 1675 |  | 13.08 @ 1490 |
| vsm_high | 13.03 | 13.05 | 15.94 @ 1652 |  | 13.29 @ 1511 |
| vsm_epic | 13.53 | 13.56 | 16.45 @ 1650 |  | 13.66 @ 1470 |
| fog_low | 12.83 | 12.80 | 15.73 @ 1639 |  | 12.90 @ 1490 |
| fog_high | 14.75 | 14.78 | 17.49 @ 1612 |  | 14.80 @ 1436 |
| tsr_native | 17.05 | 17.97 | 20.18 @ 1686 | 18.16 @ 1543 | 17.20 @ 1455 |
| tsr_quality | 10.58 | 13.60 | 12.55 @ 1708 | 11.28 @ 1625 | 10.79 @ 1545 |
| tsr_balanced | 9.29 | 13.27 | 11.11 @ 1728 | 9.97 @ 1670 | 9.49 @ 1592 |
| tsr_performance | 8.25 | 9.97 | 9.66 @ 1757 | 8.80 @ 1680 | 8.43 @ 1645 |
| nanite |  | 15.71 | 16.27 @ 1668 |  | 14.12 @ 1488 |
| effects_epic |  | 14.01 | 14.80 @ 1628 |  | 12.67 @ 1524 |
| viewdistance_epic |  | 14.63 | 15.40 @ 1684 |  | 12.58 @ 1480 |
| motionblur |  | 15.11 | 16.55 @ 1698 |  | 13.06 @ 1452 |
| preset_low |  | 6.78 | 6.81 @ 1768 |  | 5.88 @ 1680 |
| preset_medium |  | 11.90 | 11.24 @ 1710 |  | 9.97 @ 1665 |
| preset_high |  | 15.91 | 15.53 @ 1671 |  | 13.48 @ 1535 |
| preset_epic |  | 32.41 | 29.95 @ 1502 |  | 27.65 @ 1378 |
| preset_ultra |  | 64.23 | 61.54 @ 1461 |  | 56.49 @ 1348 |

With Nanite limited to Virtual Shadow Maps, runs 2 to 4 and the probes ran 12 game processes without a crash or a GPU fault.

### Limits of this benchmark

- The geometry is placeholder: engine basic shapes of 48 to 262 triangles, instanced into grass, bushes and trees. Nanite and Nanite Foliage are built for dense geometry, so this level cannot show their benefit, only their fixed cost.
- The level has 20 Nanite enabled mannequins and no Nanite foliage content, so the two experimental switches have little or nothing to act on.
- One machine, one resolution. Other GPUs will scale differently, in particular for TSR and the presets.
- Render thread time is not available in UE 5.8, see the method.

## Observations

### Nanite without Virtual Shadow Maps crashes or hitches

The first full run switched Nanite on while Virtual Shadow Maps were off, as the single feature run on top of the baseline. With the 60 shadow casting point lights of the city section, the renderer then draws Nanite geometry into six conventional cube shadow maps per light.

- **Crash.** Two of three processes died in the D3D12 layer the moment Nanite was switched on at runtime, with `No command list slot was available. Too many residency sets are open concurrently.` The breadcrumbs point at `Nanite Cubemap` passes inside `ShadowDepths`. The third process survived the same switch.
- **Hitching.** Every lap that survived hitched hard. In three fresh processes with Nanite already on at startup, none crashed, but the 1% low stayed between 215 and 232 ms, against 21 ms for the baseline.

Three engine switches were tried against it, two laps each in their own process:

| Probe | Lap 1 avg ms | Lap 1 1% low ms | Lap 2 avg ms | Lap 2 1% low ms | Result |
|---|---:|---:|---:|---:|---|
| Nanite on, VSM off, run 1 | 16.31 | 216.77 | 16.58 | 215.16 | no crash, hitching |
| Nanite on, VSM off, run 2 | 17.94 | 229.28 | 17.63 | 228.19 | no crash, hitching |
| Nanite on, VSM off, run 3 | 18.06 | 231.48 | 18.36 | 232.38 | no crash, hitching |
| plus `r.ParallelShadows 0` | 16.43 | 216.71 | 16.64 | 215.63 | no change |
| plus `r.RDG.ParallelExecute 0` | 17.08 | 235.34 | 17.73 | 225.59 | no change |
| plus `D3D12.ResidencyManagement=0` at startup | | | | | GPU page fault on the first frame, process terminated |

None of them helps, so the combination is not offered. The settings object only sets `r.Nanite 1` while Virtual Shadow Maps are on. The menu shows the Nanite row as "On, needs virtual shadows" when the switch is on but inactive. The Low preset, which has Virtual Shadow Maps off, now has Nanite off. The Nanite measurement therefore runs with Virtual Shadow Maps at Low and is compared against the Virtual Shadow Maps Low run.

### Global illumination off is not the cheapest mode

In every run Lumen Lite measured faster than the baseline with global illumination off. The engine explains it: at the High and Epic GI scalability levels it enables distance field ambient occlusion for the sky light, and screen space ambient occlusion runs as well. Lumen replaces both with its own short range occlusion.

Two probes, each two laps of one process at the sustained clock of about 1550 to 1630 MHz:

| Probe | Baseline avg ms | Lumen Lite avg ms | Lumen Lite minus baseline |
|---|---:|---:|---:|
| `r.DistanceFieldAO 0` | 11.69 | 11.14 | -0.55 ms |
| `r.DistanceFieldAO 0` and `r.AmbientOcclusionLevels 0` | 10.94 | 12.56 | +1.62 ms |

With both occlusion passes off, Lumen Lite costs what one would expect. The two passes together cost roughly 2.5 ms at 5120 x 1440, so turning global illumination off saves less than it seems unless ambient occlusion is scaled down with it.

## Recommended defaults

These defaults were adopted in G03-B03 (D-030) and are now the preset table in `UCyberGameUserSettings::GetPresetFeatures`. Where the decisions go further than the recommendations, the decisions win: Ultra runs at native resolution without upscaling (D-028), the engine's Cine level only lives in the Cinematic preset (D-029), and hardware detection stops at High on displays above 4 megapixels (D-031). None of the new presets has been measured yet; that belongs to the night run (D-032).

### Applied preset table

| Preset | Scalability | MegaLights | Global illumination | Virtual shadow maps | Volumetric fog | Anti aliasing | Nanite | Effects | View distance | Motion blur |
|---|---|---|---|---|---|---|---|---|---|---|
| Low | 0 | Off | Off | Off | Off | TSR Performance | Off | Low | Near | Off |
| Medium | 1 | Off | Lumen Lite | Low | Off | TSR Balanced | Off | Medium | Medium | Off |
| High, the default | 2 | On | Lumen | High | Low | TSR Quality | Off | Epic | Epic | Off |
| Epic | 3 | On | Lumen | Epic | Medium | TSR Quality | Off | Epic | Epic | Off |
| Ultra | 3 | On | Lumen | Epic | Medium | TSR Native | On | Epic | Epic | Off |
| Cinematic, capture only | 4, Cine | On | Lumen | Epic | High | TSR Native | On | Epic | Epic | On |

Resolution scale is 100 percent in every preset and both experimental switches are off. Epic keeps Virtual Shadow Maps at Epic, following the verdict below that Epic only costs more than it gives beneath the Epic preset. Cinematic keeps motion blur because it is meant for footage. Saves from before this table reset to the new preset defaults once, through a settings version bump.

### Default preset

**High.** In run 4 it averages 13.48 ms (74 fps) with a 1% low of 19.14 ms (52 fps), and 13.5 to 15.9 ms across all passes. At this resolution Epic averages 27.7 to 32.4 ms (31 to 36 fps) and Ultra 56.5 to 64.2 ms (16 to 18 fps), which is not playable for a shooting game. Medium is the choice when a steady 60 fps matters more than the picture: 9.97 ms average, 13.62 ms 1% low.

**Hardware detection puts this machine on Epic.** The engine benchmark rates the RTX 3090 at the top of its scale and does not know about the 7.4 megapixel display. Recommendation: cap the detected preset at High, or lower it one step when the desktop resolution is above about 4 megapixels. Not changed, this needs a decision.

### Per option

| Option | Recommended default | Evidence from run 4 | Verdict |
|---|---|---|---|
| MegaLights | On from High upward | +0.90 ms average, 1% low 3.52 ms better with 60 shadow casting point lights | Worth it wherever many shadowed lights meet; the High preset has it off today |
| Global illumination | Lumen at High and above, Lumen Lite at Medium, Off at Low | Lumen +0.10 ms, Lumen Lite -1.26 ms against the baseline | Lumen is nearly free here because it replaces both ambient occlusion passes; GI off only saves time at Low, where the scalability level drops those passes too |
| Virtual shadow maps | High | Low +0.40, High +0.70, Epic +1.14 ms; High and Epic worsen the 1% low by about 1.7 ms | Epic costs more than it gives below the Epic preset |
| Volumetric fog | Low, Medium at most | Low +0.47 ms, High +2.38 ms | High costs more than it gives; Ultra should use Medium |
| Anti aliasing and upscaling | TSR Quality | Native +4.68 ms against no anti aliasing; Quality -1.82, Balanced -3.15, Performance -4.23 ms | TSR Native costs 6.5 ms more than Quality at this resolution, more than it gives; Epic should use Quality, Native can stay for Ultra |
| Nanite static meshes | Off until production geometry exists | Nanite alone +1.02 ms on top of VSM Low, no worse 1% low | On these placeholder meshes Nanite costs a millisecond and has nothing to gain; measure again with real assets before it becomes a default |
| Effects and particle density | Epic | +0.04 ms | Free on this content, 20 fountains |
| View distance | Epic | +0.10 ms | Free on this small level |
| Motion blur | Off | +0.62 ms | Cheap, but it smears moving targets; off is a gameplay reason, not a performance one |
| Frame rate cap | Unlimited | not measured | A cap only helps where the machine runs faster than the display |
| VSync | Off | not measured | Adds input latency, which hurts aiming |
| Experimental Nanite skeletal meshes | Off | Differences of -6.2 to +2.0 ms between processes, larger than any effect | No measurable benefit with 20 mannequins; stays off and experimental |
| Experimental Nanite foliage | Off | Same spread, -7.7 to +1.4 ms | The level has no Nanite foliage for it to act on; needs real vegetation first (backlog) |

Taken together for High: MegaLights on, motion blur off and Nanite off would move it by roughly +0.9, -0.6 and -1.0 ms, so it stays near 13 ms. Epic with TSR Quality instead of Native would drop by about 6.5 ms to roughly 21 ms (48 fps); these are sums of single feature costs, not measured.
