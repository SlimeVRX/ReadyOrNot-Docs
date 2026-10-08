param(
    [int[]]$WeaponIndices = @(1,5,6,9,33,34,42,43,48,49,69,70,71,72,78,85),
    [string]$Workspace = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
)
$ErrorActionPreference = 'Stop'
if (Get-Process -Name UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue) {
    throw 'Close Unreal before starting a separate rendered control session.'
}
$saved = Join-Path $Workspace 'Ready Or Not\Saved\GunLab'
$canonical = Join-Path $saved 'action_probe_receipt.json'
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss-fff'
$backup = Join-Path $saved ('action_probe_before_rendered_' + $stamp + '.json')
$confirmation = Join-Path $saved 'rendered_confirmation_receipt.json'
$hadCanonical = Test-Path -LiteralPath $canonical -PathType Leaf
if ($hadCanonical) {
    $canonicalHash = (Get-FileHash -LiteralPath $canonical -Algorithm SHA256).Hash
    Copy-Item -LiteralPath $canonical -Destination $backup
    if ((Get-FileHash -LiteralPath $backup -Algorithm SHA256).Hash -ne $canonicalHash) { throw 'Canonical action receipt backup mismatch; Unreal was not launched.' }
}
if (Test-Path -LiteralPath $confirmation -PathType Leaf) {
    Move-Item -LiteralPath $confirmation -Destination (Join-Path $saved ('rendered_confirmation_before_' + $stamp + '.json'))
}
try {
    & (Join-Path $PSScriptRoot 'Run-GunLab.ps1') -Mode Capture -WeaponIndices $WeaponIndices -Workspace $Workspace
    $data = Get-Content -LiteralPath $canonical -Raw -Encoding UTF8 | ConvertFrom-Json
    # These flags describe this runner; native observations remain unchanged.
    $data | Add-Member -NotePropertyName runner_mode -NotePropertyValue 'Capture' -Force
    $data | Add-Member -NotePropertyName renderer_enabled -NotePropertyValue $true -Force
    $data | Add-Member -NotePropertyName audio_disabled_by_command_line -NotePropertyValue $true -Force
    $data | ConvertTo-Json -Depth 50 | Set-Content -LiteralPath $confirmation -Encoding UTF8
    Write-Host "Rendered control recorded separately: $confirmation"
} finally {
    if ($hadCanonical) {
        # Copy preserves the original receipt timestamp and therefore freshness checks.
        Copy-Item -LiteralPath $backup -Destination $canonical -Force
        if ((Get-FileHash -LiteralPath $canonical -Algorithm SHA256).Hash -ne $canonicalHash) { throw 'Canonical action receipt restoration mismatch; inspect the retained backup.' }
    } elseif (Test-Path -LiteralPath $canonical -PathType Leaf) {
        # Capture already retains action_probe_<run_id>.json; preserve its working copy too.
        Move-Item -LiteralPath $canonical -Destination $backup
    }
}
