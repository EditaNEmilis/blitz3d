# Benchmarks the Blitz3D fork and compares against a recorded baseline.
#
#   tools\bench.ps1                      run everything, write tools\bench-current.txt
#   tools\bench.ps1 -Baseline <file>     compare against a recorded baseline
#   tools\bench.ps1 -SaveBaseline <file> record the current run as the baseline
#   tools\bench.ps1 -Only bb_exec,scene  run a subset
#   tools\bench.ps1 -Runs 3              run each case N times, report the best
#
# What each case is measuring is documented at the top of the matching .bb in
# tools\bench\. The short version:
#
#   bb_exec   generated-code and BASIC runtime cost, no graphics
#   canvas2d  gxCanvas 2D primitives through DirectDraw
#   scene     World::update, the collision broadphase, animation, picking
#   render3d  the D3D7 submission path; only run when probe3d says yes
#
# Methodology, and why it is what it is
# ------------------------------------
# Every .bb case runs its workload several times and reports the MINIMUM
# internally. That is not a stylistic choice. Blitz3D's register allocator
# emits no loop alignment, so the measured cost of a small hot loop depends
# on the address it happens to land at: adding one unused function ahead of a
# benchmark moved a 1M-iteration 8-argument call loop from 8ms to 156ms on
# this machine, same source, same runtime. A median or a mean of a few runs
# measures code layout as much as it measures the code. The minimum is stable
# to a few percent.
#
# The consequence for this plan is worth stating plainly: when the change
# under test alters generated code layout, microbenchmarks can move by an
# order of magnitude for reasons unrelated to the optimisation. Those cases
# are marked layout-sensitive below, and any result for them has to be
# confirmed against a whole-program frame time.

param(
  [string]$Baseline = '',
  [string]$SaveBaseline = '',
  [string]$Only = '',
  [int]$Runs = 1,
  [switch]$Quiet
)

$ErrorActionPreference = 'Stop'
# This script deletes its own scratch files, and it has to run unattended from
# CI as well as from a shell. Without this, Remove-Item on a missing path can
# raise a confirmation prompt, which fails outright under -NonInteractive.
$ConfirmPreference = 'None'
# Baseline files are compared with -f, so a decimal comma under a European
# locale would make every ratio read as garbage.
$Invariant = [System.Globalization.CultureInfo]::InvariantCulture
[System.Threading.Thread]::CurrentThread.CurrentCulture = $Invariant
$repo = Split-Path -Parent $PSScriptRoot
$bin = Join-Path $repo '_release\bin'
$blitzcc = Join-Path $bin 'blitzcc.exe'
$benchDir = Join-Path $PSScriptRoot 'bench'
$work = Join-Path $env:TEMP 'b3bench'

if ( -not (Test-Path $blitzcc) ) { throw "Build first: $blitzcc not found" }

# The generated executables import fmod.dll from the DLL search path, and the
# import loader does search PATH, so _release\bin has to be on it. tools\
# smoketest.ps1 does the same thing and for the same reason.
$env:blitzpath = Join-Path $repo '_release'
$env:PATH = "$bin;$env:PATH"
# blitz3d_open() creates the engine-side World before it asks for a 3D device,
# and this is what lets it keep it when none exists. Without it, Graphics3D
# raises "Unable to create 3D Scene" on any Windows since 8 dropped the D3D7
# HAL, and there is no way to exercise entities, collision or animation at all.
$env:B3_ALLOW_NO_3D_DEVICE = '1'

if ( Test-Path $work ) { Remove-Item $work -Recurse -Force }
New-Item -ItemType Directory -Force -Path $work | Out-Null

# A 32x32 masked bitmap for the 2D cases. Written here rather than checked in
# so there is no binary in the tree.
$img = Join-Path $work 'tile.bmp'
if ( -not (Test-Path $img) ) {
  Add-Type -AssemblyName System.Drawing
  $bmp = New-Object System.Drawing.Bitmap 32, 32
  for ( $y = 0; $y -lt 32; $y += 8 ) {
    for ( $x = 0; $x -lt 32; $x += 8 ) {
      if ( (($x / 8) + ($y / 8)) % 2 -eq 0 ) { $bmp.SetPixel( $x, $y, [System.Drawing.Color]::Lime ) }
    }
  }
  $bmp.Save( $img, [System.Drawing.Imaging.ImageFormat]::Bmp )
  $bmp.Dispose()
}

