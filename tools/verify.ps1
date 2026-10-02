# Golden-file correctness gate.
#
#   tools\verify.ps1                 check the tree against tools\golden\expected
#   tools\verify.ps1 -Record         (re)record the expected files
#   tools\verify.ps1 -Only collide   check one case
#
# Why this exists alongside tools\smoketest.ps1: smoketest answers "does it
# still run and still exit cleanly". That is the wrong question for most of
# the work in this plan. Rewriting float formatting, rewriting the string
# library, and replacing the Matrix/Transform scratch rings are all changes
# whose failure mode is a plausible-looking wrong number rather than a crash.
# A collision broadphase whose reject is one notch too tight still runs, still
# exits 0, and quietly sends you bouncing off the wrong entity.
#
# So each case dumps a large, deterministic, full-precision transcript and the
# whole thing has to match byte for byte. Any diff is a bug until proven
# otherwise, and -Record is the only way to accept one.
#
# The three programs are chosen so each covers a different owner:
#
#   golden_core.bb     entity/transform math - the gate for the geom.h
#                      scratch-ring change, which has no other observable
#                      effect and therefore no other way to notice.
#   golden_fmt.bb      ftoa and bbruntime/bbstring.cpp - the gate for the two
#                      cheapest and highest-yield runtime rewrites.
#   golden_collide.bb  World::collide, the MeshCollider BVH, picking and
#                      projection - the gate for the broadphase.
#
# golden_collide needs B3_ALLOW_NO_3D_DEVICE, because no Direct3D 7 device can
# be created on a current Windows and entities, transforms and collision are
# all engine-side anyway.

param(
  [switch]$Record,
  [string]$Only = '',
  [int]$TimeoutSec = 600
)

$ErrorActionPreference = 'Stop'
$ConfirmPreference = 'None'
$repo = Split-Path -Parent $PSScriptRoot
$bin = Join-Path $repo '_release\bin'
$blitzcc = Join-Path $bin 'blitzcc.exe'
$src = Join-Path $PSScriptRoot 'golden'
$expected = Join-Path $src 'expected'
$work = Join-Path $env:TEMP 'b3verify'

if ( -not (Test-Path $blitzcc) ) { throw "Build first: $blitzcc not found" }

# See tools\bench.ps1: the generated executables import fmod.dll, and the
# import loader searches PATH.
$env:blitzpath = Join-Path $repo '_release'
$env:PATH = "$bin;$env:PATH"
$env:B3_ALLOW_NO_3D_DEVICE = '1'

if ( Test-Path $work ) { Remove-Item $work -Recurse -Force }
New-Item -ItemType Directory -Force -Path $work | Out-Null

# name -> files the program is expected to produce
$cases = [ordered]@{
  'core'    = @('golden_core.bb',    @('geometry.txt'))
  'fmt'     = @('golden_fmt.bb',     @('ftoa.txt', 'strings.txt'))
  'collide' = @('golden_collide.bb', @('collide.txt'))
  '2d'      = @('golden_2d.bb',      @('2d.txt', '*.bmp'))
}
if ( $Only ) {
  foreach ( $k in @($cases.Keys) ) { if ( $k -notmatch $Only ) { $cases.Remove( $k ) } }
}

# Wipe only the files belonging to the cases about to be recorded. An earlier
# version cleared the whole directory whenever -Record was used, which meant
# `-Record -Only 2d` silently deleted geometry.txt, ftoa.txt, strings.txt and
# collide.txt - the goldens you were trying to keep.
if ( $Record ) {
  New-Item -ItemType Directory -Force -Path $expected | Out-Null
  $stale = @(foreach ( $c in $cases.Values ) { foreach ( $f in $c[1] ) {
      Get-ChildItem -Path (Join-Path $expected $f) -ErrorAction SilentlyContinue } })
  if ( $stale ) { $stale | Remove-Item -Force }
}

$fail = 0
$pass = 0

