# Run the Emscripten web build from the project root
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location (Join-Path $scriptDir "project\emscripten")
& (Join-Path $scriptDir "project\emscripten\build_web.ps1")