# Cases run in this order, because scene and canvas2d each open a graphics mode
# and DirectDraw will not give it to two processes at once.
$order = @('bb_exec', 'scene', 'canvas2d')
if ( $Only ) { $order = @($Only) }

$results = [ordered]@{}
$notes = @()

foreach ( $name in $order ) {
  $src = Join-Path $benchDir "$name.bb"
  if ( -not (Test-Path $src) ) { continue }

  if ( $name -eq 'probe3d' ) { continue }   # handled separately below

  Copy-Item $src $work -Force
  Push-Location $work
  $out = & $blitzcc -o "$name.exe" "$name.bb" 2>&1
  if ( $LASTEXITCODE -ne 0 ) {
    Pop-Location
    $notes += "${name}: compile failed: $(($out | Select-Object -Last 1) -replace '\s+', ' ')"
    continue
  }
  Pop-Location

  $exe = Join-Path $work "$name.exe"
  # canvas2d and scene write a fixed results file name and take no arguments.
  # bb_exec takes the results file name from the command line.
  $argList = switch ( $name ) {
    'canvas2d' { '' }
    'scene'    { '7' }
    default    { "$name.txt" }
  }

  $best = @{}
  for ( $run = 1; $run -le $Runs; $run++ ) {
    $rf = Join-Path $work "$name.txt"
    if ( Test-Path $rf ) { Remove-Item $rf -Force }
    Push-Location $work
    $p = if ( $argList ) {
      Start-Process -FilePath $exe -ArgumentList $argList -WorkingDirectory $work -PassThru -WindowStyle Hidden
    } else {
      Start-Process -FilePath $exe -WorkingDirectory $work -PassThru -WindowStyle Hidden
    }
    if ( -not $p.WaitForExit( 120000 ) ) {
      $p | Stop-Process -Force
      Pop-Location
      $notes += "${name}: hung on run ${run} (a modal runtime error box, most likely Graphics3D)"
      break
    }
    Pop-Location
    if ( -not (Test-Path $rf) ) { $notes += "${name}: produced no results file on run ${run}"; break }
    $lines = @( Get-Content $rf )
    if ( $lines.Count -eq 0 ) { $notes += "${name}: results file was empty on run ${run}"; break }
    foreach ( $line in $lines ) {
      $f = $line -split "`t"
      if ( $f.Count -lt 4 ) { continue }
      $k = $f[0]
      $ms = [double]$f[3]
      if ( -not $best.ContainsKey($k) -or $ms -lt $best[$k] ) { $best[$k] = $ms }
    }
  }
  foreach ( $k in $best.Keys ) { $results["$name/$k"] = $best[$k] }
}

# Compile time, measured on the largest bundled programs. Separate from the
# in-program cases because it is the blitzcc process itself that is being
# timed, not a runtime benchmark.
$compileTargets = @(
  @{ name = 'compile/tunnelrun';  dir = Join-Path $repo '_release\Games\TunnelRun' },
  @{ name = 'compile/asteroids';  dir = Join-Path $repo '_release\Games\bb3d_asteroids' },
  @{ name = 'compile/wingring';   dir = Join-Path $repo '_release\Games\wing_ring' }
)
foreach ( $t in $compileTargets ) {
  if ( -not (Test-Path $t.dir ) ) { continue }
  $entry = Get-ChildItem $t.dir -Filter *.bb -File |
    Where-Object { $_.Name -match '^(start|main)\.bb$' } | Select-Object -First 1
  if ( -not $entry ) { $entry = Get-ChildItem $t.dir -Filter *.bb -File | Sort-Object Length -Descending | Select-Object -First 1 }
  if ( -not $entry ) { continue }
  $times = @()
  foreach ( $k in 1..5 ) {
    Push-Location $t.dir
    $sw = [Diagnostics.Stopwatch]::StartNew()
    & $blitzcc -c $entry.FullName *> $null
    $sw.Stop()
    Pop-Location
    $times += $sw.Elapsed.TotalMilliseconds
  }
  $sorted = $times | Sort-Object
  $results[$t.name] = $sorted[2]   # median of 5
}

