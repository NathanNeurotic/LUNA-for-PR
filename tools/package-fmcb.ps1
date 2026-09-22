# Original LUNA code: Danny Nunez (dnunezx) 2026
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

$workspace = Split-Path -Parent $PSScriptRoot
$dist = Join-Path $workspace 'dist'
$package = Join-Path $dist 'LUNA-FMCB-mc0'
$app = Join-Path $package 'APP_LUNA'
$archive = Join-Path $dist 'LUNA-FMCB-mc0.zip'

$launcher = Join-Path $dist 'LUNA-Release-Candidate.elf'
$launcherConfig = Join-Path $workspace 'nhddl/examples/luna.yaml'
$neutrinoRoot = Join-Path $workspace 'neutrino/ee/loader'
$required = @(
    $launcher,
    $launcherConfig,
    (Join-Path $neutrinoRoot 'neutrino.elf'),
    (Join-Path $neutrinoRoot 'version.txt'),
    (Join-Path $neutrinoRoot 'config/system.toml'),
    (Join-Path $neutrinoRoot 'modules/ee_core.elf'),
    (Join-Path $workspace 'FMCB.md')
)

foreach ($path in $required) {
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Required FMCB package input is missing: $path"
    }
}

if (Test-Path -LiteralPath $package) {
    $resolvedPackage = (Resolve-Path -LiteralPath $package).Path
    $resolvedDist = (Resolve-Path -LiteralPath $dist).Path
    if ((Split-Path -Parent $resolvedPackage) -ne $resolvedDist -or
        (Split-Path -Leaf $resolvedPackage) -ne 'LUNA-FMCB-mc0') {
        throw "Refusing to replace unexpected package path: $resolvedPackage"
    }
    Remove-Item -LiteralPath $resolvedPackage -Recurse -Force
}

New-Item -ItemType Directory -Path $app -Force | Out-Null
Copy-Item -LiteralPath $launcher -Destination (Join-Path $app 'luna.elf')
Copy-Item -LiteralPath $launcherConfig -Destination (Join-Path $app 'luna.yaml')
Copy-Item -LiteralPath (Join-Path $neutrinoRoot 'neutrino.elf') -Destination $app
Copy-Item -LiteralPath (Join-Path $neutrinoRoot 'version.txt') -Destination $app
Copy-Item -LiteralPath (Join-Path $neutrinoRoot 'config') -Destination $app -Recurse
Copy-Item -LiteralPath (Join-Path $neutrinoRoot 'modules') -Destination $app -Recurse
Copy-Item -LiteralPath (Join-Path $workspace 'FMCB.md') -Destination (Join-Path $package 'README.md')

$licenseSource = Join-Path $dist 'LUNA-hardware-test-1/LICENSES'
if (Test-Path -LiteralPath $licenseSource) {
    Copy-Item -LiteralPath $licenseSource -Destination $package -Recurse
}

$packageRoot = (Resolve-Path -LiteralPath $package).Path
$checksumLines = Get-ChildItem -LiteralPath $package -File -Recurse |
    Where-Object { $_.Name -ne 'SHA256SUMS.txt' } |
    Sort-Object FullName |
    ForEach-Object {
        $relative = $_.FullName.Substring($packageRoot.Length + 1).Replace('\', '/')
        $hash = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        "$hash  $relative"
    }
Set-Content -LiteralPath (Join-Path $package 'SHA256SUMS.txt') -Value $checksumLines -Encoding ascii

if (Test-Path -LiteralPath $archive) {
    Remove-Item -LiteralPath $archive -Force
}
Compress-Archive -LiteralPath $package -DestinationPath $archive -CompressionLevel Optimal

$distChecksums = Join-Path $dist 'SHA256SUMS.txt'
$archiveHash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
$distChecksumLines = @()
if (Test-Path -LiteralPath $distChecksums) {
    $distChecksumLines = @(Get-Content -LiteralPath $distChecksums |
        Where-Object { $_ -notmatch '  LUNA-FMCB-mc0\.zip$' })
}
$distChecksumLines += "$archiveHash  LUNA-FMCB-mc0.zip"
Set-Content -LiteralPath $distChecksums -Value ($distChecksumLines | Sort-Object) -Encoding ascii

Write-Host "Created $package"
Write-Host "Created $archive"
Write-Host "Updated $distChecksums"
