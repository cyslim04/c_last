$ErrorActionPreference = "Stop"

# 无论从哪个目录调用脚本，均以脚本目录作为编译和运行工作目录。
# 后端使用相对路径 data/trust_work.db；不固定工作目录会导致服务启动后
# 找不到数据库或直接退出，前端随后只会显示 Failed to fetch。
Push-Location $PSScriptRoot
try {

$chainEnvPath = Join-Path $PSScriptRoot ".env"
if (Test-Path $chainEnvPath) {
  Get-Content $chainEnvPath | ForEach-Object {
    $entry = $_.Trim()
    if ($entry -and -not $entry.StartsWith("#") -and $entry.Contains("=")) {
      $key, $value = $entry.Split("=", 2)
      if ($key.Trim() -match '^TRUST_WORK_(RPC|CONTRACT)_(31337|11155111)$') {
        [Environment]::SetEnvironmentVariable($key.Trim(), $value.Trim(), "Process")
      }
    }
  }
}

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
} finally {
  Pop-Location
}
