param(
  [string] $OutputPath = "trust_work_backend.exe"
)

$ErrorActionPreference = "Stop"

$sqliteRoot = "C:\Users\yce\.codegeex\mamba\envs\codegeex-agent\Library"
$gccArgs = @(
  "-Wall",
  "-Wextra",
  "-std=c11",
  "-Iinclude",
  "-I$($sqliteRoot)\include",
  "-o",
  $OutputPath,
  "src/server.c",
  "src/utils/sha256.c",
  "src/utils/chain_rpc.c",
  "-L$($sqliteRoot)\lib",
  "-lsqlite3",
  "-lws2_32",
  "-lwinhttp"
)

& gcc @gccArgs
if ($LASTEXITCODE -ne 0) {
  throw "Backend build failed."
}
Write-Host "Build completed: $OutputPath"
