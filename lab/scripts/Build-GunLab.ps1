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
