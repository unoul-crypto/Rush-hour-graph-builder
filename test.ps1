$ErrorActionPreference = 'Stop'
$env:PATH = "C:\raylib\w64devkit\bin;$env:PATH"
$compiler = 'C:\raylib\w64devkit\bin\g++.exe'
$output = Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Force -Path $output | Out-Null
& $compiler -std=c++17 -Wall -Wextra -pedantic -O2 `
    (Join-Path $PSScriptRoot 'tests\board_tests.cpp') (Join-Path $PSScriptRoot 'board.cpp') `
    -o (Join-Path $output 'board_tests.exe')
if ($LASTEXITCODE -ne 0) { throw 'Test compilation failed' }
& (Join-Path $output 'board_tests.exe') (Join-Path $output 'test_board.txt')
if ($LASTEXITCODE -ne 0) { throw 'Board tests failed' }
& $compiler -std=c++17 -Wall -Wextra -pedantic -O2 `
    (Join-Path $PSScriptRoot 'tests\graph_tests.cpp') (Join-Path $PSScriptRoot 'board.cpp') `
    (Join-Path $PSScriptRoot 'graph.cpp') -o (Join-Path $output 'graph_tests.exe')
if ($LASTEXITCODE -ne 0) { throw 'Graph test compilation failed' }
& (Join-Path $output 'graph_tests.exe')
if ($LASTEXITCODE -ne 0) { throw 'Graph tests failed' }
Write-Host 'Board and graph tests passed.'
