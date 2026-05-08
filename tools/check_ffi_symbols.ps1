param(
    [Parameter(Mandatory = $true)]
    [string]$BinaryPath,
    [Parameter(Mandatory = $true)]
    [string]$BaselinePath,
    [string]$ReportPath = "",
    [switch]$UpdateBaseline
)

$ErrorActionPreference = "Stop"

function Get-ToolPath([string[]]$Names) {
    foreach ($name in $Names) {
        $cmd = Get-Command $name -ErrorAction SilentlyContinue
        if ($cmd) {
            return $cmd.Source
        }
    }
    return $null
}

function Get-ExportedSymbols([string]$Binary) {
    if (!(Test-Path -LiteralPath $Binary)) {
        throw "Binary not found: $Binary"
    }

    $symbols = @()

    if ($env:OS -eq "Windows_NT") {
        $dumpbin = Get-ToolPath @("dumpbin")
        if ($dumpbin) {
            $lines = & $dumpbin /nologo /exports $Binary
            foreach ($line in $lines) {
                if ($line -match "(mce_[a-zA-Z0-9_]+)\s*$") {
                    $symbols += $matches[1]
                }
            }
        } else {
            $objdump = Get-ToolPath @("objdump")
            if (!$objdump) {
                throw "Neither dumpbin nor objdump is available in PATH."
            }
            $lines = & $objdump -p $Binary
            foreach ($line in $lines) {
                if ($line -match "(mce_[a-zA-Z0-9_]+)\s*$") {
                    $symbols += $matches[1]
                }
            }
        }
    } else {
        $nm = Get-ToolPath @("nm")
        if (!$nm) {
            throw "nm is required on non-Windows platforms."
        }
        $lines = & $nm -D --defined-only $Binary
        foreach ($line in $lines) {
            if ($line -match "(mce_[a-zA-Z0-9_]+)\s*$") {
                $symbols += $matches[1]
            }
        }
    }

    return $symbols | Sort-Object -Unique
}

function Read-BaselineSymbols([string]$Path) {
    if (!(Test-Path -LiteralPath $Path)) {
        throw "Baseline not found: $Path"
    }
    return (Get-Content -LiteralPath $Path) `
        | ForEach-Object { $_.Trim() } `
        | Where-Object { $_ -ne "" -and -not $_.StartsWith("#") } `
        | Sort-Object -Unique
}

function Write-Report(
    [string]$Path,
    [string[]]$Baseline,
    [string[]]$Current,
    [string[]]$Added,
    [string[]]$Removed
) {
    if ([string]::IsNullOrWhiteSpace($Path)) {
        return
    }

    $dir = Split-Path -Parent $Path
    if ($dir -and !(Test-Path -LiteralPath $dir)) {
        New-Item -ItemType Directory -Path $dir | Out-Null
    }

    $status = if ($Added.Count -eq 0 -and $Removed.Count -eq 0) { "PASS" } else { "FAIL" }
    $lines = @()
    $lines += "ffi_symbol_guard_status=$status"
    $lines += "binary_path=$BinaryPath"
    $lines += "baseline_path=$BaselinePath"
    $lines += "checked_at_utc=$([DateTime]::UtcNow.ToString('o'))"
    $lines += "baseline_count=$($Baseline.Count)"
    $lines += "current_count=$($Current.Count)"
    $lines += "added_count=$($Added.Count)"
    $lines += "removed_count=$($Removed.Count)"
    $lines += ""
    $lines += "[added]"
    if ($Added.Count -eq 0) {
        $lines += "(none)"
    } else {
        $lines += $Added
    }
    $lines += ""
    $lines += "[removed]"
    if ($Removed.Count -eq 0) {
        $lines += "(none)"
    } else {
        $lines += $Removed
    }
    $lines += ""
    $lines += "[current]"
    $lines += $Current

    Set-Content -LiteralPath $Path -Value $lines -Encoding UTF8
    Write-Host "Wrote symbol report: $Path"
}

$current = Get-ExportedSymbols -Binary $BinaryPath

if ($UpdateBaseline) {
    $dir = Split-Path -Parent $BaselinePath
    if ($dir -and !(Test-Path -LiteralPath $dir)) {
        New-Item -ItemType Directory -Path $dir | Out-Null
    }
    Set-Content -LiteralPath $BaselinePath -Value $current
    Write-Host "Updated symbol baseline: $BaselinePath"
    exit 0
}

$baseline = Read-BaselineSymbols -Path $BaselinePath

$added = $current | Where-Object { $_ -notin $baseline }
$removed = $baseline | Where-Object { $_ -notin $current }

Write-Report -Path $ReportPath -Baseline $baseline -Current $current -Added $added -Removed $removed

if ($added.Count -eq 0 -and $removed.Count -eq 0) {
    Write-Host "FFI symbol check passed. Symbols: $($current.Count)"
    exit 0
}

Write-Error "FFI symbol baseline mismatch."
if ($added.Count -gt 0) {
    Write-Host "Added symbols:"
    $added | ForEach-Object { Write-Host "  + $_" }
}
if ($removed.Count -gt 0) {
    Write-Host "Removed symbols:"
    $removed | ForEach-Object { Write-Host "  - $_" }
}
exit 1
