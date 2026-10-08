param(
    [ValidateSet('Generate','Editor','Audit','Probe','Capture')][string]$Mode = 'Editor',
    [int]$WeaponIndex = 1,
    [int[]]$WeaponIndices = @(),
    [string]$Workspace = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
)
$ErrorActionPreference = 'Stop'
if (Get-Process -Name UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue) {
    throw 'An Unreal editor or commandlet is already running. Close it before starting a separate lab session.'
}

function Assert-GunLabReceipt {
    param(
        [object]$Receipt,
        [ValidateSet('Audit','Probe','Capture')][string]$RunMode,
        [int[]]$RequestedIndices = @()
    )
    $expectedMode = if ($RunMode -eq 'Audit') { 'equip_audit' } else { 'action_probe' }
    if ($Receipt.mode -ne $expectedMode) { throw "Expected $expectedMode receipt, got '$($Receipt.mode)'." }
    $candidateCount = 0
    if (-not [int]::TryParse([string]$Receipt.candidate_count, [ref]$candidateCount) -or $candidateCount -le 0) {
        throw 'Receipt has no valid positive candidate_count.'
    }
    $rows = @($Receipt.results)
    if ($rows.Count -eq 0 -or $null -eq $rows[0]) { throw 'Receipt has no result rows.' }
    $allowedStatuses = if ($RunMode -eq 'Audit') {
        @('equipped', 'equipped_instant_fallback', 'class_rejected')
    } else {
        @('equipped', 'equipped_instant_fallback', 'action_probe')
    }
    foreach ($row in $rows) {
        $rowIndex = -1
        if (-not [int]::TryParse([string]$row.index, [ref]$rowIndex) -or $rowIndex -lt 0 -or $rowIndex -ge $candidateCount) {
            throw 'Receipt contains a missing or out-of-range result index.'
        }
        if ($row.status -notin $allowedStatuses) {
            throw "Gun Lab reported '$($row.status)' for index $($rowIndex + 1): $($row.detail)"
        }
    }
    if ($RunMode -eq 'Audit') {
        $expected = @(0..($candidateCount - 1))
        $completed = $rows
    } else {
        $expected = @($RequestedIndices | Sort-Object -Unique | ForEach-Object { $_ - 1 })
        if ($expected.Count -eq 0 -or @($expected | Where-Object { $_ -lt 0 -or $_ -ge $candidateCount }).Count -gt 0) {
            throw "Requested probe indices must be between 1 and $candidateCount."
        }
        $completed = @($rows | Where-Object { $_.status -eq 'action_probe' })
        if (@($rows | Where-Object { [int]$_.index -notin $expected }).Count -gt 0) {
            throw 'Probe receipt contains a result for an unrequested index.'
        }
    }
    $actual = @($completed | ForEach-Object { [int]$_.index })
    $uniqueActual = @($actual | Sort-Object -Unique)
    $missing = @($expected | Where-Object { $_ -notin $uniqueActual } | ForEach-Object { $_ + 1 })
    if ($actual.Count -ne $expected.Count -or $uniqueActual.Count -ne $expected.Count -or $missing.Count -gt 0) {
        throw "Incomplete or duplicate $expectedMode results: expected $($expected.Count), got $($actual.Count) rows ($($uniqueActual.Count) distinct). Missing 1-based indices: $($missing -join ', ')."
    }
    # Ammo, aiming and reload observations may legitimately be false. Preserve
    # them as measured outcomes instead of treating them as runner failures.
}

