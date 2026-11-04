# winsmoke.ps1: the in-session half of `smoke.py --host win`. Runs on the Windows laptop in the logged-in user's
# desktop session (started by a scheduled task with /IT, created and deleted by smoke.py for each run).
#
# Starts <GameDir>\mb_standalone.exe, presses Return at the launcher until the engine's log (rgl_log.txt) shows loading
# past the launcher's lines, waits for "Finished All", takes two shots of the game window 8 s and 14 s later
# (menu.png, menu_2.png in <Out>), and ends the game it started (by PID, never by name). Progress goes to
# <Out>\status.txt: "launched", "menu <seconds>" or "fail <reason>"; the file <Out>\done appears last.
param([string]$GameDir, [string]$Out, [int]$Timeout = 420)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
Add-Type @"
using System; using System.Runtime.InteropServices;
public class W {
  [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr after, int x, int y, int cx, int cy, uint fl);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint fl);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int c);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
  [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte sc, int fl, IntPtr ex);
  public delegate bool CB(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] static extern bool EnumWindows(CB cb, IntPtr l);
  [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr h);
  // the visible top-level window of the process with the largest client area (the launcher, then the game)
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
New-Item -ItemType Directory -Force $Out | Out-Null
Remove-Item "$Out\done", "$Out\status.txt", "$Out\menu*.png", "$Out\load_*.png" -ErrorAction SilentlyContinue
function Status($s) { Set-Content -Path "$Out\status.txt" -Value $s }

# the client area of the game's window, falling back to the whole primary screen
function Shot($proc, $path) {
  $proc.Refresh()
  $h = [W]::Biggest([uint32]$proc.Id)
  $b = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
  $x = $b.X; $y = $b.Y; $w = $b.Width; $hh = $b.Height
  if ($h -ne [IntPtr]::Zero) {
    $r = New-Object W+RECT
    $p = New-Object W+POINT
    if ([W]::GetClientRect($h, [ref]$r) -and ($r.R - $r.L) -gt 100 -and [W]::ClientToScreen($h, [ref]$p)) {
      $x = $p.X; $y = $p.Y; $w = $r.R - $r.L; $hh = $r.B - $r.T
    }
  }
  if ($h -ne [IntPtr]::Zero) {   # above everything else on the user's desktop (an always-on-top Task Manager covered the menu once)
    [W]::SetWindowPos($h, [IntPtr]::new(-1), 0, 0, 0, 0, 0x43) | Out-Null
    [W]::SetForegroundWindow($h) | Out-Null
    Start-Sleep -Milliseconds 400
  }
  # DWM's own copy of the window (PW_RENDERFULLCONTENT): windows the user has on top of the game (an always-on-top Task
  # Manager did) are not in it. Used when it is not blank; else the screen is copied.
  $bmp = $null
  if ($h -ne [IntPtr]::Zero) {
    $wr = New-Object W+RECT
    [W]::GetWindowRect($h, [ref]$wr) | Out-Null
  }
  # a window being created or resized can be 0 high for a moment: then the screen copy below (a 0-sized bitmap throws)
  if ($h -ne [IntPtr]::Zero -and ($wr.R - $wr.L) -gt 0 -and ($wr.B - $wr.T) -gt 0) {
    $full = New-Object System.Drawing.Bitmap ($wr.R - $wr.L), ($wr.B - $wr.T)
    $g = [System.Drawing.Graphics]::FromImage($full)
    $dc = $g.GetHdc(); [W]::PrintWindow($h, $dc, 2) | Out-Null; $g.ReleaseHdc($dc); $g.Dispose()
    $crop = New-Object System.Drawing.Rectangle ($x - $wr.L), ($y - $wr.T), $w, $hh
    $crop.Intersect((New-Object System.Drawing.Rectangle 0, 0, $full.Width, $full.Height))
    if ($crop.Width -gt 0 -and $crop.Height -gt 0) {
      $cand = $full.Clone($crop, $full.PixelFormat)
      $sum = 0; $cnt = 0
      for ($i = 0; $i -lt 40; $i++) { for ($j = 0; $j -lt 40; $j++) {
        $c = $cand.GetPixel([int]($cand.Width * ($i + 0.5) / 40), [int]($cand.Height * ($j + 0.5) / 40)); $sum += $c.R + $c.G + $c.B; $cnt++ } }
      if ($sum / $cnt -gt 6) { $bmp = $cand } else { $cand.Dispose() }
    }
    $full.Dispose()
  }
  if ($bmp -eq $null) {
    if ($w -le 0 -or $hh -le 0) { $b = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds; $x = $b.X; $y = $b.Y; $w = $b.Width; $hh = $b.Height }
    $bmp = New-Object System.Drawing.Bitmap $w, $hh
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($x, $y, 0, 0, $bmp.Size)
    $g.Dispose()
  }
  $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose()
  return (Test-Path $path)
}

$gp = $null
try {
  $log = "$GameDir\rgl_log.txt"
  $t0 = Get-Date
  $gp = Start-Process -FilePath "$GameDir\mb_standalone.exe" -WorkingDirectory $GameDir -PassThru
  Set-Content -Path "$Out\game.pid" -Value $gp.Id
  Status "launched"
  $started = $false; $menu = $false; $n = 0
  while (((Get-Date) - $t0).TotalSeconds -lt $Timeout) {
    Start-Sleep -Seconds 3
    if ($gp.HasExited) { Status "fail game exited ($($gp.ExitCode))"; return }
    $fresh = (Test-Path $log) -and ((Get-Item $log).LastWriteTime -gt $t0)
    if ($fresh -and -not $started -and (Get-Item $log).Length -gt 200) { $started = $true }
    if (-not $started) {
      # the launcher ignores a key sent right after it appears, so press Return again every few seconds
      $gp.Refresh()
      $hw = [W]::Biggest([uint32]$gp.Id)
      if ($hw -ne [IntPtr]::Zero) {
        [W]::ShowWindow($hw, 9) | Out-Null
        [W]::SetForegroundWindow($hw) | Out-Null
        Start-Sleep -Milliseconds 500
        [W]::keybd_event(13, 0, 0, [IntPtr]::Zero); [W]::keybd_event(13, 0, 2, [IntPtr]::Zero)
      }
      continue
    }
    $txt = ''
    if ($fresh) {   # the engine keeps the log open: read it with full sharing
      try {
        $fs = New-Object System.IO.FileStream($log, 'Open', 'Read', 'ReadWrite,Delete')
        $txt = (New-Object System.IO.StreamReader($fs)).ReadToEnd(); $fs.Close()
      } catch { $txt = '' }
    }
    if ($txt.Contains('Finished All')) {
      $secs = [int]((Get-Date) - $t0).TotalSeconds
      Start-Sleep -Seconds 8
      $ok1 = Shot $gp "$Out\menu.png"
      Start-Sleep -Seconds 6
      $ok2 = Shot $gp "$Out\menu_2.png"
      Status "menu $secs"
      $menu = $true
      break
    }
    if ([int]((Get-Date) - $t0).TotalSeconds / 20 -gt $n) { $n++; try { Shot $gp ("$Out\load_{0:D3}.png" -f ($n * 20)) | Out-Null } catch { } }   # a progress shot never fails the run
  }
  if (-not $menu) { Status "fail timeout" }
} catch {
  Status ("fail " + $_.Exception.Message)
} finally {
  if ($gp -ne $null -and -not $gp.HasExited) { Stop-Process -Id $gp.Id -Force }   # only the process this script started
  Set-Content -Path "$Out\done" -Value "1"
}
