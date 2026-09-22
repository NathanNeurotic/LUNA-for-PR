# Original LUNA code: Danny Nunez (dnunezx) 2026
[CmdletBinding()]
param(
    [string]$PackagePath = ''
)

$ErrorActionPreference = 'Stop'
$workspace = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($PackagePath)) {
    $PackagePath = Join-Path $workspace 'dist/LUNA-FMCB-mc0'
}

$package = (Resolve-Path -LiteralPath $PackagePath).Path
$app = Join-Path $package 'APP_LUNA'
$required = @(
    'luna.elf',
    'luna.yaml',
    'neutrino.elf',
    'version.txt',
    'config/system.toml',
    'config/bsd-ata.toml',
    'modules/ata_bd.irx',
    'modules/ee_core.elf'
)

foreach ($relative in $required) {
    $path = Join-Path $app $relative
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Missing FMCB runtime file: $relative"
    }
}

$config = Get-Content -LiteralPath (Join-Path $app 'luna.yaml')
if (-not ($config -match '^mode:\s*ata\s*$')) {
    throw 'luna.yaml does not restrict LUNA to the ATA backend.'
}
if (-not ($config -match '^return_path:\s*mc0:/LUNA/luna\.elf\s*$')) {
    throw 'luna.yaml does not return directly to mc0:/LUNA/luna.elf.'
}

foreach ($forbidden in @('ART', 'favorites.txt', 'cache.bin', 'lastTitle.bin', 'global.yaml')) {
    if (Get-ChildItem -LiteralPath $app -Recurse -Force |
        Where-Object { $_.Name -ieq $forbidden }) {
        throw "Per-drive data must not be packaged on the memory card: $forbidden"
    }
}

$checksumPath = Join-Path $package 'SHA256SUMS.txt'
if (-not (Test-Path -LiteralPath $checksumPath -PathType Leaf)) {
    throw 'Missing SHA256SUMS.txt.'
}

$checksumLines = @(Get-Content -LiteralPath $checksumPath)
$packagedFiles = @(Get-ChildItem -LiteralPath $package -File -Recurse |
    Where-Object { $_.FullName -ne $checksumPath })
if ($checksumLines.Count -ne $packagedFiles.Count) {
    throw "Checksum manifest has $($checksumLines.Count) entries for $($packagedFiles.Count) files."
}

foreach ($line in $checksumLines) {
    if ($line -notmatch '^([0-9a-f]{64})  (.+)$') {
        throw "Malformed checksum line: $line"
    }
    $expected = $Matches[1]
    $relative = $Matches[2].Replace('/', [IO.Path]::DirectorySeparatorChar)
    $path = Join-Path $package $relative
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Checksum target is missing: $relative"
    }
    $actual = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -ne $expected) {
        throw "Checksum mismatch: $relative"
    }
}

$promotedLauncher = Join-Path $workspace 'dist/LUNA-Release-Candidate.elf'
if (Test-Path -LiteralPath $promotedLauncher -PathType Leaf) {
    $packagedLauncherHash = (Get-FileHash -LiteralPath (Join-Path $app 'luna.elf') -Algorithm SHA256).Hash
    $promotedLauncherHash = (Get-FileHash -LiteralPath $promotedLauncher -Algorithm SHA256).Hash
    if ($packagedLauncherHash -ne $promotedLauncherHash) {
        throw 'Packaged luna.elf is not the promoted Release Candidate.'
    }
}

$bytes = (Get-ChildItem -LiteralPath $app -Recurse -File | Measure-Object Length -Sum).Sum
Write-Host "Verified FMCB package at $package"
Write-Host "APP_LUNA size: $bytes bytes"
Write-Host 'Runtime is on mc0; Favorites and other library state remain per ATA hard drive.'
