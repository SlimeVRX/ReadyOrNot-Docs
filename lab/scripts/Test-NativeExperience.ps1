param(
    [string]$Workspace = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path,
    [switch]$WithoutAudio,
    [switch]$InspectLoadoutUI
)
$ErrorActionPreference = 'Stop'
$projectDir = Join-Path $Workspace 'Ready Or Not'
$outDir = Join-Path $projectDir 'Saved\GunLab'
$project = Join-Path $projectDir 'ReadyOrNot.uproject'
$binaryDir = Join-Path $Workspace 'Engine\Binaries\Win64'
if (Get-Process -Name UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue) { throw 'Close the editor/commandlet before running the standalone native experience check.' }
New-Item -ItemType Directory -Force -Path $outDir | Out-Null
foreach ($fileName in @('experience_receipt.json','Experience.png','NativeLoadout.png')) {
    $filePath = Join-Path $outDir $fileName
    if (Test-Path -LiteralPath $filePath) {
        $archivedName = ([IO.Path]::GetFileNameWithoutExtension($fileName)) + '-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + [IO.Path]::GetExtension($fileName)
        Move-Item -LiteralPath $filePath -Destination (Join-Path $outDir $archivedName)
    }
}
$arguments = @(('"' + $project + '"'), '/Game/ReadyOrNot/Level/Study/ReadyOrNot_GunLab', '-game', '-GunLabExperienceQA', '-GunLabExit', '-unattended', '-nop4', '-nosplash', '-RenderOffscreen', '-windowed', '-ResX=1600', '-ResY=900', '-ExecCmds="t.MaxFPS 60,viewmode lit"', ('-abslog="' + (Join-Path $outDir 'experience.log') + '"'))
if ($WithoutAudio) { $arguments += '-nosound' }
if ($InspectLoadoutUI) { $arguments += '-GunLabInspectLoadoutUI' }
$startedAtUtc = [DateTime]::UtcNow
$profilePreservation = $null
if ($InspectLoadoutUI) {
    # The original UI may save during open/sanitization as well as normal exit.
    # Preserve exactly this profile file; do not modify other save games.
    $profilePath = [IO.Path]::GetFullPath((Join-Path $projectDir 'Saved\SaveGames\MetaGameProfile.sav'))
    $backupName = $startedAtUtc.ToString('yyyyMMddTHHmmssfffffffZ') + '-' + [guid]::NewGuid().ToString('N')
    $profileBackupDir = Join-Path $projectDir ('Saved\Codex\ProfileBackups\' + $backupName)
    New-Item -ItemType Directory -Path $profileBackupDir | Out-Null
    $profileBackupPath = Join-Path $profileBackupDir 'MetaGameProfile.sav'
    $profileReceiptPath = Join-Path $outDir 'profile_preservation.json'
    $profileExisted = Test-Path -LiteralPath $profilePath -PathType Leaf
    $profilePreservation = [ordered]@{
        schema = 1
        scope = 'Optional native loadout UI audit: restore only MetaGameProfile.sav to its exact pre-run bytes or original absence. This does not describe earlier unbacked runs.'
        started_at_utc = $startedAtUtc.ToString('o')
        profile_path = $profilePath
        backup_path = $(if ($profileExisted) { $profileBackupPath } else { $null })
        before_exists = $profileExisted
        before_sha256 = $null
        before_last_write_utc = $null
        after_process_exists = $null
        after_process_sha256 = $null
        after_process_last_write_utc = $null
        restored_exists = $null
        restored_sha256 = $null
        finished_at_utc = $null
        status = 'backed_up'
    }
    if ($profileExisted) {
        $profilePreservation.before_sha256 = (Get-FileHash -LiteralPath $profilePath -Algorithm SHA256).Hash
        $profilePreservation.before_last_write_utc = (Get-Item -LiteralPath $profilePath).LastWriteTimeUtc.ToString('o')
        Copy-Item -LiteralPath $profilePath -Destination $profileBackupPath
        if ((Get-FileHash -LiteralPath $profileBackupPath -Algorithm SHA256).Hash -ne $profilePreservation.before_sha256) { throw 'Profile backup hash mismatch; Unreal was not launched.' }
    }
    $profilePreservation | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $profileReceiptPath -Encoding UTF8
}
try {
    $process = Start-Process -FilePath (Join-Path $binaryDir 'UnrealEditor-Cmd.exe') -ArgumentList $arguments -WorkingDirectory $binaryDir -WindowStyle Hidden -Wait -PassThru
}
finally {
    if ($profilePreservation) {
        try {
            $profilePreservation.after_process_exists = Test-Path -LiteralPath $profilePath -PathType Leaf
            if ($profilePreservation.after_process_exists) {
                $profilePreservation.after_process_sha256 = (Get-FileHash -LiteralPath $profilePath -Algorithm SHA256).Hash
                $profilePreservation.after_process_last_write_utc = (Get-Item -LiteralPath $profilePath).LastWriteTimeUtc.ToString('o')
            }
            if ($profilePreservation.before_exists) {
                if ((Get-FileHash -LiteralPath $profileBackupPath -Algorithm SHA256).Hash -ne $profilePreservation.before_sha256) { throw 'Profile backup changed; refusing to restore unverified bytes.' }
                New-Item -ItemType Directory -Force -Path (Split-Path -Parent $profilePath) | Out-Null
                Copy-Item -LiteralPath $profileBackupPath -Destination $profilePath -Force
            }
            elseif (Test-Path -LiteralPath $profilePath) {
                if (-not (Test-Path -LiteralPath $profilePath -PathType Leaf)) { throw 'Profile path is not a file; refusing to remove it.' }
                Remove-Item -LiteralPath $profilePath -Force
            }
            $profilePreservation.restored_exists = Test-Path -LiteralPath $profilePath -PathType Leaf
            if ($profilePreservation.restored_exists) { $profilePreservation.restored_sha256 = (Get-FileHash -LiteralPath $profilePath -Algorithm SHA256).Hash }
            if ($profilePreservation.restored_exists -ne $profilePreservation.before_exists -or $profilePreservation.restored_sha256 -ne $profilePreservation.before_sha256) { throw 'Profile restoration verification failed; inspect the retained backup.' }
            $profilePreservation.status = 'restored'
        }
        catch {
            $profilePreservation.status = 'restore_failed'
            $profilePreservation['error'] = $_.Exception.Message
            throw
        }
        finally {
            $profilePreservation.finished_at_utc = [DateTime]::UtcNow.ToString('o')
            $profilePreservation | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $profileReceiptPath -Encoding UTF8
            Copy-Item -LiteralPath $profileReceiptPath -Destination (Join-Path $profileBackupDir 'preservation.json')
        }
    }
}
if ($process.ExitCode -ne 0) { throw "Native experience process exited $($process.ExitCode). Read Saved/GunLab/experience.log." }
$receiptPath = Join-Path $outDir 'experience_receipt.json'
if (-not (Test-Path -LiteralPath $receiptPath) -or (Get-Item -LiteralPath $receiptPath).LastWriteTimeUtc -lt $startedAtUtc) { throw 'Missing fresh native experience receipt.' }
$receipt = Get-Content -LiteralPath $receiptPath -Raw -Encoding UTF8 | ConvertFrom-Json
if (-not $receipt.complete -or $receipt.mode -ne 'native_experience_input_probe') { throw 'Native experience sequence incomplete. Inspect observations.' }
$names = @($receipt.observations | ForEach-Object step)
foreach ($required in @('baseline','fire_selector_after_short_press','ads','canted','fire_and_recoil','tactical_reload','held_magcheck','low_ready','lean_right','crouch','free_look','secondary_slot','primary_slot_restored','optic_attached','optic_ads','attachment_toggle','native_target_before','native_target_after_live_fire','help_open')) {
    if ($required -notin $names) { throw "Missing native observation $required" }
}
if (@($names | Where-Object { $_ -match 'timeout|missing' }).Count) { throw 'Native experience reported missing fixture or timeout.' }
$imagePath = Join-Path $outDir 'Experience.png'
if (-not (Test-Path -LiteralPath $imagePath) -or (Get-Item -LiteralPath $imagePath).LastWriteTimeUtc -lt $startedAtUtc) { throw 'No fresh native experience screenshot.' }
if ($InspectLoadoutUI -and (-not $receipt.native_loadout_ui_opened_by_probe -or -not (Test-Path -LiteralPath (Join-Path $outDir 'NativeLoadout.png')))) { throw 'Native loadout UI did not present and capture.' }
& node (Join-Path $PSScriptRoot 'verify_native_experience.mjs') $receiptPath --output (Join-Path $outDir 'experience_verification.json')
if ($LASTEXITCODE -ne 0) { throw 'Native experience assertions failed or evidence is missing. Inspect Saved/GunLab/experience_verification.json.' }
Write-Output 'Native input assertions passed. Inspect the screenshot and test audio/subjective feel interactively.'
