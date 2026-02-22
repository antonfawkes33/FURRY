@echo off
set SDK_BIN=C:\VulkanSDK\1.4.341.1\Bin
if not exist %SDK_BIN%\glslc.exe (
    echo Vulkan SDK glslc.exe not found at %SDK_BIN%
    exit /b 1
)

mkdir ..\shaders\spirv 2>nul
for %%f in (..\shaders\*.vert ..\shaders\*.frag) do (
    echo Compiling %%f...
    %SDK_BIN%\glslc.exe %%f -o ..\shaders\spirv\%%~nxf.spv
)
