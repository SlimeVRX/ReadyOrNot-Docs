param(
    [ValidateSet('Generate','Editor','Audit','Probe','Capture')][string]$Mode = 'Editor',
    [int]$WeaponIndex = 1,
    [string]$Workspace = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
)
$ErrorActionPreference = 'Stop'
$projectDir = Join-Path $Workspace 'Ready Or Not'
$project = Join-Path $projectDir 'ReadyOrNot.uproject'
$binaryDir = Join-Path $Workspace 'Engine\Binaries\Win64'
$logDir = Join-Path $projectDir 'Saved\GunLab'
New-Item -ItemType Directory -Path $logDir -Force | Out-Null
$map = '/Game/ReadyOrNot/Level/Study/ReadyOrNot_GunLab'
$arguments = @(('"' + $project + '"'), '-nop4', '-nosplash')
if ($Mode -eq 'Editor') {
    $arguments += $map
    # The interactive editor is intentionally visible for the user.
    Start-Process -FilePath (Join-Path $binaryDir 'UnrealEditor.exe') -ArgumentList $arguments -WorkingDirectory $binaryDir
} else {
    if ($Mode -eq 'Generate') {
        $script = Join-Path $PSScriptRoot 'generate_gun_lab.py'
        $arguments += @('-run=pythonscript', ('-script="' + $script + '"'))
    } elseif ($Mode -in @('Probe','Capture')) {
        $arguments += @($map, '-game', '-GunLabProbe', '-GunLabExit', "-GunLabIndex=$WeaponIndex")
        if ($Mode -eq 'Capture') { $arguments += @('-GunLabCapture', '-RenderOffscreen', '-windowed', '-ResX=1600', '-ResY=900', '-ExecCmds="t.MaxFPS 60"') }
    } else {
        $arguments += @($map, '-game', '-GunLabSmoke', '-GunLabExit')
    }
    $arguments += @('-unattended', '-nosound', ('-abslog="' + (Join-Path $logDir ($Mode.ToLower() + '.log')) + '"'))
    if ($Mode -ne 'Capture') { $arguments += '-nullrhi' }
    $process = Start-Process -FilePath (Join-Path $binaryDir 'UnrealEditor-Cmd.exe') -ArgumentList $arguments -WorkingDirectory $binaryDir -WindowStyle Hidden -PassThru -Wait
    if ($process.ExitCode -ne 0) { throw "Unreal exited $($process.ExitCode). Inspect $logDir" }
}
