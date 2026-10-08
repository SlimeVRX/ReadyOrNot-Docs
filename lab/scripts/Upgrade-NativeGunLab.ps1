param(
    [string]$Workspace = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path,
    [string]$CameraNode = $env:RON_GUNLAB_CAMERA_NODE,
    [switch]$SkipBuild,
    [switch]$TestExperience
)
$ErrorActionPreference = 'Stop'
if (Get-Process -Name UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue) { throw 'Close Unreal before upgrading the local lab.' }
if (-not $CameraNode) { $CameraNode = (Get-Command node -ErrorAction SilentlyContinue).Source }
if (-not $CameraNode -or -not (Test-Path -LiteralPath $CameraNode -PathType Leaf)) { throw 'Install Node 18+, provide -CameraNode, or set RON_GUNLAB_CAMERA_NODE.' }
$CameraNode = (Resolve-Path -LiteralPath $CameraNode).Path
$projectDir = Join-Path $Workspace 'Ready Or Not'
$saved = Join-Path $projectDir 'Saved\GunLab'
New-Item -ItemType Directory -Force -Path $saved | Out-Null
& $CameraNode (Join-Path $PSScriptRoot 'repair_native_gunplay_port.mjs') $Workspace
if ($LASTEXITCODE -ne 0) { throw 'Guarded native migration repair failed.' }
if (-not $SkipBuild) { & (Join-Path $PSScriptRoot 'Build-GunLab.ps1') -Workspace $Workspace }
$mapFile = Join-Path $projectDir 'Content\ReadyOrNot\Level\Study\ReadyOrNot_GunLab.umap'
if (-not (Test-Path -LiteralPath $mapFile)) { & (Join-Path $PSScriptRoot 'Run-GunLab.ps1') -Workspace $Workspace -Mode Generate }
& $CameraNode (Join-Path $PSScriptRoot 'extract_legacy_camera_tracks.mjs') $Workspace
if ($LASTEXITCODE -ne 0) { throw 'Legacy camera extraction failed.' }
# JSON string literals containing forward-slash paths are also valid Python strings.
# The wrapper stays in Saved; it contains paths only, never source assets or curves.
$scriptsPath = ($PSScriptRoot -replace '\\','/') | ConvertTo-Json -Compress
$wrapper = Join-Path $saved 'run_native_upgrade.py'
$python = @"
import os, runpy, json, datetime, unreal
scripts = $scriptsPath
runpy.run_path(os.path.join(scripts, 'upgrade_gun_lab.py'), run_name='__main__')
runpy.run_path(os.path.join(scripts, 'recover_legacy_camera_sequences.py'), run_name='__main__')
with open(os.path.join(unreal.Paths.project_saved_dir(), 'GunLab', 'native_upgrade_pipeline.json'), 'w', encoding='utf-8') as handle:
    json.dump({'complete': True, 'completed_utc': datetime.datetime.now(datetime.timezone.utc).isoformat()}, handle)
"@
Set-Content -LiteralPath $wrapper -Value $python -Encoding utf8
$binaryDir = Join-Path $Workspace 'Engine\Binaries\Win64'
$started = [DateTime]::UtcNow
$arguments = @(('"' + (Join-Path $projectDir 'ReadyOrNot.uproject') + '"'), '-run=pythonscript', ('-script="' + ($wrapper -replace '\\','/') + '"'), '-unattended', '-nop4', '-nosplash', '-nullrhi', '-nosound', ('-abslog="' + (Join-Path $saved 'native-upgrade.log') + '"'))
$previousCameraNode = $env:RON_GUNLAB_CAMERA_NODE
try {
    $env:RON_GUNLAB_CAMERA_NODE = $CameraNode
    $process = Start-Process -FilePath (Join-Path $binaryDir 'UnrealEditor-Cmd.exe') -ArgumentList $arguments -WorkingDirectory $binaryDir -WindowStyle Hidden -Wait -PassThru
} finally {
    if ($null -eq $previousCameraNode) { Remove-Item Env:RON_GUNLAB_CAMERA_NODE -ErrorAction SilentlyContinue }
    else { $env:RON_GUNLAB_CAMERA_NODE = $previousCameraNode }
}
if ($process.ExitCode -notin @(0,1)) { throw "Native upgrade exited $($process.ExitCode); inspect Saved/GunLab/native-upgrade.log." }
foreach ($name in @('native_upgrade_receipt.json','legacy-camera-recovery-map.json','native_upgrade_pipeline.json')) {
    $file = Join-Path $saved $name
    if (-not (Test-Path -LiteralPath $file) -or (Get-Item -LiteralPath $file).LastWriteTimeUtc -lt $started) { throw "Missing fresh $name" }
}
$pipelinePath = Join-Path $saved 'native_upgrade_pipeline.json'
$pipeline = Get-Content -LiteralPath $pipelinePath -Raw | ConvertFrom-Json
if (-not $pipeline.complete) { throw 'The Python upgrade pipeline did not reach its completion marker.' }
$pipeline | Add-Member -NotePropertyName commandlet_exit_code -NotePropertyValue $process.ExitCode -Force
$pipeline | ConvertTo-Json | Set-Content -LiteralPath $pipelinePath -Encoding utf8
if ($process.ExitCode -eq 1) { Write-Warning 'Both upgrade stages completed, but Unreal reported asset/compiler errors. Inspect native-upgrade.log; runtime behavior still needs verification.' }
$camera = Get-Content -LiteralPath (Join-Path $saved 'legacy-camera-recovery-map.json') -Raw | ConvertFrom-Json
if (-not $camera.validated_transform_semantics -or $camera.conversion_failure_count -ne 0 -or @($camera.mappings).Count -eq 0) { throw 'No complete validated native camera mapping. Read the camera rejection report.' }
Write-Output 'Native fixtures and validated local camera assets saved. Runtime tests are separate evidence.'
if ($TestExperience) { & (Join-Path $PSScriptRoot 'Test-NativeExperience.ps1') -Workspace $Workspace }
