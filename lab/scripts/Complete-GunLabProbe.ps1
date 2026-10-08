param(
    [int[]]$WeaponIndices = @(1..102 | Where-Object { $_ -ne 74 }),
    [string]$ResumeReceipt,
    [ValidateRange(1,32)][int]$MaxSessions = 8,
    [switch]$ValidateOnly,
    [string]$Workspace = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
)
$ErrorActionPreference = 'Stop'

function Get-ProbeInputs {
    param([hashtable]$Paths)
    $fingerprints = [ordered]@{}
    $latest = [DateTime]::MinValue
    foreach ($key in @('tested_binary_sha256','tested_host_game_binary_sha256','tested_map_sha256','tested_camera_mapping_sha256')) {
        if (-not (Test-Path -LiteralPath $Paths[$key] -PathType Leaf)) { throw "Missing current input: $($Paths[$key])" }
        $fingerprints[$key] = (Get-FileHash -LiteralPath $Paths[$key] -Algorithm SHA256).Hash
        $modified = (Get-Item -LiteralPath $Paths[$key]).LastWriteTimeUtc
        if ($modified -gt $latest) { $latest = $modified }
    }
    [pscustomobject]@{ fingerprints = $fingerprints; latest_write_utc = $latest }
}

function Assert-ProbeInputsUnchanged {
    param([object]$Before, [object]$After)
    foreach ($key in $Before.fingerprints.Keys) {
        if ($Before.fingerprints[$key] -ne $After.fingerprints[$key]) { throw "Input changed while completing probes: $key. Results cannot be combined." }
    }
}

function Read-CheckedNativeProbe {
    param([string]$Path, [object[]]$Catalog, [int[]]$AllowedIndices, [DateTime]$FreshAfter)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Missing native receipt: $Path" }
    $item = Get-Item -LiteralPath $Path
    if ($item.LastWriteTimeUtc -lt $FreshAfter) { throw "Native receipt predates current inputs or this session: $Path" }
    $data = Get-Content -LiteralPath $Path -Raw -Encoding UTF8 | ConvertFrom-Json
    if ($data.mode -ne 'action_probe' -or $data.map -ne 'ReadyOrNot_GunLab' -or $data.candidate_count -ne $Catalog.Count) { throw 'Native receipt mode/map/catalog mismatch.' }
    if ($data.run_id_utc -notmatch '^\d{8}-\d{6}$' -or $data.source_runs) { throw 'Resume accepts a native per-run receipt, not an aggregate or unidentified receipt.' }
    $rows = @($data.results)
    if (-not $rows.Count -or $null -eq $rows[0]) { throw 'Native receipt has no outcomes.' }
    $byIndex = @{}
    foreach ($row in $rows) {
        $index = -1
        if (-not [int]::TryParse([string]$row.index, [ref]$index) -or $index -lt 0 -or $index -ge $Catalog.Count) { throw 'Invalid native receipt index.' }
        if (($index + 1) -notin $AllowedIndices) { throw "Native receipt includes unrequested lab #$($index + 1)." }
        if ($row.class_path -ne $Catalog[$index].class_path) { throw "Native receipt class/index mismatch at lab #$($index + 1)." }
        if ($row.status -notin @('equipped','equipped_instant_fallback','action_probe')) { throw "Explicit native failure at lab #$($index + 1): $($row.status)." }
        if ($row.held_class -and $row.held_class -ne $row.class_path) { throw "Held class mismatch at lab #$($index + 1)." }
        if ($row.native_owner -ne $true) { throw "Native ownership was not confirmed at lab #$($index + 1)." }
        if (-not $byIndex.ContainsKey($index)) { $byIndex[$index] = @() }
        $byIndex[$index] += $row
    }
    $actions = @()
    foreach ($index in @($byIndex.Keys | Sort-Object)) {
        $group = @($byIndex[$index])
        $equipRows = @($group | Where-Object { $_.status -in @('equipped','equipped_instant_fallback') })
        $actionRows = @($group | Where-Object { $_.status -eq 'action_probe' })
        if ($equipRows.Count -ne 1 -or $actionRows.Count -gt 1) { throw "Duplicate/missing equip or duplicate action at lab #$($index + 1)." }
        if ($actionRows.Count -eq 1) {
            if ($group[0].status -eq 'action_probe') { throw "Action precedes equip at lab #$($index + 1)." }
            $actions += [int]$index
        }
    }
    if (-not $actions.Count) { throw 'Native session made no completed action progress; will not retry automatically.' }
    [pscustomobject]@{ data = $data; rows = $rows; completed = @($actions); observed = @($byIndex.Keys | Sort-Object);
        sha256 = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash; last_write_utc = $item.LastWriteTimeUtc }
}

