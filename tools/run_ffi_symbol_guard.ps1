param(
    [string]$BuildDir = "build_codex",
    [string]$Config = "Debug",
    [string]$ArtifactDir = "",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

function Require-Command([string]$Name) {
    $cmd = Get-Command $Name -ErrorAction SilentlyContinue
    if (!$cmd) {
        throw "Required command not found in PATH: $Name"
    }
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$resolvedBuildDir = if ([System.IO.Path]::IsPathRooted($BuildDir)) {
    $BuildDir
} else {
    Join-Path $repoRoot $BuildDir
}

$reportPath = Join-Path $resolvedBuildDir "ffi_symbol_report.txt"

Require-Command "cmake"
Require-Command "ctest"

Push-Location $repoRoot
try {
    if (-not $SkipBuild) {
        cmake -S . -B $resolvedBuildDir -DBUILD_TESTING=ON
        cmake --build $resolvedBuildDir --config $Config
    }

    ctest --test-dir $resolvedBuildDir -C $Config --output-on-failure -R pure_core_ffi_symbol_guard

    if (!(Test-Path -LiteralPath $reportPath)) {
        throw "FFI symbol report not found: $reportPath"
    }

    if (-not [string]::IsNullOrWhiteSpace($ArtifactDir)) {
        $resolvedArtifactDir = if ([System.IO.Path]::IsPathRooted($ArtifactDir)) {
            $ArtifactDir
        } else {
            Join-Path $repoRoot $ArtifactDir
        }
        New-Item -ItemType Directory -Path $resolvedArtifactDir -Force | Out-Null
        $target = Join-Path $resolvedArtifactDir "ffi_symbol_report.txt"
        Copy-Item -LiteralPath $reportPath -Destination $target -Force
        Write-Host "Copied symbol report artifact to: $target"
    }

    Write-Host "FFI symbol guard passed."
    Write-Host "Report: $reportPath"
} finally {
    Pop-Location
}
