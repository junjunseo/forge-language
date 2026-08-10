$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$compiler = (Get-Command g++ -ErrorAction Stop).Source
$version = (Get-Content -LiteralPath (Join-Path $root "VERSION") -Raw).Trim()

if ($version -notmatch '^\d+\.\d+\.\d+$') {
    throw "VERSION must contain a semantic version such as 0.1.0"
}

$versionDefinition = '-DIEUM_VERSION=\"' + $version + '\"'

Push-Location $root
try {
    New-Item -ItemType Directory -Force -Path "build" | Out-Null

    $commonArgs = @(
        "-std=c++17",
        "-Wall",
        "-Wextra",
        "-pedantic",
        "-Isrc",
        $versionDefinition
    )

    $targets = @(
        @{ Source = "test/testParser.cpp";   Output = "build/testParser.exe" },
        @{ Source = "test/testPipeline.cpp"; Output = "build/testPipeline.exe" },
        @{ Source = "test/testChecker.cpp";  Output = "build/testChecker.exe" }
    )

    foreach ($target in $targets) {
        Write-Host "Build: $($target.Source)"
        & $compiler @commonArgs $target.Source -o $target.Output
        if ($LASTEXITCODE -ne 0) {
            throw "Compilation failed: $($target.Source)"
        }

        Write-Host "Run: $($target.Output)"
        & ".\$($target.Output)"
        if ($LASTEXITCODE -ne 0) {
            throw "Test failed: $($target.Output)"
        }
        Write-Host ""
    }

    Write-Host "Build: src/main.cpp"
    & $compiler @commonArgs "src/main.cpp" -o "build/ieum.exe"
    if ($LASTEXITCODE -ne 0) {
        throw "Compilation failed: src/main.cpp"
    }

    Write-Host "Run: build/ieum.exe --version"
    $versionOutput = & ".\build\ieum.exe" "--version" 2>&1
    $versionExitCode = $LASTEXITCODE
    $versionText = ($versionOutput -join "`n").Trim()
    Write-Host $versionText
    if ($versionExitCode -ne 0) {
        throw "Expected --version to exit with 0, got $versionExitCode"
    }
    if ($versionText -ne "ieum $version") {
        throw "Expected version output 'ieum $version', got '$versionText'"
    }
    Write-Host ""

    Write-Host "Run: build/ieum.exe examples/valid.ieum"
    $validOutput = & ".\build\ieum.exe" ".\examples\valid.ieum" 2>&1
    $validExitCode = $LASTEXITCODE
    $validText = $validOutput -join "`n"
    Write-Host $validText
    if ($validExitCode -ne 0) {
        throw "Expected valid example to pass"
    }
    Write-Host ""

    $invalidExamples = @(
        @{ Name = "implicit_dependency"; Expected = "notification" },
        @{ Name = "cyclic_dependency"; Expected = "order -> payment -> order" },
        @{ Name = "layer_violation"; Expected = "'data'" },
        @{ Name = "transitive_layer_violation"; Expected = "'data'" },
        @{ Name = "invalid_declarations"; Expected = "'missing'" }
    )

    foreach ($example in $invalidExamples) {
        $path = ".\examples\$($example.Name).ieum"
        Write-Host "Run: build/ieum.exe $path"
        $exampleOutput = & ".\build\ieum.exe" $path 2>&1
        $exampleExitCode = $LASTEXITCODE
        $exampleText = $exampleOutput -join "`n"
        Write-Host $exampleText
        if ($exampleExitCode -eq 0) {
            throw "Expected structural violation for $path"
        }
        if ($exampleExitCode -ne 1) {
            throw "Expected exit code 1 for $path, got $exampleExitCode"
        }
        if (-not $exampleText.Contains($example.Expected)) {
            throw "Expected output '$($example.Expected)' for $path"
        }
        Write-Host ""
    }

    Write-Host "All tests passed."
} finally {
    Pop-Location
}
