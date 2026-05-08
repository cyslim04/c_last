$ErrorActionPreference = "Stop"

$sqliteRoot = "C:\Users\yce\.codegeex\mamba\envs\codegeex-agent\Library"
$gccArgs = @(
  "-Wall",
  "-Wextra",
  "-std=c11",
  "-Iinclude",
  "-I$($sqliteRoot)\include",
  "-o",
  "trust_work_backend.exe",
  "src/server.c",
  "src/utils/sha256.c",
  "-L$($sqliteRoot)\lib",
  "-lsqlite3",
  "-lws2_32"
)

& gcc @gccArgs
if ($LASTEXITCODE -ne 0) {
  throw "Backend build failed."
}
Write-Host "Build completed: backend\trust_work_backend.exe"
