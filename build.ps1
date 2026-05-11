param(
    [string]$Arch = "sm_86"  # ignored for OpenCL build, kept for compatibility
)

$Source = "gpu-miner.c"
$Output = "gpu-miner.exe"
$VCVarsPaths = @(
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat",
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat",
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat",
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\19\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
)

function Find-Vcvars {
    foreach ($p in $VCVarsPaths) {
        if (Test-Path -LiteralPath $p) { return $p }
    }
    $roots = @(
        "${env:ProgramFiles}\Microsoft Visual Studio",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio"
    )
    foreach ($root in $roots) {
        if (-not $root) { continue }
        $paths = Get-ChildItem -Path "$root\*\*\VC\Auxiliary\Build\vcvars64.bat" -ErrorAction SilentlyContinue
        if ($paths) { return $paths[-1].FullName }
    }
    return $null
}

$vcvarsPath = Find-Vcvars
if (-not $vcvarsPath) {
    Write-Error "Visual Studio C++ compiler not found. Install Visual Studio with 'Desktop development with C++' workload."
    exit 1
}

Write-Host "Found vcvars at: $vcvarsPath"
Write-Host "Compiling $Source ..."

$cudaLib = "${env:ProgramFiles}\NVIDIA GPU Computing Toolkit\CUDA\v13.2\lib\x64"
$cmd = "call `"$vcvarsPath`" > nul 2>&1 && cl.exe -O2 -Fe:`"$Output`" `"$Source`" /link /libpath:`"$cudaLib`" OpenCL.lib"
& cmd.exe /c $cmd

if ($LASTEXITCODE -eq 0) {
    Write-Host "Build successful: $Output"
} else {
    Write-Error "Build failed with exit code $LASTEXITCODE"
    exit $LASTEXITCODE
}
