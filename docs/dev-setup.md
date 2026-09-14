# Dev Setup

Local environment notes for working on the game track. Everything here applies to one workstation and is not versioned engine or project configuration, unless stated otherwise.

## Required tools

| Tool | Notes |
|------|-------|
| Unreal Engine 5.8.2 | Installed at `C:\Program Files\Epic Games\UE_5.8` |
| Git with Git LFS | LFS must be installed before the first clone, see D-001 |
| MCP client | Reads the repo root `.mcp.json`, which points at the editor on the loopback address, see D-009 |

The project enables the Unreal MCP and All Toolsets plugins. The editor serves MCP at `http://127.0.0.1:8000/mcp` once the server is started.

## Opening the project

Open `CYB3RGUN/CYB3RGUN.uproject` directly, by double clicking it or by passing its path to `UnrealEditor.exe` as in the build loop below. Never start the editor through the Epic Games Launcher and pick the project afterwards: the engine then resolves the project only after it has computed its binary paths, and reports a compile error that is not real.

## Required local setting: background CPU throttle off

The editor must not throttle itself when its window is in the background (D-022).

MCP driven runs never have the editor in the foreground. With the throttle on, Play In Editor drops to about 3 fps. Timers stretch, projectiles tunnel and hit registration looks broken when it is not.

Set it once per workstation in either of these ways:

- **Editor UI.** Edit, Editor Preferences, General, Performance, clear "Use Less CPU when in Background".
- **Config file.** The setting is per user and per engine version, not per project. With the editor closed, add the section below to `%LOCALAPPDATA%\UnrealEngine\5.8\Saved\Config\WindowsEditor\EditorSettings.ini`.

```ini
[/Script/UnrealEd.EditorPerformanceSettings]
bThrottleCPUWhenNotForeground=False
```

The file lives outside the repository, so every workstation sets this itself. Until it is set, an MCP driven session can switch it off in memory on the `Default__EditorPerformanceSettings` object before starting play.

## Build loop

Live Coding is not used. It crashes when new reflected types are added. Every C++ change goes through this loop:

1. **Close the editor.** Close it normally so it can save. Also end `LiveCodingConsole.exe` if it is still running.
2. **Build from the command line.**

```bat
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" CYB3RGUNEditor Win64 Development -project="C:\Projects\CYB3RGUN\THEGAME\CYB3RGUN\CYB3RGUN.uproject" -waitmutex
```

3. **Relaunch with the MCP server.**

```bat
"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "C:\Projects\CYB3RGUN\THEGAME\CYB3RGUN\CYB3RGUN.uproject" -AutoDeclinePackageRecovery -ExecCmds="ModelContextProtocol.StartServer"
```

4. **Wait for MCP.** The server is up when `http://127.0.0.1:8000/mcp` answers a plain GET with HTTP 405.

## Automated play verification

Scenarios ship console commands for automated runs in non shipping builds: `DoorRange.*`, `Zombie.*` and `Rail.*`.

- **Console input.** Drive the editor console through the Slate inspector toolset. Select the main editor window first, because the Message Log window takes focus after a level load. Press Shift+F1 during play to release the mouse, then type into the console box.
- **Screenshots.** The editor image capture tool returns PNG data inline. Decode it into `CYB3RGUN/Saved/Screenshots`.
- **Respawn.** Encounter levels respawn the player (D-059): the zombie test puts a fallen player back at a player start after 3 s with the starting loadout, and the rail rider gets back up after 3 s and the ride goes on. Long automated runs therefore need no change to enemy damage. `Player.Damage <amount>` takes health off the player to test it.
