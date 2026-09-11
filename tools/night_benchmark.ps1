# CYB3RGUN THEGAME night benchmark (D-032).
#
# Measures the six quality presets plus Ultra at 150 and 200 percent resolution scale in one game process, every
# configuration between two baseline laps (the run 4 method in docs/benchmark.md), at the primary display's resolution.
# It takes about 20 minutes and keeps the GPU busy the whole time: run it with the Unreal Editor closed and nothing
# else running on the machine.
#
# Start it from the repository root with one command:
#
#     powershell -ExecutionPolicy Bypass -File tools/night_benchmark.ps1
#
# Results: CYB3RGUN/Saved/Benchmark/night_<time>.log with one BENCH line per lap, the same rows appended to
# CYB3RGUN/Saved/Benchmark/bench_results.csv, and night_<time>_gpu.csv with the GPU clock every 5 s when nvidia-smi
# is installed. Every BENCH line names the GPU, the driver version and the resolution it was taken at.

param(
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
)

$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$project = Join-Path $repo 'CYB3RGUN\CYB3RGUN.uproject'
$outDir = Join-Path $repo 'CYB3RGUN\Saved\Benchmark'

if (-not (Test-Path $Editor)) { throw "Unreal Editor not found at $Editor. Pass -Editor <path to UnrealEditor.exe>." }
if (-not (Test-Path $project)) { throw "Project not found at $project." }
if (Get-Process -Name UnrealEditor -ErrorAction SilentlyContinue) { throw 'An Unreal Editor process is running. Close it first, a second process would share the GPU and falsify the numbers.' }

New-Item -ItemType Directory -Force -Path $outDir | Out-Null
$stamp = Get-Date -Format 'yyyyMMdd-HHmm'
$log = Join-Path $outDir "night_$stamp.log"
$gpuLog = Join-Path $outDir "night_${stamp}_gpu.csv"

# the physical resolution of the active display; the WMI value is not affected by Windows display scaling
$video = Get-CimInstance Win32_VideoController | Where-Object { $_.CurrentHorizontalResolution -gt 0 } | Select-Object -First 1
if ($video) {
    $width = [int]$video.CurrentHorizontalResolution
    $height = [int]$video.CurrentVerticalResolution
} else {
    Add-Type -AssemblyName System.Windows.Forms
    $width = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds.Width
    $height = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds.Height
}

Write-Host "Night benchmark $stamp at $width x $height"
Write-Host "Log: $log"

# the GPU clock beside every lap, the driver's own tool, only when it is present
$logger = $null
if (Get-Command nvidia-smi -ErrorAction SilentlyContinue) {
    $logger = Start-Process -FilePath 'nvidia-smi' -NoNewWindow -PassThru -RedirectStandardOutput $gpuLog -ArgumentList @(
        '--query-gpu=timestamp,temperature.gpu,clocks.gr,power.draw,utilization.gpu,clocks_event_reasons.active',
        '--format=csv', '-l', '5')
}

# the experimental Nanite switches are forced off so the preset laps are comparable with run 4
$startup = '[ConsoleVariables]:r.Nanite.AllowSkinnedMeshes=0,[ConsoleVariables]:r.Nanite.Foliage=0,[ConsoleVariables]:r.Nanite.AllowAssemblies=0'
$commands = "Settings.Set WindowMode Borderless, Settings.Set Resolution ${width}x${height}, Bench.Suite night quit"
$arguments = @(
    "`"$project`"", '/Game/CYB3RGUN/Maps/Lvl_Benchmark', '-game', "-ResX=$width", "-ResY=$height", '-windowed', '-WinX=0', '-WinY=0',
    '-NoSplash', '-unattended', '-log', "`"-abslog=$log`"", "-ini:Engine:$startup", "`"-ExecCmds=$commands`"")

$started = Get-Date
try {
    $game = Start-Process -FilePath $Editor -ArgumentList $arguments -PassThru -Wait
    $exitCode = $game.ExitCode
} finally {
    if ($logger -and -not $logger.HasExited) { Stop-Process -Id $logger.Id -Force }
}
$minutes = [math]::Round(((Get-Date) - $started).TotalMinutes, 1)

Write-Host "Finished after $minutes minutes, exit code $exitCode"
if (Test-Path $log) {
    $bench = Select-String -Path $log -Pattern 'BENCH\|' | ForEach-Object { $_.Line -replace '^.*BENCH\|', '' }
    $crash = Select-String -Path $log -Pattern 'Fatal error|Crash in runnable|GPU crash detected|Too many residency' -Quiet
    Write-Host "$($bench.Count) laps measured, the night suite has 19"
    $bench | ForEach-Object { Write-Host $_ }
    if ($crash) { Write-Host 'The log contains a crash signature, see the log.' }
} else {
    Write-Host 'No log was written, the game did not start.'
}
