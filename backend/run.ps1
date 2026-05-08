$ErrorActionPreference = "Stop"

$sqliteBin = "C:\Users\yce\.codegeex\mamba\envs\codegeex-agent\Library\bin"
$env:PATH = "$sqliteBin;$env:PATH"

$backendPath = Join-Path $PSScriptRoot "trust_work_backend.exe"
$runningBackend = Get-Process trust_work_backend -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $backendPath }
if ($runningBackend) {
  $runningBackend | Stop-Process -Force
  Start-Sleep -Milliseconds 500
}

& "$PSScriptRoot\build.ps1"
& $backendPath
