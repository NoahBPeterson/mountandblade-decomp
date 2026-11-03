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
$env:PYTHONHASHSEED = "0"; $env:PYTHONUTF8 = "1"
$log = "$jobs\$Name.log"
$p = Start-Process -FilePath "cmd.exe" -ArgumentList "/c $Cmd > `"$log`" 2>&1" -WorkingDirectory "D:\mb\repo" -WindowStyle Hidden -PassThru
$p.PriorityClass = "Idle"
Set-Content -Path "$jobs\$Name.pid" -Value $p.Id
"started $Name pid $($p.Id) (cap $cap)"
if ($Wait) { $p.WaitForExit(); "exit $($p.ExitCode)" }