function Invoke-CheckedProbeRunner {
    param([string]$RunnerPath, [string]$Root, [int[]]$Indices)
    # Dot-source only inside this isolated invocation scope. This exposes the
    # runner's real Process.ExitCode even if its complete-batch assertion throws.
    $process = $null
    $startedAtUtc = $null
    $runnerError = $null
    try { . $RunnerPath -Mode Probe -WeaponIndices $Indices -Workspace $Root }
    catch { $runnerError = $_.Exception.Message }
    [pscustomobject]@{ exit_code = $(if ($process) { $process.ExitCode } else { $null });
        started_at_utc = $startedAtUtc; error = $runnerError }
}

$completionProject = Join-Path $Workspace 'Ready Or Not'
$completionSaved = Join-Path $completionProject 'Saved\GunLab'
$completionCanonical = Join-Path $completionSaved 'action_probe_receipt.json'
$completionCatalogPath = Join-Path $PSScriptRoot '..\manifests\selection_order.json'
$completionCatalog = @(Get-Content -LiteralPath $completionCatalogPath -Raw -Encoding UTF8 | ConvertFrom-Json | Sort-Object index)
for ($i = 0; $i -lt $completionCatalog.Count; $i++) {
    if ($completionCatalog[$i].index -ne ($i + 1) -or -not $completionCatalog[$i].class_path) { throw 'Selection order must use contiguous one-based indices and class paths.' }
}
if (@($completionCatalog.class_path | Sort-Object -Unique).Count -ne $completionCatalog.Count) { throw 'Selection order contains duplicate classes.' }
$completionRequested = @($WeaponIndices | Sort-Object -Unique)
if (-not $completionRequested.Count -or @($completionRequested | Where-Object { $_ -lt 1 -or $_ -gt $completionCatalog.Count }).Count) { throw 'Requested probe indices are empty or out of catalog range.' }
$completionPaths = @{
    tested_binary_sha256 = (Join-Path $completionProject 'Plugins\ReadyOrNotGunLab\Binaries\Win64\UnrealEditor-ReadyOrNotGunLab.dll')
    tested_host_game_binary_sha256 = (Join-Path $completionProject 'Binaries\Win64\UnrealEditor-ReadyOrNot.dll')
    tested_map_sha256 = (Join-Path $completionProject 'Content\ReadyOrNot\Level\Study\ReadyOrNot_GunLab.umap')
    tested_camera_mapping_sha256 = (Join-Path $completionSaved 'legacy-camera-recovery-map.json')
}
$completionInputs = Get-ProbeInputs -Paths $completionPaths
$completionResume = $null
if ($ResumeReceipt) {
    $completionResume = Read-CheckedNativeProbe -Path ([IO.Path]::GetFullPath($ResumeReceipt)) -Catalog $completionCatalog -AllowedIndices $completionRequested -FreshAfter $completionInputs.latest_write_utc
}
$completionAlready = @($(if ($completionResume) { $completionResume.completed | ForEach-Object { $_ + 1 } }))
if ($ValidateOnly) {
    [pscustomobject]@{ validation_only = $true; requested = $completionRequested.Count; existing_completed = $completionAlready.Count;
        remaining_indices_one_based = @($completionRequested | Where-Object { $_ -notin $completionAlready }); fingerprints = $completionInputs.fingerprints } | ConvertTo-Json -Depth 5
    return
}
if (Get-Process -Name UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue) { throw 'Close Unreal before completing the probe in separate sessions.' }
New-Item -ItemType Directory -Path $completionSaved -Force | Out-Null
$completionBatchId = 'multipart-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N').Substring(0,8)
$completionBackupDir = Join-Path $completionSaved ('ProbeCompletionBackups\' + $completionBatchId)
New-Item -ItemType Directory -Path $completionBackupDir | Out-Null
$completionBackup = Join-Path $completionBackupDir 'action_probe_receipt.json'
$completionHadCanonical = Test-Path -LiteralPath $completionCanonical -PathType Leaf
$completionOriginalHash = $null
if ($completionHadCanonical) {
    $completionOriginalHash = (Get-FileHash -LiteralPath $completionCanonical -Algorithm SHA256).Hash
    Copy-Item -LiteralPath $completionCanonical -Destination $completionBackup
    if ((Get-FileHash -LiteralPath $completionBackup -Algorithm SHA256).Hash -ne $completionOriginalHash) { throw 'Receipt backup mismatch; Unreal was not launched.' }
}
$completionRows = [System.Collections.Generic.List[object]]::new()
$completionSources = [System.Collections.Generic.List[object]]::new()
$completionDone = [System.Collections.Generic.HashSet[int]]::new()
$completionRunIds = [System.Collections.Generic.HashSet[string]]::new()
$completionPublished = $false
$completionCheckpoint = Join-Path $completionSaved ('action_probe_batch_' + $completionBatchId + '.json')

function Add-CompletedProbeSource {
    param([object]$Checked, [int[]]$RequestedSubset, [string]$Origin, [object]$ExitCode, [string]$RunnerError)
    if (-not $completionRunIds.Add([string]$Checked.data.run_id_utc)) { throw 'Duplicate source run id; refusing stale receipt reuse.' }
    $nativeName = 'action_probe_' + $Checked.data.run_id_utc + '.json'
    $nativePath = Join-Path $completionSaved $nativeName
    # Never overwrite an original per-run native file.
    if (-not (Test-Path -LiteralPath $nativePath -PathType Leaf)) { throw "Missing original per-run receipt: $nativePath" }
    if ((Get-FileHash -LiteralPath $nativePath -Algorithm SHA256).Hash -ne $Checked.sha256) { throw 'Native per-run receipt differs from the supplied working copy.' }
    foreach ($index in $Checked.completed) {
        if (-not $completionDone.Add([int]$index)) { throw "Repeated completed action at lab #$($index + 1)." }
    }
    foreach ($row in $Checked.rows) {
        if ([int]$row.index -notin $Checked.completed) { continue }
        $copy = [ordered]@{}
        foreach ($property in $row.PSObject.Properties) { $copy[$property.Name] = $property.Value }
        $copy['source_run_id_utc'] = [string]$Checked.data.run_id_utc
        $completionRows.Add([pscustomobject]$copy)
    }
    $source = [ordered]@{ run_id_utc = [string]$Checked.data.run_id_utc; receipt_file = $nativeName; receipt_sha256 = $Checked.sha256;
        source_kind = $Origin; process_exit_code = $ExitCode; runner_validation_error = $RunnerError;
        requested_indices_one_based = @($RequestedSubset); completed_indices_zero_based = @($Checked.completed);
        observed_indices_zero_based = @($Checked.observed); omitted_equip_only_indices_zero_based = @($Checked.observed | Where-Object { $_ -notin $Checked.completed });
        source_row_count = $Checked.rows.Count; accepted_row_count = @($Checked.rows | Where-Object { [int]$_.index -in $Checked.completed }).Count;
        receipt_last_write_utc = $Checked.last_write_utc.ToString('o'); incomplete_for_requested_subset = ($Checked.completed.Count -lt $RequestedSubset.Count) }
    foreach ($key in $completionInputs.fingerprints.Keys) { $source[$key] = $completionInputs.fingerprints[$key] }
    $completionSources.Add([pscustomobject]$source)
}

function Write-ProbeCompletionCheckpoint {
    param([bool]$Complete)
    $aggregate = [ordered]@{ schema = 2; mode = 'action_probe'; run_id_utc = $completionBatchId; map = 'ReadyOrNot_GunLab';
        candidate_count = $completionCatalog.Count; complete = $Complete; multipart = $true;
        world_reset_between_runs = ($completionSources.Count -gt 1); requested_indices_one_based = @($completionRequested);
        completed_indices_zero_based = @($completionDone | Sort-Object); source_runs = @($completionSources.ToArray());
        runner_mode = 'Probe'; renderer_enabled = $false; audio_disabled_by_command_line = $true;
        scope = 'Completed native equip/action pairs combined across separate world sessions. Original per-run receipts are unchanged. This is not one uninterrupted test, nor certification that firing/reload/presentation succeeded.';
        resume_exit_code_scope = 'An externally supplied resume receipt has no embedded process exit code; its field stays null. Sessions launched by this wrapper require an observed process exit code of zero.';
        generated_at_utc = [DateTime]::UtcNow.ToString('o'); results = @($completionRows.ToArray()) }
    foreach ($key in $completionInputs.fingerprints.Keys) { $aggregate[$key] = $completionInputs.fingerprints[$key] }
    $aggregate | ConvertTo-Json -Depth 50 | Set-Content -LiteralPath $completionCheckpoint -Encoding UTF8
}

try {
    Assert-ProbeInputsUnchanged $completionInputs (Get-ProbeInputs -Paths $completionPaths)
    if ($completionResume) {
        Add-CompletedProbeSource -Checked $completionResume -RequestedSubset $completionRequested -Origin 'provided_native_resume_receipt' -ExitCode $null -RunnerError 'Externally supplied partial; native receipt does not store process exit code.'
        Write-ProbeCompletionCheckpoint -Complete $false
    }
    $completionSessionCount = 0
    while ($completionDone.Count -lt $completionRequested.Count) {
        if ($completionSessionCount -ge $MaxSessions) { throw "Reached MaxSessions=$MaxSessions. Preserved native runs and partial checkpoint: $completionCheckpoint" }
        $completionRemaining = @($completionRequested | Where-Object { -not $completionDone.Contains($_ - 1) })
        Assert-ProbeInputsUnchanged $completionInputs (Get-ProbeInputs -Paths $completionPaths)
        Write-Host "Fresh world session $($completionSessionCount + 1): $($completionRemaining.Count) remaining lab indices."
        $completionRun = Invoke-CheckedProbeRunner -RunnerPath (Join-Path $PSScriptRoot 'Run-GunLab.ps1') -Root $Workspace -Indices $completionRemaining
        $completionSessionCount++
        Assert-ProbeInputsUnchanged $completionInputs (Get-ProbeInputs -Paths $completionPaths)
        if ($null -eq $completionRun.exit_code -or $completionRun.exit_code -ne 0) { throw "Probe process did not exit zero: $($completionRun.exit_code). $($completionRun.error)" }
        if ($completionRun.error -and $completionRun.error -notlike 'Incomplete or duplicate action_probe results:*') { throw "Runner failed for a reason other than incomplete coverage: $($completionRun.error)" }
        if ($null -eq $completionRun.started_at_utc) { throw 'Runner did not record a start time.' }
        $completionFreshAfter = $completionInputs.latest_write_utc
        if ($completionRun.started_at_utc -gt $completionFreshAfter) { $completionFreshAfter = $completionRun.started_at_utc }
        $completionChecked = Read-CheckedNativeProbe -Path $completionCanonical -Catalog $completionCatalog -AllowedIndices $completionRemaining -FreshAfter $completionFreshAfter
        Add-CompletedProbeSource -Checked $completionChecked -RequestedSubset $completionRemaining -Origin 'wrapper_started_native_session' -ExitCode $completionRun.exit_code -RunnerError $completionRun.error
        Write-ProbeCompletionCheckpoint -Complete $false
    }
    Assert-ProbeInputsUnchanged $completionInputs (Get-ProbeInputs -Paths $completionPaths)
    Write-ProbeCompletionCheckpoint -Complete $true
    Copy-Item -LiteralPath $completionCheckpoint -Destination $completionCanonical -Force
    if ((Get-FileHash -LiteralPath $completionCanonical -Algorithm SHA256).Hash -ne (Get-FileHash -LiteralPath $completionCheckpoint -Algorithm SHA256).Hash) { throw 'Aggregate publication hash mismatch.' }
    $completionPublished = $true
    Write-Host "Completed $($completionDone.Count) actions in $($completionSources.Count) native source sessions. Aggregate: $completionCanonical"
} finally {
    if (-not $completionPublished) {
        if ($completionHadCanonical) {
            if ((Get-FileHash -LiteralPath $completionBackup -Algorithm SHA256).Hash -ne $completionOriginalHash) { throw 'Original receipt backup changed; refusing unverified restoration.' }
            Copy-Item -LiteralPath $completionBackup -Destination $completionCanonical -Force
            if ((Get-FileHash -LiteralPath $completionCanonical -Algorithm SHA256).Hash -ne $completionOriginalHash) { throw 'Original receipt restoration hash mismatch.' }
        } elseif (Test-Path -LiteralPath $completionCanonical -PathType Leaf) {
            Move-Item -LiteralPath $completionCanonical -Destination (Join-Path $completionBackupDir 'last_native_working_copy.json')
        }
    }
}
