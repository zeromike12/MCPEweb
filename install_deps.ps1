# Install Emscripten SDK for Minecraft Web Build
# Run this from the web-build folder before building.
# Requires: Git, Python 3

$ErrorActionPreference = "Stop"
$BuildRoot = $PSScriptRoot
# emsdk at parent (sibling of web-build), same as web-fixed – build uses ../../../emsdk
$EmsdkPath = Join-Path (Split-Path -Parent $BuildRoot) "emsdk"

if (!(Test-Path $EmsdkPath)) {
    Write-Host "Cloning emsdk into $EmsdkPath ..."
    git clone https://github.com/emscripten-core/emsdk.git $EmsdkPath
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Failed to clone emsdk."
        exit 1
    }
}

$EmsdkPs1 = Join-Path $EmsdkPath "emsdk.ps1"
if (!(Test-Path $EmsdkPs1)) {
    Write-Error "emsdk.ps1 not found at $EmsdkPs1"
    exit 1
}

Push-Location $EmsdkPath
try {
    Write-Host "Installing latest emsdk toolchain (this may take several minutes)..."
    & $EmsdkPs1 install latest
    if ($LASTEXITCODE -ne 0) {
        Write-Error "emsdk install failed."
        exit 1
    }

    Write-Host "Activating latest emsdk toolchain..."
    & $EmsdkPs1 activate latest
    if ($LASTEXITCODE -ne 0) {
        Write-Error "emsdk activate failed."
        exit 1
    }

    Write-Host ""
    Write-Host "Dependencies installed successfully." -ForegroundColor Green
    Write-Host "Run: .\project\emscripten\build_web.ps1" -ForegroundColor Cyan
}
finally {
    Pop-Location
}
