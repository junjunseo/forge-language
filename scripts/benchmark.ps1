param(
    [string]$ModuleCounts = "50,100,200",
    [ValidateRange(1, 1000)]
    [int]$Iterations = 7
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$compiler = (Get-Command g++ -ErrorAction Stop).Source
$counts = @()

foreach ($value in ($ModuleCounts -split ',')) {
    $count = 0
    if (-not [int]::TryParse($value.Trim(), [ref]$count) -or $count -lt 2) {
        throw "ModuleCounts must be a comma-separated list of integers greater than or equal to 2"
    }
    $counts += $count
}

if ($counts.Count -eq 0) {
    throw "ModuleCounts must contain at least one module count"
}

Push-Location $root
try {
    New-Item -ItemType Directory -Force -Path "build" | Out-Null

    Write-Host "Build: benchmark/benchmarkChecker.cpp"
    & $compiler `
        -std=c++17 `
        -O2 `
        -DNDEBUG `
        -Wall `
        -Wextra `
        -pedantic `
        -Isrc `
        benchmark/benchmarkChecker.cpp `
        -o build/benchmarkChecker.exe

    if ($LASTEXITCODE -ne 0) {
        throw "Compilation failed: benchmark/benchmarkChecker.cpp"
    }

    foreach ($count in $counts) {
        Write-Host ""
        Write-Host "Run: modules=$count, iterations=$Iterations"
        & ".\build\benchmarkChecker.exe" $count $Iterations
        if ($LASTEXITCODE -ne 0) {
            throw "Benchmark failed: modules=$count"
        }
    }
} finally {
    Pop-Location
}
