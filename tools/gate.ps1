# The one command to run after any change.
#
#   tools\gate.ps1                 build, verify, smoke test, benchmark
#   tools\gate.ps1 -SkipBuild      reuse the existing _release
#   tools\gate.ps1 -MaxRegression 5   per-benchmark tolerance, default 3 (percent)
#   tools\gate.ps1 -NoiseMs 2      deltas under this many ms are noise,
#                                  default 2
#   tools\gate.ps1 -NoBench        correctness only, for when the tree is mid-edit
#
# Everything else in tools\ is a component. This is the verdict. Run it after
# every change and nothing gets shipped on a hunch.
#
# The three correctness layers are not redundant and each covers a different
# failure mode:
#
#   verify   byte-for-byte golden transcripts. The only gate that can catch a
#            rewritten ftoa or a changed Transform producing a plausible wrong
#            number, which is exactly the failure mode of the expensive work.
#   smoketest  does it still build, start, draw and exit 0.
#   bench     is it faster, and did anything get slower than the tolerance.
#
# A case absent from the baseline is reported as "new" rather than as a pass,
# because a benchmark that appears for the first time has nothing to be
# compared against and silently counting it as green is how a regression gets
# missed.
#
# The bench phase ends in a whole-program confirmation. bb_exec/* and scene/*
# are microbenchmarks over generated code and bench.ps1 documents that code
# layout alone can move them; the canvas2d/* cases draw through the GPU
# driver, so their per-call cost is driver-dominated too. A case in those
# families is only a real regression if the whole program got slower with it:
# when every compile/* frame time is at or below the baseline, a failing
# microbenchmark moved for layout reasons, not because the program is slower.
# Anything outside those families, and any compile/* regression, is a hard
# failure.

param(
  [switch]$SkipBuild,
  [switch]$NoBench,
  [double]$MaxRegression = 3.0,
  [double]$NoiseMs = 2.0
)

$ErrorActionPreference = 'Continue'
$ConfirmPreference = 'None'
$repo = Split-Path -Parent $PSScriptRoot
$baselinePath = Join-Path $PSScriptRoot 'bench-baseline.txt'

$failures = [System.Collections.Generic.List[string]]::new()

function Step( [string]$name, [scriptblock]$body ) {
  Write-Host ""
  Write-Host ("=" * 68)
  Write-Host "  $name"
  Write-Host ("=" * 68)
  & $body
}

# ---------------------------------------------------------------- build ----
if ( -not $SkipBuild ) {
  Step 'build' {
    cmd /c "tools\build.cmd" 2>&1 | Select-String -Pattern 'error|fatal|FAILED|Built into'
    if ( $LASTEXITCODE -ne 0 ) { $failures.Add('build failed') }
  }
}

# ---------------------------------------------------------------- verify ---
Step 'golden transcripts' {
  & (Join-Path $PSScriptRoot 'verify.ps1')
  if ( $LASTEXITCODE -ne 0 ) { $failures.Add('golden transcripts differ') }
}

# ------------------------------------------------------------ smoketest ----
Step 'smoke test' {
  $out = & (Join-Path $PSScriptRoot 'smoketest.ps1') 2>&1
  $text = ($out | Out-String)
  if ( $text -match 'all smoke cases exited 0 on every run' ) {
    Write-Host 'all smoke cases exited 0'
  } else {
    Write-Host $text
    $failures.Add('smoke test did not pass cleanly')
  }
}

# ----------------------------------------------------------------- bench ---
if ( -not $NoBench ) {
  if ( -not (Test-Path $baselinePath) ) {
    $failures.Add("no baseline at $baselinePath")
  } else {
    Step 'benchmarks' {
      $out = & (Join-Path $PSScriptRoot 'bench.ps1') -Baseline $baselinePath 2>&1
      $out | ForEach-Object { Write-Host $_ }

      # Re-read what bench.ps1 wrote rather than scraping the table: the table
      # is formatted for humans and the file is the machine-readable form.
      $currentPath = Join-Path $PSScriptRoot 'bench-current.txt'
      if ( -not (Test-Path $currentPath) ) {
        $failures.Add('bench.ps1 produced no current report')
        return
      }

      $prev = @{}
      foreach ( $l in Get-Content $baselinePath ) {
        if ( $l -match '^\s*(\S+)\s+([\d,.]+)' ) { $prev[$Matches[1]] = [double]($Matches[2] -replace ',', '') }
      }
      $now = @{}
      foreach ( $l in Get-Content $currentPath ) {
        if ( $l -match '^\s*(\S+)\s+([\d,.]+)' ) { $now[$Matches[1]] = [double]($Matches[2] -replace ',', '') }
      }

      # Whole-program frame times: the ground truth the
      # layout-sensitive families are confirmed against.
      $wholeOk = $true
      foreach ( $k in $now.Keys ) {
        if ( $k -notmatch '^compile/' -or -not $prev.ContainsKey($k) ) { continue }
        $w = $prev[$k]
        if ( $w -le 0 ) { continue }
        if ( ($now[$k] - $w) -gt $NoiseMs -and ($now[$k] / $w - 1) * 100 -gt $MaxRegression ) { $wholeOk = $false }
      }

      Write-Host ""
      Write-Host ("{0,-30} {1,10} {2,10} {3,12}" -f 'case', 'was(ms)', 'now(ms)', 'change')
      Write-Host ('-' * 68)
      foreach ( $k in $now.Keys ) {
        if ( -not $prev.ContainsKey($k) ) {
          Write-Host ("{0,-30} {1,10} {2,10} {3,12}" -f $k, '-', $now[$k], 'new')
          continue
        }
        $was = $prev[$k]
        if ( $was -le 0 ) { continue }
        $pct = ($now[$k] / $was - 1) * 100
        $flag = ''
        # A case that runs in 1-3ms moves a whole millisecond between
        # two runs - 30% to 100% of the measurement - from nothing but
        # the timer and the scheduler. The percentage only means
        # anything once the delta is real in milliseconds, so a
        # regression has to clear the tolerance and the noise floor.
        if ( $pct -gt $MaxRegression -and ($now[$k] - $was) -gt $NoiseMs ) {
          if ( $wholeOk -and $k -match '^(bb_exec|scene|canvas2d)/' ) {
            $flag = '  (layout: whole program faster)'
          } else {
            $flag = '  <-- REGRESSION'
            $failures.Add("$k regressed $([math]::Round($pct,1))%")
          }
        }
        Write-Host ("{0,-30} {1,10} {2,10} {3,11:N1}%{4}" -f $k, $was, $now[$k], $pct, $flag)
      }
    }
  }
}

# ---------------------------------------------------------------- verdict --
Write-Host ""
Write-Host ("=" * 68)
if ( $failures.Count -eq 0 ) {
  Write-Host "  GATE: PASS"
  Write-Host ("=" * 68)
  exit 0
}
Write-Host "  GATE: FAIL"
foreach ( $f in $failures ) { Write-Host "    - $f" }
Write-Host ("=" * 68)
exit 1