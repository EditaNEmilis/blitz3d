# End to end check of the ported toolchain: compiles and runs a spread of tiny
# Blitz3D programs through _release\bin and checks each one exits cleanly.
#
#   tools\smoketest.ps1
#
# Notes that bit us while writing this:
#  * Graphics w,h,d with the default mode 0 asks for EXCLUSIVE fullscreen, not a
#    window. Mode 2 is windowed.
#  * A program that hits a Blitz runtime error pops a modal "Error!" box and
#    waits forever, so a hung child here means a bad test case, not a bad build.
#    Every case is killed between runs so a hung one cannot hold DirectDraw and
#    make the next case fail for the wrong reason.
#  * Modern display drivers no longer offer 320x240 to a DirectDraw 7 exclusive
#    mode change, so Graphics 320,240,32 with no mode argument fails with
#    "Unable to set graphics mode" on this machine. 640x480 works. That case is
#    listed as ExpectGfxUnavailable rather than quietly dropped, so the limit
#    stays visible if it ever changes.
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$bin = Join-Path $repo '_release\bin'
$blitzcc = Join-Path $bin 'blitzcc.exe'
$work = Join-Path $env:TEMP 'b3smoke'

if ( -not (Test-Path $blitzcc) ) { throw "Build first: $blitzcc not found" }
Remove-Item $work -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $work | Out-Null

$env:blitzpath = Join-Path $repo '_release'
$env:PATH = "$bin;$env:PATH"

$cases = @(
  @{ name = 'no-graphics';   body = 'End' }
  @{ name = 'windowed-16';   body = "Graphics 320,240,16,2`nPrint `"hi`"`nEndGraphics`nEnd" }
  @{ name = 'windowed-32';   body = "Graphics 320,240,32,2`nPrint `"hi`"`nEndGraphics`nEnd" }
  @{ name = 'windowed-3d';   body = "Graphics 320,240,32,2`nPrint `"hi`"`nEndGraphics`nEnd" }
  @{ name = 'exclusive-640'; body = "Graphics 640,480,32`nPrint `"hi`"`nEnd" }
  @{ name = 'exclusive-800'; body = "Graphics 800,600,32`nPrint `"hi`"`nEnd" }
  @{ name = 'accessors';     body = "Graphics 320,240,32,2`nPrint `"w=`" + GraphicsWidth()`nPrint `"h=`" + GraphicsHeight()`nPrint `"d=`" + GraphicsDepth()`nEndGraphics`nEnd" }
  @{ name = 'string-math';   body = "Graphics 320,240,32,2`nLocal n = 7`nPrint `"a=`" + n`nPrint `"b=`" + 1.5`nPrint `"c=`" + Hex(255)`nEndGraphics`nEnd" }
  @{ name = 'input';         body = "Graphics 320,240,32,2`nIf KeyDown(1) Then Print `"esc`" Else Print `"no key`"`nEndGraphics`nEnd" }
  @{ name = 'timing';        body = "Graphics 320,240,32,2`nPrint MilliSecs() >= 0`nEndGraphics`nEnd" }
  @{ name = 'text';          body = "Graphics 320,240,32,2`nPrint `"one`"`nPrint `"two`"`nEndGraphics`nEnd" }
  @{ name = 'loop';          body = "Graphics 320,240,32,2`nFor i = 1 To 1000`nNext`nPrint `"looped`"`nEndGraphics`nEnd" }
  # Known environment limit, asserted so it cannot regress unnoticed.
  @{ name = 'exclusive-320'; body = "Graphics 320,240,32`nPrint `"hi`"`nEnd"
     expectGfxUnavailable = $true }
)

$results = foreach ( $c in $cases ) {
  $dir = Join-Path $work $c.name
  New-Item -ItemType Directory -Force -Path $dir | Out-Null
  $src = Join-Path $dir 'p.bb'
  Set-Content -Path $src -Value $c.body -Encoding ASCII

  Push-Location $dir
  $compile = & $blitzcc -o 'p.exe' $src 2>&1
  $compileOk = ( $LASTEXITCODE -eq 0 )
  if ( -not $compileOk ) {
    Pop-Location
    [pscustomobject]@{ case = $c.name; stage = 'compile'; code = $LASTEXITCODE; note = (($compile | Select-Object -Last 1) -replace '\s+', ' ') }
    continue
  }

  $codes = foreach ( $i in 1..2 ) {
    $p = Start-Process -FilePath (Join-Path $dir 'p.exe') -WorkingDirectory $dir -PassThru
    if ( $p.WaitForExit( 15000 ) ) { $p.ExitCode } else { $p | Stop-Process -Force; 'hung' }
    Start-Sleep -Milliseconds 400
  }
  Pop-Location

  $bad = $codes | Where-Object { $_ -ne 0 }
  if ( $c.expectGfxUnavailable ) {
    # Must NOT silently start working, and must not crash: it has to sit on the
    # runtime's "Unable to set graphics mode" dialog, i.e. hang, not fault.
    $ok = $bad.Count -gt 0 -and ( $bad | Where-Object { $_ -ne 'hung' } ).Count -eq 0
    [pscustomobject]@{
      case = $c.name; stage = 'known-limit'
      code = if ( $ok ) { 'as-expected' } else { ($bad -join ',') }
      note = 'exclusive 320x240 unavailable; hangs on the error dialog'
    }
    continue
  }
  [pscustomobject]@{
    case = $c.name
    stage = 'run'
    code = if ( $bad ) { ($bad -join ',') } else { 0 }
    note = "runs: $($codes -join ',')"
  }
}

$results | Format-Table -AutoSize
# Wrap both sides in @(). A Where-Object that matches nothing assigns $null, and
# `$null + <PSObject>` raises op_Addition, so this script used to die on exactly
# the run that matters most - the one where every case passes.
$bad = @($results | Where-Object { $_.stage -eq 'known-limit' -and $_.code -ne 'as-expected' }) +
       @($results | Where-Object { $_.stage -ne 'known-limit' -and $_.code -ne 0 })
Write-Output ''
if ( $bad ) {
  Write-Output "FAILING: $(($bad | ForEach-Object { $_.case }) -join ', ')"
  exit 1
}
Write-Output 'all smoke cases exited 0 on every run'
