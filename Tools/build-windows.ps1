$ErrorActionPreference = "Stop"

$vswhere = "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path -LiteralPath $vswhere)) {
    throw "Visual Studio Build Tools with Desktop development with C++ is required."
}
$visualStudio = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $visualStudio) { throw "The Visual Studio C++ x64 toolchain was not found." }

$developerCommand = Join-Path $visualStudio "Common7\Tools\VsDevCmd.bat"
$projectRoot = Split-Path -Parent $PSScriptRoot
$workspace = Split-Path -Parent $projectRoot
$juceSource = Join-Path $workspace "LJuno-116\build\nmake-release\_deps\juce-src"
$buildDirectory = Join-Path $projectRoot "build\nmake-release"
$sourceOverride = if (Test-Path -LiteralPath (Join-Path $juceSource "CMakeLists.txt")) {
    ' -DFETCHCONTENT_SOURCE_DIR_JUCE="{0}"' -f $juceSource
} else { "" }
$commandLine = '"{0}" -arch=x64 && cmake -S "{1}" -B "{2}" -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release{3} && cmake --build "{2}" && ctest --test-dir "{2}" --output-on-failure' -f $developerCommand, $projectRoot, $buildDirectory, $sourceOverride

& cmd.exe /d /s /c $commandLine
if ($LASTEXITCODE -ne 0) { throw "LR-608 build failed with exit code $LASTEXITCODE." }
Write-Host "Build completed: $buildDirectory\LR608_artefacts\Release\VST3\LR-608.vst3"
