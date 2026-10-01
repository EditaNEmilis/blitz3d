# Probe-compiles Blitz3D sources with the modern MSVC toolchain and reports
# per-file error summaries, so the porting effort can be measured before and
# after each fix.
#
#   tools\probe.ps1 -Dir stdutil
#   tools\probe.ps1 -Dir compiler -Recurse
#   tools\probe.ps1 -Dir gxruntime -Define WIN32,NDEBUG,_MBCS,_LIB,PRO
param(
  [Parameter(Mandatory = $true)][string[]]$Dir,
  [switch]$Recurse,
  [string[]]$Define = @('WIN32', 'NDEBUG', '_MBCS', 'PRO'),
  [string[]]$Include = @(),
  [string[]]$ExtraFlags = @(),
  [int]$MaxErrorsPerFile = 10
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$vcvars = 'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat'
$work = Join-Path $env:TEMP ('b3probe_' + [guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Force -Path $work | Out-Null
$objdir = Join-Path $work 'obj'
New-Item -ItemType Directory -Force -Path $objdir | Out-Null

$sources = foreach ($d in $Dir) {
  $path = Join-Path $repo $d
  if (-not (Test-Path $path)) { throw "no such directory: $d" }
  if ($Recurse) { Get-ChildItem $path -Filter *.cpp -Recurse -File } else { Get-ChildItem $path -Filter *.cpp -File }
}
$sources = @($sources | Sort-Object FullName)
if ($sources.Count -eq 0) { throw "no .cpp found under $($Dir -join ',')" }

$defFlags = ($Define | ForEach-Object { "/D $_" }) -join ' '
$incFlags = ($Include | ForEach-Object { "/I `"$_`"" }) -join ' '
$flags = "/nologo /c /MT /W3 /O1 $defFlags $incFlags $($ExtraFlags -join ' ')"

# One cmd process sets up vcvarsall once, then compiles every file, so the
# toolchain probe cost is not paid per source file.
$fileList = ($sources | ForEach-Object { 'call :one "' + $_.FullName + '"' }) -join "`r`n"
$driver = @(
  '@echo off',
  "call `"$vcvars`" x86 >nul 2>&1",
  "cd /d `"$repo`"",
  $fileList,
  'exit /b 0',
  ':one',
  "  cl $flags /Fo`"$objdir\\`" %1 > `"$work\out.txt`" 2>&1",
  '  if errorlevel 1 (echo ###FILE###FAIL^|%1) else (echo ###FILE###OK^|%1)',
  '  type "%~dp0out.txt"',
  '  goto :eof'
) -join "`r`n"
Set-Content -Path (Join-Path $work 'drive.cmd') -Value $driver -Encoding ASCII

$out = & cmd /c "cd /d `"$work`" && drive.cmd" 2>&1 | Out-String
Remove-Item $work -Recurse -Force -ErrorAction SilentlyContinue

$rx = [regex]'(?ms)^###FILE###(\w+)\|(.+?)\r?\n(.*?)(?=^###FILE###|\z)'
$bad = [ordered]@{}
$ok = @()
foreach ($m in $rx.Matches($out)) {
  $status = $m.Groups[1].Value
  $name = $m.Groups[2].Value.Trim().Replace($repo + '\', '')
  $body = $m.Groups[3].Value
  if ($status -eq 'OK') {
    $ok += $name
    continue
  }
  $errs = [regex]::Matches($body, '(?m)^.*\berror [A-Z]+\d+.*$') | ForEach-Object { $_.Value.Trim() }
  $bad[$name] = @($errs)
}

foreach ($k in $bad.Keys) {
  Write-Output "[FAIL] $k"
  $bad[$k] | Select-Object -First $MaxErrorsPerFile | ForEach-Object { "         $_" }
}
Write-Output ''
Write-Output '================ SUMMARY ================'
foreach ($k in ($bad.Keys | Sort-Object { $bad[$_].Count } -Descending)) { "  {0,5}  {1}" -f $bad[$k].Count, $k }
Write-Output "  failing: $($bad.Count) / $($sources.Count)   ok: $($ok.Count)"
