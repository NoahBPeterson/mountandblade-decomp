# startjob.ps1: start one heavy job on the shared Windows worker at Idle priority, within the concurrency cap.
#   powershell -NoProfile -ExecutionPolicy Bypass -File startjob.ps1 -Name NAME -Cmd "cmd.exe command line" [-Wait]
# Cap (checked before each start; running jobs are never preempted): 11 heavy jobs, whether or not another project's game runs.
# A "heavy job" is one this script started (pid files in D:\mb\jobs); the job's stdout/stderr go to D:\mb\jobs\NAME.log.
# The job runs under cmd /c with Idle priority in the repo root (D:\mb\repo); PYTHONHASHSEED=0, PYTHONUTF8=1.
param([Parameter(Mandatory=$true)][string]$Name, [Parameter(Mandatory=$true)][string]$Cmd, [switch]$Wait)
$jobs = "D:\mb\jobs"
New-Item -ItemType Directory -Force -Path $jobs | Out-Null
function Running-Jobs {
    $n = 0
    foreach ($f in Get-ChildItem "$jobs\*.pid" -ErrorAction SilentlyContinue) {
        $p = [int](Get-Content $f.FullName)
        if (Get-Process -Id $p -ErrorAction SilentlyContinue) { $n++ } else { Remove-Item $f.FullName }
    }
    return $n
}
while ($true) {
    $cap = 11
    if ((Running-Jobs) -lt $cap) { break }
    Start-Sleep -Seconds 10
}
$log = "$jobs\$Name.log"
# WMI starts the job outside this ssh session: sshd kills what the session started (Start-Process) when it disconnects.
# A process WMI creates does not inherit this environment, so the variables are set on its command line.
$line = "cmd.exe /c `"set PYTHONHASHSEED=0&& set PYTHONUTF8=1&& $Cmd > `"$log`" 2>&1`""
$r = Invoke-CimMethod -ClassName Win32_Process -MethodName Create -Arguments @{CommandLine=$line; CurrentDirectory="D:\mb\repo"}
if ($r.ReturnValue -ne 0) { throw "Win32_Process.Create failed: $($r.ReturnValue)" }
$p = Get-Process -Id $r.ProcessId
$p.PriorityClass = "Idle"
Set-Content -Path "$jobs\$Name.pid" -Value $r.ProcessId
"started $Name pid $($r.ProcessId) (cap $cap)"
if ($Wait) { $p.WaitForExit(); "finished" }
