$ErrorActionPreference = "Stop"

$variableName = "TRUST_WORK_ENABLE_DEMO_QUICK_LOGIN"
$previousValue = [Environment]::GetEnvironmentVariable($variableName, "Process")
[Environment]::SetEnvironmentVariable($variableName, "1", "Process")

try {
  & (Join-Path $PSScriptRoot "run.ps1")
} finally {
  [Environment]::SetEnvironmentVariable($variableName, $previousValue, "Process")
}
