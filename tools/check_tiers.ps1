# CYB3RGUN THEGAME content tier check (D-067, D-069).
#
# Closed tier content is licensed separately and must never enter git history. This script fails when any closed path
# is tracked in the index or staged for the next commit, and names every file it found. It also fails when a closed
# path is missing from .gitignore, so the ignore rules and this list cannot drift apart. docs/content-tiers.md holds the
# same list with the reasons.
#
# Run it from anywhere inside the repository before every push:
#
#     powershell -ExecutionPolicy Bypass -File tools/check_tiers.ps1
#
# Exit code 0 means clean, 1 means a violation was found, 2 means the check itself could not run.

$ErrorActionPreference = 'Stop'

$closedPaths = @(
    'CYB3RGUN/Content/CYB3RGUN/Characters/'
    'CYB3RGUN/Content/CYB3RGUN/Enemies/Bodies/'
    'CYB3RGUN/Content/CYB3RGUN/Environments/'
    'CYB3RGUN/Content/CYB3RGUN/Audio/'
    'CYB3RGUN/Content/CYB3RGUN/Personalities/'
    'CYB3RGUN/Content/CYB3RGUN/Licensed/'
    'CYB3RGUN/Content/Fab/'
    'CYB3RGUN/Content/Megascans/'
    'CYB3RGUN/Content/MetaHumans/'
)

$repo = Split-Path -Parent $PSScriptRoot
Push-Location $repo
try {
    git rev-parse --is-inside-work-tree 2>$null | Out-Null
    if ($LASTEXITCODE -ne 0) {
        Write-Host "check_tiers: $repo is not a git work tree."
        exit 2
    }

    $violations = New-Object System.Collections.Generic.List[string]

    # every tracked file, which includes everything already staged for the next commit
    $tracked = @(git -c core.quotepath=off ls-files --cached)
    # staged additions, renames and copies, listed separately so the report says what the next commit would add
    $staged = @(git -c core.quotepath=off diff --cached --name-only --diff-filter=ACR)

    foreach ($path in $closedPaths) {
        foreach ($file in $staged) {
            if ($file.StartsWith($path, [System.StringComparison]::OrdinalIgnoreCase)) {
                $violations.Add("staged:  $file")
            }
        }
        foreach ($file in $tracked) {
            if ($file.StartsWith($path, [System.StringComparison]::OrdinalIgnoreCase) -and -not ($staged -contains $file)) {
                $violations.Add("tracked: $file")
            }
        }
    }

    $ignoreRules = @()
    if (Test-Path '.gitignore') {
        $ignoreRules = @(Get-Content '.gitignore' | ForEach-Object { $_.Trim() })
    }
    $unignored = @($closedPaths | Where-Object { $ignoreRules -notcontains $_ })

    if ($violations.Count -eq 0 -and $unignored.Count -eq 0) {
        Write-Host "check_tiers: clean. No closed tier content is tracked or staged, and all $($closedPaths.Count) closed paths are ignored."
        exit 0
    }

    if ($violations.Count -gt 0) {
        Write-Host "check_tiers: FAILED. Closed tier content is in git ($($violations.Count) files):"
        $violations | ForEach-Object { Write-Host "  $_" }
        Write-Host 'Unstage with: git restore --staged <file>. A file that is already committed needs the architect''s decision (D-069).'
    }
    if ($unignored.Count -gt 0) {
        Write-Host "check_tiers: FAILED. Closed paths missing from .gitignore:"
        $unignored | ForEach-Object { Write-Host "  $_" }
    }
    exit 1
}
finally {
    Pop-Location
}
