$ErrorActionPreference = 'Stop'
$compiler = 'C:\raylib\w64devkit\bin\g++.exe'
$env:PATH = "C:\raylib\w64devkit\bin;$env:PATH"
$raylib = 'C:\raylib\raylib\src'
$output = Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Force -Path $output | Out-Null
& $compiler -std=c++17 -Wall -Wextra -pedantic -O2 -I $raylib `
    (Join-Path $PSScriptRoot 'main.cpp') (Join-Path $PSScriptRoot 'board.cpp') `
    (Join-Path $PSScriptRoot 'graph.cpp') (Join-Path $PSScriptRoot 'graph_layout.cpp') `
    (Join-Path $PSScriptRoot 'graph_view.cpp') `
    (Join-Path $raylib 'libraylib.a') -lopengl32 -lgdi32 -lwinmm `
    -o (Join-Path $output 'rush_hour_editor.exe')
if ($LASTEXITCODE -ne 0) { throw 'Compilation failed' }
Write-Host "Built $output\rush_hour_editor.exe"
