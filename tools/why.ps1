# Runs a Blitz3D program; if it does not exit, dumps the text of any window it
# owns, which is how the runtime reports a Blitz level error ("Error!" plus the
# message in a static control).
#
#   tools\why.ps1 <exe> [timeout-ms]
param(
  [Parameter(Mandatory = $true)][string]$Exe,
  [int]$TimeoutMs = 15000
)

$code = @'
using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

public static class WinProbe {
  [DllImport("user32.dll")] static extern bool EnumWindows(EnumProc cb, IntPtr p);
  [DllImport("user32.dll")] static extern bool EnumChildWindows(IntPtr h, EnumProc cb, IntPtr p);
  [DllImport("user32.dll")] static extern int GetWindowTextLength(IntPtr h);
  [DllImport("user32.dll", CharSet = CharSet.Unicode)] static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr h);
  delegate bool EnumProc(IntPtr h, IntPtr p);

  static string Text(IntPtr h) {
    int n = GetWindowTextLength(h);
    if (n <= 0) return null;
    var sb = new StringBuilder(n + 1);
    GetWindowText(h, sb, sb.Capacity);
    return sb.ToString();
  }

  public static string[] ForProcess(uint target) {
    var lines = new List<string>();
    EnumWindows((h, p) => {
      uint owner;
      GetWindowThreadProcessId(h, out owner);
      if (owner != target) return true;
      string title = Text(h);
      if (title == null) return true;
      lines.Add("WINDOW \"" + title + "\" visible=" + IsWindowVisible(h));
      EnumChildWindows(h, (c, q) => {
        string t = Text(c);
        if (t != null && t.Length > 0) lines.Add("   | " + t);
        return true;
      }, IntPtr.Zero);
      return true;
    }, IntPtr.Zero);
    return lines.ToArray();
  }
}
'@

Add-Type -TypeDefinition $code -Language CSharp

$p = Start-Process -FilePath $Exe -PassThru
if ( $p.WaitForExit( $TimeoutMs ) ) {
  Write-Output "exited with code $($p.ExitCode)"
  exit 0
}

Write-Output "still running after ${TimeoutMs}ms (pid $($p.Id)); windows it owns:"
$w = [WinProbe]::ForProcess( [uint32]$p.Id )
if ( $w.Count -eq 0 ) { Write-Output '  (none - the process has no window at all)' }
else { $w | ForEach-Object { Write-Output "  $_" } }
$p | Stop-Process -Force
exit 1