foreach ( $name in $cases.Keys ) {
  $file = $cases[$name][0]
  $outs = $cases[$name][1]
  $srcPath = Join-Path $src $file
  if ( -not (Test-Path $srcPath) ) { Write-Warning "${name}: $file missing"; continue }

  Copy-Item $srcPath $work -Force
  Push-Location $work
  $log = & $blitzcc -o "$name.exe" $file 2>&1
  if ( $LASTEXITCODE -ne 0 ) {
    Pop-Location
    Write-Warning "${name}: compile failed: $(($log | Select-Object -Last 1) -replace '\s+', ' ')"
    $fail++
    continue
  }
  Pop-Location

  foreach ( $o in $outs ) {
    # Globs have to be expanded here, or yesterday's .bmp survives into
    # today's run and a framebuffer the program no longer draws still passes.
    if ( $o -like '*.bmp' ) {
      Get-ChildItem -Path (Join-Path $work $o) -ErrorAction SilentlyContinue | Remove-Item -Force
    } else {
      $of = Join-Path $work $o
      if ( Test-Path $of ) { Remove-Item $of -Force }
    }
  }

  $p = Start-Process -FilePath (Join-Path $work "$name.exe") -WorkingDirectory $work -PassThru -WindowStyle Hidden
  # WaitForExit(int) on its own is not enough: a Blitz runtime error pops a
  # modal box that the process sits on forever, and that has to read as a
  # failure rather than as an empty transcript.
  $exited = $p.WaitForExit( $TimeoutSec * 1000 )
  if ( -not $exited -or -not $p.HasExited ) {
    $p | Stop-Process -Force
    Write-Warning "${name}: did not exit within ${TimeoutSec}s - most likely a modal runtime error box. See tools\why.ps1."
    $fail++
    continue
  }
  if ( $p.ExitCode -ne 0 ) {
    Write-Warning "${name}: exited with code $($p.ExitCode)"
    $fail++
    continue
  }

  foreach ( $o in $outs ) {
    if ( $o -like '*.bmp' ) {
      # Expand the glob and compare by hash. A BMP is the framebuffer verbatim,
      # so a single flipped bit anywhere is a single changed hash - there is no
      # summary that can talk a change through.
      $bmpFiles = @( Get-ChildItem -Path (Join-Path $work $o) -ErrorAction SilentlyContinue | Sort-Object Name )
      if ( $bmpFiles.Count -eq 0 ) { Write-Warning "${name}: produced no $o"; $fail++; continue }
      foreach ( $bf in $bmpFiles ) {
        $got = (Get-FileHash $bf.FullName -Algorithm SHA256).Hash
        $ef = Join-Path $expected $bf.Name
        if ( $Record ) {
          Copy-Item $bf.FullName $ef -Force
          Write-Host ("{0,-10} recorded  {1,6} bytes  {2}  {3}" -f $name, $bf.Length, $bf.Name, $got.Substring(0,16))
          $pass++
        } elseif ( -not (Test-Path $ef) ) {
          Write-Warning "${name}: no expected $($bf.Name). Run tools\verify.ps1 -Record once the tree is in a state you trust."
          $fail++
        } else {
          $want = (Get-FileHash $ef -Algorithm SHA256).Hash
          if ( $got -eq $want ) {
            Write-Host ("{0,-10} ok        {1,6} bytes  {2}  {3}" -f $name, $bf.Length, $bf.Name, $got.Substring(0,16))
            $pass++
          } else {
            Write-Host ("{0,-10} PIXEL DIFF  {1}  expected {2}  got {3}" -f $name, $bf.Name, $want.Substring(0,16), $got.Substring(0,16))
            $fail++
          }
        }
      }
      continue
    }

    $of = Join-Path $work $o
    if ( -not (Test-Path $of) ) { Write-Warning "${name}: $o was not produced"; $fail++; continue }
    $lines = @( Get-Content $of )
    if ( $lines.Count -eq 0 ) { Write-Warning "${name}: $o is empty"; $fail++; continue }

    if ( $Record ) {
      Copy-Item $of (Join-Path $expected $o) -Force
      Write-Host ("{0,-10} recorded  {1,6} lines  {2}" -f $name, $lines.Count, $o)
      $pass++
      continue
    }

    $ef = Join-Path $expected $o
    if ( -not (Test-Path $ef) ) {
      Write-Warning "${name}: no expected $o. Run tools\verify.ps1 -Record once the tree is in a state you trust."
      $fail++
      continue
    }

    $diff = Compare-Object (Get-Content $ef) $lines
    if ( $diff ) {
      Write-Host ("{0,-10} FAIL      {1}  ({2} differing lines)" -f $name, $o, $diff.Count)
      $diff | Select-Object -First 12 | ForEach-Object {
        $side = if ( $_.SideIndicator -eq '<=' ) { 'expected' } else { 'actual  ' }
        Write-Host ("    {0} {1}" -f $side, $_.InputObject)
      }
      if ( $diff.Count -gt 12 ) { Write-Host "    ... $($diff.Count - 12) more" }
      $fail++
    } else {
      Write-Host ("{0,-10} ok        {1,6} lines  {2}" -f $name, $lines.Count, $o)
      $pass++
    }
  }
}

Write-Host ""
if ( $fail -eq 0 ) {
  Write-Host "golden: $pass case(s) match, 0 differ"
  exit 0
}
Write-Host "golden: $pass case(s) match, $fail DIFFER"
exit 1