# --- probe3d ---------------------------------------------------------------
# Does a Direct3D 7 device exist here at all? gxScene::clear/render/end
# dereference dir3dDev with no null check, so a "no" answer means the 3D
# submission benchmarks are not runnable on this machine and the Tier 2 3D
# items have to be justified by reasoning rather than by measurement.
$has3D = $false
$src = Join-Path $benchDir 'probe3d.bb'
if ( ( -not $Only -or $Only -match 'probe3d' ) -and (Test-Path $src) ) {
  Copy-Item $src $work -Force
  Push-Location $work
  & $blitzcc -o 'probe3d.exe' 'probe3d.bb' *> $null
  Pop-Location
  if ( $LASTEXITCODE -eq 0 ) {
    $rf3d = Join-Path $work 'probe3d.txt'
    if ( Test-Path $rf3d ) { Remove-Item $rf3d -Force }
    Push-Location $work
    $p = Start-Process -FilePath (Join-Path $work 'probe3d.exe') -WorkingDirectory $work -PassThru -WindowStyle Hidden
    if ( $p.WaitForExit( 20000 ) ) { $has3D = (Test-Path (Join-Path $work 'probe3d.txt')) }
    else { $p | Stop-Process -Force }
    Pop-Location
  }
}
if ( -not $Quiet ) {
  Write-Host ""
  Write-Host ("Direct3D 7 device available: " + $(if ($has3D) { 'YES' } else { 'NO - 3D submission benchmarks skipped' }))
}

# --- report ----------------------------------------------------------------
Write-Host ""
Write-Host ("{0,-30} {1,14} {2,12} {3,10}" -f 'case', 'ms', 'ops/sec', 'vs base')
Write-Host ('-' * 68)

$prev = $null
if ( $Baseline -and (Test-Path $Baseline) ) {
  $prev = @{}
  foreach ( $line in Get-Content $Baseline ) {
    # Thousands separators have to come out before the number is parsed, or
    # "1,272.000" reads as 1 and every ratio comes out as -99.9%.
    if ( $line -match '^\s*(\S+)\s+([\d,.]+)' ) { $prev[$Matches[1]] = [double]($Matches[2] -replace ',', '') }
  }
}

$lines = @()
foreach ( $k in $results.Keys ) {
  $ms = $results[$k]
  $ops = if ( $ms -gt 0 ) { 1 / $ms } else { 0 }
  $cmp = ''
  if ( $prev -and $prev.ContainsKey($k) -and $prev[$k] -gt 0 ) {
    $ratio = $prev[$k] / $ms
    $cmp = ('{0,9:P1}' -f ($ratio - 1))
  }
  Write-Host ("{0,-30} {1} {2} {3,10}" -f $k, ([string]::Format($Invariant,'{0:0.##}',$ms)), ([string]::Format($Invariant,'{0:0.##}',$ops)), $cmp)
  $lines += ("{0,-30} {1}" -f $k, ([string]::Format( $Invariant, '{0:0.###}', $ms )))
}

$header = @(
  "# Blitz3D fork benchmark baseline",
  "# generated by tools\bench.ps1",
  "# recorded: $((Get-Date).ToString('yyyy-MM-dd HH:mm'))",
  "# machine: $env:COMPUTERNAME  cpu: $((Get-CimInstance Win32_Processor).Name)",
  "# direct3d7_available: $has3D",
  "# The baseline is the pre-optimisation code, re-recorded whenever
  # the machine's background load changes enough to move the minimums:
  # the cases are min-of-N and a busier machine raises the floor for
  # every case, which would read as a regression in code that is
  # measurably faster than the code the old baseline came from.",
  "# layout-sensitive cases (microbenchmarks over generated code):",
  "#   bb_exec/*, scene/*  - codegen changes can move these by an order of",
  "#   magnitude from code layout alone, not from the optimisation itself",
  ""
)
$text = $header + $lines
$outFile = Join-Path $PSScriptRoot 'bench-current.txt'
Set-Content -Path $outFile -Value $text -Encoding ASCII

if ( $SaveBaseline ) {
  Set-Content -Path $SaveBaseline -Value $text -Encoding ASCII
  Write-Host ""
  Write-Host "baseline written to $SaveBaseline"
}

foreach ( $n in $notes ) { Write-Warning $n }
Write-Host ""
Write-Host "wrote $outFile"