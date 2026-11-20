# physdrive.ps1: the in-session half of port/physics/trace/record_win.py. Runs on the Windows laptop in the logged-in
# user's desktop session (a scheduled task with /IT, created and deleted by record_win.py), like tools/relink/winsmoke.ps1.
#
# Starts <GameDir>\<Exe> (the original game with its Newton import pointed at the logging proxy) with NEWTON_TRACE set,
# then executes command files <Dir>\cmd\NNNN.txt in order, one line each, writing <Dir>\cmd\NNNN.done when done:
#   click FX FY        left click at a fraction (0..1) of the game window's client area
#   move FX FY         move the mouse there
#   key VK [MS]        press and release a virtual key (decimal), held MS milliseconds (default 60)
#   shot NAME          capture the game window to <Dir>\out\NAME.png
#   close              ask the game window to close (WM_CLOSE)
#   quit               end the game (by its PID) and this script
# Ends by itself when the game exits or after -Timeout seconds; status in <Dir>\out\status.txt, <Dir>\out\done last.
param([string]$GameDir, [string]$Exe, [string]$Dir, [string]$Trace, [int]$Timeout = 2400)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
Add-Type @"
using System; using System.Runtime.InteropServices;
public class W {
  [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int c);
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr after, int x, int y, int cx, int cy, uint fl);
  [DllImport("user32.dll")] public static extern IntPtr WindowFromPoint(POINT p);
  [DllImport("user32.dll")] public static extern IntPtr GetAncestor(IntPtr h, uint flags);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern void mouse_event(int fl, int dx, int dy, int data, IntPtr ex);
  [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte sc, int fl, IntPtr ex);
  [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint code, uint type);
  public delegate bool CB(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] static extern bool EnumWindows(CB cb, IntPtr l);
  [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr h);
  public static IntPtr Biggest(uint pid) {
    IntPtr best = IntPtr.Zero; long area = 0;
    EnumWindows(delegate(IntPtr h, IntPtr l) {
      uint p; GetWindowThreadProcessId(h, out p);
      RECT r;
      if (p == pid && IsWindowVisible(h) && GetClientRect(h, out r)) {
        long a = (long)(r.R - r.L) * (r.B - r.T);
        if (a > area) { area = a; best = h; }
      }
      return true; }, IntPtr.Zero);
    return best;
  }
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
  [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
}
"@
[W]::SetProcessDPIAware() | Out-Null
$Out = "$Dir\out"; $Cmd = "$Dir\cmd"
New-Item -ItemType Directory -Force $Out, $Cmd | Out-Null
function Status($s) { Set-Content -Path "$Out\status.txt" -Value $s }

# the game window's client rectangle on screen (x, y, w, h), else the primary screen
function Client($proc) {
  $proc.Refresh()
  $h = [W]::Biggest([uint32]$proc.Id)
  $b = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
  $res = @($b.X, $b.Y, $b.Width, $b.Height, $h)
  if ($h -ne [IntPtr]::Zero) {
    $r = New-Object W+RECT; $p = New-Object W+POINT
    if ([W]::GetClientRect($h, [ref]$r) -and ($r.R - $r.L) -gt 100 -and [W]::ClientToScreen($h, [ref]$p)) {
      $res = @($p.X, $p.Y, ($r.R - $r.L), ($r.B - $r.T), $h)
    }
  }
  return $res
}
# above everything on the user's desktop (an always-on-top Task Manager is often open there), then focused
function Front($c) {
  if ($c[4] -ne [IntPtr]::Zero) {
    [W]::SetWindowPos($c[4], [IntPtr]::new(-1), 0, 0, 0, 0, 0x43) | Out-Null
    [W]::SetForegroundWindow($c[4]) | Out-Null
    Start-Sleep -Milliseconds 150
  }
}
# true when the screen point is on a window of the game (never click another program's window)
function OnGame($proc, $x, $y) {
  $p = New-Object W+POINT; $p.X = $x; $p.Y = $y
  $h = [W]::WindowFromPoint($p)
  if ($h -eq [IntPtr]::Zero) { return $false }
  $pid2 = [uint32]0
  [W]::GetWindowThreadProcessId([W]::GetAncestor($h, 2), [ref]$pid2) | Out-Null
  return ($pid2 -eq [uint32]$proc.Id)
}
function Shot($proc, $path) {
  $c = Client $proc
  $bmp = New-Object System.Drawing.Bitmap $c[2], $c[3]
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $g.CopyFromScreen($c[0], $c[1], 0, 0, $bmp.Size)
  $g.Dispose()
  $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose()
}

$gp = $null
try {
  $t0 = Get-Date
  $env:NEWTON_TRACE = $Trace
  $gp = Start-Process -FilePath "$GameDir\$Exe" -WorkingDirectory $GameDir -PassThru
  Set-Content -Path "$Out\game.pid" -Value $gp.Id
  Status "launched"
  $next = 1
  while (((Get-Date) - $t0).TotalSeconds -lt $Timeout) {
    if ($gp.HasExited) { Status "game exited ($($gp.ExitCode))"; break }
    $f = "$Cmd\{0:D4}.txt" -f $next
    if (-not (Test-Path $f)) { Start-Sleep -Milliseconds 250; continue }
    $line = (Get-Content $f -Raw).Trim()
    $a = $line -split '\s+'
    $res = "ok"
    try {
      $c = Client $gp
      switch ($a[0]) {
        'click' {
          Front $c
          $x = [int]($c[0] + [double]$a[1] * $c[2]); $y = [int]($c[1] + [double]$a[2] * $c[3])
          if (-not (OnGame $gp $x $y)) { throw "($x,$y) is not on the game window: not clicking" }
          [W]::SetCursorPos($x, $y) | Out-Null; Start-Sleep -Milliseconds 120
          [W]::mouse_event(2, 0, 0, 0, [IntPtr]::Zero); Start-Sleep -Milliseconds 80; [W]::mouse_event(4, 0, 0, 0, [IntPtr]::Zero)
          $res = "ok $x $y"
        }
        'move' {
          $x = [int]($c[0] + [double]$a[1] * $c[2]); $y = [int]($c[1] + [double]$a[2] * $c[3])
          [W]::SetCursorPos($x, $y) | Out-Null; $res = "ok $x $y"
        }
        'key' {
          Front $c
          $vk = [byte][int]$a[1]; $ms = 60; if ($a.Length -gt 2) { $ms = [int]$a[2] }
          $sc = [byte][W]::MapVirtualKey($vk, 0)
          [W]::keybd_event($vk, $sc, 0, [IntPtr]::Zero); Start-Sleep -Milliseconds $ms; [W]::keybd_event($vk, $sc, 2, [IntPtr]::Zero)
        }
        'shot' { Front $c; Shot $gp "$Out\$($a[1]).png"; $res = "ok $($c[2])x$($c[3])" }
        'close' { $gp.CloseMainWindow() | Out-Null }
        'quit' { $res = "quit" }
        default { $res = "unknown command" }
      }
    } catch { $res = "error " + $_.Exception.Message }
    Set-Content -Path ("$Cmd\{0:D4}.done" -f $next) -Value $res
    $next++
    if ($res -eq "quit") { Status "quit"; break }
  }
  if (((Get-Date) - $t0).TotalSeconds -ge $Timeout) { Status "timeout" }
} catch {
  Status ("fail " + $_.Exception.Message)
} finally {
  if ($gp -ne $null -and -not $gp.HasExited) { Stop-Process -Id $gp.Id -Force }   # only the process this script started
  Set-Content -Path "$Out\done" -Value "1"
}
