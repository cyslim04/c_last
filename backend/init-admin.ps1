$ErrorActionPreference = "Stop"

$sqliteBin = "C:\Users\yce\.codegeex\mamba\envs\codegeex-agent\Library\bin"
$env:PATH = "$sqliteBin;$env:PATH"

& "$PSScriptRoot\build.ps1"

$username = Read-Host "管理员账号"
$securePassword = Read-Host "管理员密码" -AsSecureString
$password = [System.Net.NetworkCredential]::new("", $securePassword).Password

if ([string]::IsNullOrWhiteSpace($username) -or [string]::IsNullOrWhiteSpace($password)) {
  throw "管理员账号和密码不能为空。"
}

$env:TRUST_WORK_ADMIN_USERNAME = $username
$env:TRUST_WORK_ADMIN_PASSWORD = $password
& "$PSScriptRoot\trust_work_backend.exe" --init-admin
if ($LASTEXITCODE -ne 0) {
  throw "管理员初始化失败。"
}
