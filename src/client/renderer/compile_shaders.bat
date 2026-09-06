@echo off
setlocal enabledelayedexpansion

:: Check if Vulkan SDK is installed
if "%VULKAN_SDK%"=="" (
    echo VULKAN_SDK environment variable is not set. Please install the Vulkan SDK.
    exit /b 1
)

set GLSLC="%VULKAN_SDK%\Bin\glslc.exe"

if not exist %GLSLC% (
    echo glslc compiler not found at %GLSLC%
    exit /b 1
)

echo Compiling Shaders...

:: Terrain Shaders
%GLSLC% shaders\terrain.vert -o shaders\terrain.vert.spv
if errorlevel 1 goto :error
%GLSLC% shaders\terrain.frag -o shaders\terrain.frag.spv
if errorlevel 1 goto :error
echo Built terrain.spv

:: Entity Shaders
%GLSLC% shaders\entity.vert -o shaders\entity.vert.spv
if errorlevel 1 goto :error
%GLSLC% shaders\entity.frag -o shaders\entity.frag.spv
if errorlevel 1 goto :error
echo Built entity.spv

:: Sky Shaders (using same as HUD for now)
%GLSLC% shaders\hud.vert -o shaders\sky.vert.spv
if errorlevel 1 goto :error
%GLSLC% shaders\hud.frag -o shaders\sky.frag.spv
if errorlevel 1 goto :error
echo Built sky.spv

:: HUD Shaders
%GLSLC% shaders\hud.vert -o shaders\hud.vert.spv
if errorlevel 1 goto :error
%GLSLC% shaders\hud.frag -o shaders\hud.frag.spv
if errorlevel 1 goto :error
echo Built hud.spv

echo Done.
exit /b 0

:error
echo Failed to compile shaders
exit /b 1
