# start tools/remote/angr_server.py on the laptop, detached from the ssh session (WMI), at Idle priority.
# Its PID goes to D:\mb\eq\server.pid (not D:\mb\jobs: it is not one of startjob.ps1's heavy jobs; its solver
# children are, and they run at Idle priority too). Stop: taskkill /pid <that PID> /t /f
$old = "D:\mb\eq\server.pid"
if (Test-Path $old) {
    $p = [int](Get-Content $old)
    if (Get-Process -Id $p -ErrorAction SilentlyContinue) { "already running: pid $p"; exit 0 }
}
$cmd = 'cmd.exe /c "D:\mb\repo\.venv\Scripts\python D:\mb\eq\angr_server.py --bind 100.84.111.92 --port 47815 --slots 6 > D:\mb\eq\server.log 2>&1"'
$r = Invoke-CimMethod -ClassName Win32_Process -MethodName Create -Arguments @{CommandLine=$cmd; CurrentDirectory='D:\mb\eq'}
(Get-Process -Id $r.ProcessId).PriorityClass = 'Idle'
Set-Content $old $r.ProcessId
"started pid $($r.ProcessId)"
