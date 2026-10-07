param(
    [string]$Workspace = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path,
    [string]$SdkVersion = '10.0.22621.0',
    [string]$CompilerVersion = '14.36.32532'
)
$ErrorActionPreference = 'Stop'
$projectDir = Join-Path $Workspace 'Ready Or Not'
$project = Join-Path $projectDir 'ReadyOrNot.uproject'
$source = Join-Path $PSScriptRoot '..\plugin\ReadyOrNotGunLab'
$destination = Join-Path $projectDir 'Plugins\ReadyOrNotGunLab'
if (-not (Test-Path -LiteralPath $project)) { throw "Project missing: $project" }
if (Get-Process -Name UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue) {
    throw 'Close the editor and any commandlet before updating its plugin binaries.'
}
New-Item -ItemType Directory -Path $destination -Force | Out-Null
Copy-Item -Path (Join-Path $source '*') -Destination $destination -Recurse -Force
& (Join-Path $Workspace 'Engine\Build\BatchFiles\Build.bat') ReadyOrNotEditor Win64 Development "-Project=$project" -WaitMutex -NoHotReloadFromIDE "-WindowsSDKVersion=$SdkVersion" "-CompilerVersion=$CompilerVersion"
if ($LASTEXITCODE -ne 0) { throw "Build failed: $LASTEXITCODE" }
$pluginDll = Join-Path $destination 'Binaries\Win64\UnrealEditor-ReadyOrNotGunLab.dll'
if (-not (Test-Path -LiteralPath $pluginDll)) { throw "Build returned success but plugin DLL is missing: $pluginDll" }
$engineVersion = Get-Content -LiteralPath (Join-Path $Workspace 'Engine\Build\Build.version') -Raw | ConvertFrom-Json
$manifestDir = Join-Path $PSScriptRoot '..\manifests'
New-Item -ItemType Directory -Path $manifestDir -Force | Out-Null
$buildReceipt = [ordered]@{
    build_verified_utc = [DateTime]::UtcNow.ToString('o')
    engine = ('custom UE {0}.{1}.{2}' -f $engineVersion.MajorVersion, $engineVersion.MinorVersion, $engineVersion.PatchVersion)
    target = 'ReadyOrNotEditor Win64 Development'
    requested_compiler_directory_version = $CompilerVersion
    requested_windows_sdk = $SdkVersion
    build_exit_code = 0
    plugin_dll_sha256 = (Get-FileHash -LiteralPath $pluginDll -Algorithm SHA256).Hash
    plugin_dll_last_write_utc = (Get-Item -LiteralPath $pluginDll).LastWriteTimeUtc.ToString('o')
    scope = 'Successful Build.bat completion and resulting DLL fingerprint. Requested toolchain flags are not a parsed compiler-version report. Rerun runtime checks after a changed binary.'
}
$buildReceipt | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $manifestDir 'build_receipt.json') -Encoding utf8
Write-Output ('Build verified; updated lab/manifests/build_receipt.json for DLL ' + $buildReceipt.plugin_dll_sha256)
