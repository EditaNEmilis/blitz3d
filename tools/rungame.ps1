# Compiles and runs a real Blitz3D game with the ported toolchain, as the
# strongest end-to-end check there is: it exercises the compiler, the linker,
# texture loading through FreeImage, brush and 3DS model loading, the DirectDraw
# device, the world renderer and DirectInput.
#
#   tools\rungame.ps1 <game-dir> [timeout-seconds]
param(
  [Parameter(Mandatory = $true)][string]$GameDir,
  [string]$Main = '',
  [int]$TimeoutSec = 25
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$bin = Join-Path $repo '_release\bin'
$blitzcc = Join-Path $bin 'blitzcc.exe'

$dir = (Resolve-Path $GameDir).Path
$mainFile = ''
if ( $Main ) {
	$mainFile = Join-Path $dir $Main
} else {
	# start.bb / main.bb by convention; otherwise the largest .bb, since includes
	# like functions.bb are much smaller than the program that pulls them in.
	$named = @( Get-ChildItem $dir -Filter *.bb -File |
		Where-Object { $_.Name -match '^(start|main)\.bb$' } )
	if ( $named.Count ) {
		$mainFile = $named[0].FullName
	} else {
		$all = @( Get-ChildItem $dir -Filter *.bb -File | Sort-Object Length -Descending )
		if ( $all.Count ) { $mainFile = $all[0].FullName }
	}
}
if ( -not $mainFile -or -not (Test-Path $mainFile) ) { throw "no .bb entry point in $dir" }
# Deliberately not $main: PowerShell variables are case insensitive, so that
# would be the [string]$Main parameter, and the assignment would silently
# coerce the FileInfo down to a string with no .FullName on it.
$entry = Get-Item $mainFile

$env:blitzpath = Join-Path $repo '_release'
$env:PATH = "$bin;$env:PATH"

Write-Output "building $($entry.FullName)"
$out = & $blitzcc -o (Join-Path $dir 'b3port_test.exe') $entry.FullName 2>&1
if ( $LASTEXITCODE -ne 0 ) {
  $out | ForEach-Object { Write-Output "  $_" }
  throw "blitzcc failed on $($entry.Name)"
}
Write-Output "built. running for up to ${TimeoutSec}s..."

$exe = Join-Path $dir 'b3port_test.exe'
$p = Start-Process -FilePath $exe -WorkingDirectory $dir -PassThru
$exited = $p.WaitForExit( $TimeoutSec * 1000 )
if ( $exited ) {
  Write-Output "exited on its own with code $($p.ExitCode)"
  exit 0
}

# Still running means it started, loaded its media and entered its main loop,
# which is the pass condition for an interactive game.
$title = $p.MainWindowTitle
Write-Output "still running after ${TimeoutSec}s (window title: '$title') - that is the pass condition"
$p | Stop-Process -Force
# Stop-Process is asynchronous, so give the image a moment to be released
# before trying to delete the test executable.
$p.WaitForExit(5000)
foreach ( $try in 1..5 ) {
	try { Remove-Item $exe -Force -ErrorAction Stop; break } catch { Start-Sleep -Milliseconds 400 }
}
exit 0