$projectDir = Join-Path $Workspace 'Ready Or Not'
$project = Join-Path $projectDir 'ReadyOrNot.uproject'
$binaryDir = Join-Path $Workspace 'Engine\Binaries\Win64'
$logDir = Join-Path $projectDir 'Saved\GunLab'
New-Item -ItemType Directory -Path $logDir -Force | Out-Null
$map = '/Game/ReadyOrNot/Level/Study/ReadyOrNot_GunLab'
$arguments = @(('"' + $project + '"'), '-nop4', '-nosplash')
$requestedProbeIndices = @()
if ($Mode -eq 'Editor') {
    # UnrealEdMisc reads only the first command-line token as the initial map.
    # Put the map immediately after the project, before optional switches.
    $arguments = @(('"' + $project + '"'), $map, '-nop4', '-nosplash')
    # The interactive editor is intentionally visible for the user.
    Start-Process -FilePath (Join-Path $binaryDir 'UnrealEditor.exe') -ArgumentList $arguments -WorkingDirectory $binaryDir
} else {
    if ($Mode -eq 'Generate') {
        $script = Join-Path $PSScriptRoot 'generate_gun_lab.py'
        $arguments += @('-run=pythonscript', ('-script="' + $script + '"'))
    } elseif ($Mode -in @('Probe','Capture')) {
        $requestedProbeIndices = if ($WeaponIndices.Count -gt 0) { @($WeaponIndices | Sort-Object -Unique) } else { @($WeaponIndex) }
        if (@($requestedProbeIndices | Where-Object { $_ -lt 1 }).Count -gt 0) { throw 'Probe indices are 1-based positive integers.' }
        $arguments += @($map, '-game', '-GunLabProbe', '-GunLabExit', "-GunLabIndex=$WeaponIndex")
        # Windows UE reconstructs quotes only for arguments containing spaces;
        # FParse::Value otherwise stops at the first comma, even if we quoted it.
        if ($WeaponIndices.Count -gt 0) { $arguments += ('-GunLabProbeIndices="' + ($WeaponIndices -join ', ') + '"') }
        if ($Mode -eq 'Capture') {
            $previousCapture = Join-Path $logDir 'Range.png'
            if (Test-Path -LiteralPath $previousCapture) {
                Move-Item -LiteralPath $previousCapture -Destination (Join-Path $logDir ('Range-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.png'))
            }
            $arguments += @('-GunLabCapture', '-RenderOffscreen', '-windowed', '-ResX=1600', '-ResY=900', '-ExecCmds="t.MaxFPS 60"')
        }
    } else {
        $arguments += @($map, '-game', '-GunLabSmoke', '-GunLabExit')
    }
    $arguments += @('-unattended', '-nosound', ('-abslog="' + (Join-Path $logDir ($Mode.ToLower() + '.log')) + '"'))
    if ($Mode -ne 'Capture') { $arguments += '-nullrhi' }
    $startedAtUtc = [DateTime]::UtcNow
    $process = Start-Process -FilePath (Join-Path $binaryDir 'UnrealEditor-Cmd.exe') -ArgumentList $arguments -WorkingDirectory $binaryDir -WindowStyle Hidden -PassThru -Wait
    if ($process.ExitCode -ne 0) { throw "Unreal exited $($process.ExitCode). Inspect $logDir" }
    if ($Mode -in @('Audit','Probe','Capture')) {
        $receiptName = if ($Mode -eq 'Audit') { 'equip_audit_receipt.json' } else { 'action_probe_receipt.json' }
        $receiptPath = Join-Path $logDir $receiptName
        if (-not (Test-Path -LiteralPath $receiptPath -PathType Leaf)) { throw "Missing runtime receipt: $receiptPath" }
        if ((Get-Item -LiteralPath $receiptPath).LastWriteTimeUtc -lt $startedAtUtc) { throw "Stale runtime receipt: $receiptPath" }
        $receipt = Get-Content -LiteralPath $receiptPath -Raw -Encoding UTF8 | ConvertFrom-Json
        Assert-GunLabReceipt -Receipt $receipt -RunMode $Mode -RequestedIndices $requestedProbeIndices
        if ($Mode -eq 'Capture') {
            $capturePath = Join-Path $logDir 'Range.png'
            if (-not (Test-Path -LiteralPath $capturePath -PathType Leaf)) { throw "Capture completed without a screenshot: $capturePath" }
            $captureItem = Get-Item -LiteralPath $capturePath
            if ($captureItem.LastWriteTimeUtc -lt $startedAtUtc -or $captureItem.Length -eq 0) {
                throw "Capture did not produce a fresh, nonempty screenshot: $capturePath"
            }
        }
        Write-Host "$Mode completed with a fresh, complete runtime receipt. Observed gunplay outcomes remain in $receiptPath"
    }
}
