param([switch]$PackageOnly)
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$build = Join-Path $root 'build'
New-Item -ItemType Directory -Force -Path $build | Out-Null
if (!$PackageOnly) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    $vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (!$vs) { throw 'Visual Studio C++ tools are required (including ClangCL and a Windows SDK).' }
    $driver = @"
@echo off
call "$vs\VC\Auxiliary\Build\vcvarsall.bat" x86
if errorlevel 1 exit /b 1
cd /d "$root\thprac"
cl /Isrc\3rdParties\yyjson /nologo /EHsc /O2 /std:c++20 /utf-8 loc_json.cpp src\3rdParties\yyjson\yyjson.c /Fe:loc_json.exe
if errorlevel 1 exit /b 1
"$root\thprac\loc_json.exe" src\thprac
if errorlevel 1 exit /b 1
cd /d "$root"
msbuild thprac.sln /m:1 /p:Configuration=ReleaseLLVM /p:OutDir="$build\win32\\" /verbosity:minimal
if errorlevel 1 exit /b 1
"@
    $driverPath = Join-Path $build 'build-driver.cmd'
    [IO.File]::WriteAllText($driverPath,$driver,[Text.Encoding]::Default)
    & $env:ComSpec /d /c $driverPath
    if ($LASTEXITCODE) { throw 'The existing x86 launcher build failed.' }
    & cmake -S $root -B "$build\x64" -A x64
    if ($LASTEXITCODE) { throw 'CMake configuration failed.' }
    & "$vs\MSBuild\Current\Bin\MSBuild.exe" "$build\x64\ALL_BUILD.vcxproj" /p:Configuration=Release /p:Platform=x64 /m:1 /v:minimal
    if ($LASTEXITCODE) { throw 'The x64 module build failed.' }
}
$package = Join-Path $build 'thprac-nc'
New-Item -ItemType Directory -Force -Path $package | Out-Null
Copy-Item -LiteralPath "$build\win32\thprac.exe" -Destination $package
foreach ($name in 'thprac_bridge64.exe','thprac_th06nc.dll','freetype.dll') {
    Copy-Item -LiteralPath "$build\x64\bin\Release\$name" -Destination $package
}
foreach ($name in 'README_NC.md','VALIDATION_NC.md','LICENCE') {
    Copy-Item -LiteralPath (Join-Path $root $name) -Destination $package
}
$licenses=Join-Path $package 'licenses'
New-Item -ItemType Directory -Force -Path $licenses | Out-Null
Copy-Item -LiteralPath "$root\thprac\src\3rdParties\FreeType\LICENSE.TXT" -Destination "$licenses\FreeType-LICENSE.txt"
Copy-Item -LiteralPath "$root\thprac\src\3rdParties\FreeType\FTL.TXT" -Destination "$licenses\FreeType-FTL.txt"
Copy-Item -LiteralPath "$root\thprac\src\3rdParties\MinHook\LICENSE.txt" -Destination "$licenses\MinHook.txt"
Copy-Item -LiteralPath "$root\thprac\src\3rdParties\ImGui\LICENSE.txt" -Destination "$licenses\ImGui.txt"
Get-ChildItem -LiteralPath $package -File | Where-Object Name -ne 'SHA256.json' | Get-FileHash -Algorithm SHA256 | Select-Object Hash,@{n='File';e={Split-Path $_.Path -Leaf}} | ConvertTo-Json | Set-Content -Encoding UTF8 "$package\SHA256.json"
Write-Host "Built: $package\thprac.exe"
