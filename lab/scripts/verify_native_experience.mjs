#!/usr/bin/env node
// Outcome verification for the original Gun Lab input driver. Does not run Unreal.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import { fileURLToPath } from 'node:url';

export function verifyNativeExperience(receipt) {
  const checks = [];
  const observations = Array.isArray(receipt?.observations) ? receipt.observations : [];
  const byStep = new Map();
  const duplicates = [];
  for (const row of observations) {
    if (byStep.has(row?.step)) duplicates.push(row.step);
    byStep.set(row?.step, row);
  }
  const number = value => typeof value === 'number' && Number.isFinite(value);
  const text = value => typeof value === 'string' && value.length > 0;
  const equal = (a, b) => number(a) && number(b) && Math.abs(a - b) < 0.01;
  const record = (id, status, expectation, evidence) => checks.push({ id, status, expectation, evidence });
  const check = (id, expectation, fields, predicate) => {
    const missing = fields.filter(([step, field]) => !byStep.has(step) || !Object.hasOwn(byStep.get(step) ?? {}, field));
    if (missing.length) return record(id, 'missing', expectation, { missing: missing.map(([step, field]) => `${step}.${field}`) });
    const values = fields.map(([step, field]) => byStep.get(step)[field]);
    const evidence = Object.fromEntries(fields.map(([step, field], index) => [`${step}.${field}`, values[index]]));
    let passed = false;
    try { passed = predicate(...values) === true; } catch { passed = false; }
    record(id, passed ? 'passed' : 'failed', expectation, evidence);
  };
  const at = (step, ...fields) => fields.map(field => [step, field]);
  record('sequence_complete', receipt?.complete === true && receipt?.mode === 'native_experience_input_probe' ? 'passed' : 'failed',
    'The input probe completed and used the expected receipt mode.', { complete: receipt?.complete, mode: receipt?.mode });
  record('unique_observations', observations.length > 0 && duplicates.length === 0 ? 'passed' : 'failed',
    'Every observation step appears exactly once.', { count: observations.length, duplicates });
  record('monotonic_observation_times', observations.length > 0 && observations.every((row, i) => number(row.time) && (i === 0 || row.time >= observations[i - 1].time)) ? 'passed' : 'failed',
    'Recorded observation times are finite and do not go backwards.', {});
  const fatalSteps = observations.filter(row => /timeout|selection_rejected|class_missing|target_missing/.test(row?.step ?? '')).map(row => row.step);
  record('no_probe_failure_steps', fatalSteps.length ? 'failed' : 'passed', 'No startup/equip/target/renderer timeout was recorded.', { fatal_steps: fatalSteps });

  check('native_primary', 'The initial SR16 is in the native primary slot with a live FP animation instance.',
    at('baseline', 'weapon', 'native_primary_slot', 'fp_anim_instance'), (weapon, primary, anim) => weapon.endsWith('Primary_SR16.Primary_SR16_C') && primary === true && text(anim));
  check('fire_selector', 'A short native selector press changes the held SR16 mode without changing weapon or ammo.',
    [...at('baseline', 'weapon', 'fire_mode', 'ammo'), ...at('fire_selector_after_short_press', 'weapon', 'fire_mode', 'ammo')],
    (w0, m0, a0, w1, m1, a1) => w0 === w1 && text(m0) && text(m1) && m0 !== m1 && equal(a0, a1));
  check('ads_state_and_fov', 'Native ADS becomes active and narrows FOV relative to the initial hipfire view.',
    [...at('baseline', 'ads', 'camera_fov'), ...at('ads', 'ads', 'camera_fov')],
    (off, fov0, on, fov1) => off === false && on === true && number(fov0) && number(fov1) && fov1 > 0 && fov1 < fov0);
  check('canted_state', 'Canted aiming activates through the bound native action.', at('canted', 'ads', 'canted'), (ads, canted) => ads === true && canted === true);
  check('fire_ammo_and_native_events', 'The initial firing action consumes ammunition and emits matching local/server native fire events.',
    [...at('canted', 'ammo', 'local_fire_events', 'server_fire_events'), ...at('fire_and_recoil', 'ammo', 'local_fire_events', 'server_fire_events')],
    (a0, l0, s0, a1, l1, s1) => [a0, l0, s0, a1, l1, s1].every(number) && a0 > a1 && equal(a0 - a1, l1 - l0) && equal(a0 - a1, s1 - s0));
  check('native_recoil_state', 'A positive native PendingRecoil response was sampled while firing.', at('fire_and_recoil', 'max_pending_recoil_observed'), max => number(max) && max > 0);
  check('tactical_reload', 'The same SR16 replenishes ammunition through tactical reload while retaining its magazine count.',
    [...at('fire_and_recoil', 'weapon', 'ammo', 'magazines'), ...at('tactical_reload', 'weapon', 'ammo', 'magazines', 'weapon_tactical_reload', 'weapon_reloading')],
    (w0, a0, m0, w1, a1, m1, tactical, reloading) => w0 === w1 && number(a0) && number(a1) && a1 > a0 && equal(m0, m1) && tactical === true && reloading === false);
  check('native_magcheck', 'Held R plays the native magazine-check montage without consuming/replenishing ammunition.',
    [...at('tactical_reload', 'ammo'), ...at('held_magcheck', 'ammo', 'magcheck_playing', 'active_montage')],
    (a0, a1, active, montage) => equal(a0, a1) && active === true && text(montage) && /mag_check/i.test(montage));
  check('low_ready', 'The native low-ready action activates.', at('low_ready', 'low_ready'), value => value === true);
  check('quick_lean', 'The native lean axis produces nonzero lean.', at('lean_right', 'lean'), value => number(value) && Math.abs(value) > 0);
  check('crouch', 'The player reaches native crouch state.', at('crouch', 'crouched'), value => value === true);
  check('free_look', 'The native free-look action activates.', at('free_look', 'freelook'), value => value === true);
  check('native_secondary_slot', 'The secondary key selects a different weapon in the real secondary slot.',
    [...at('baseline', 'weapon'), ...at('secondary_slot', 'weapon', 'native_secondary_slot', 'native_primary_slot')],
    (primary, secondary, secondarySlot, primarySlot) => text(secondary) && primary !== secondary && secondarySlot === true && primarySlot === false);
  check('primary_slot_preserves_state', 'Returning to the primary preserves its ammo, reserve magazine count and ammunition row across the native 1/2 swap.',
    [...at('free_look', 'weapon', 'ammo', 'magazines', 'ammo_type'), ...at('primary_slot_restored', 'weapon', 'ammo', 'magazines', 'ammo_type', 'native_primary_slot')],
    (w0, a0, m0, t0, w1, a1, m1, t1, primary) => w0 === w1 && equal(a0, a1) && equal(m0, m1) && t0 === t1 && primary === true);
  check('optic_component_added', 'A native attachment component is added to the same gun while its ammo is retained.',
    [...at('primary_slot_restored', 'weapon', 'ammo', 'attachments'), ...at('optic_attached', 'weapon', 'ammo', 'attachments')],
    (w0, a0, oldItems, w1, a1, items) => w0 === w1 && equal(a0, a1) && Array.isArray(oldItems) && Array.isArray(items) && items.some(item => text(item) && !oldItems.includes(item)));
  check('optic_ads', 'The configured optic remains on the same weapon during ADS with a narrower FOV.',
    [...at('optic_attached', 'weapon', 'attachments', 'camera_fov'), ...at('optic_ads', 'weapon', 'attachments', 'ads', 'camera_fov')],
    (w0, a0, f0, w1, a1, ads, f1) => w0 === w1 && Array.isArray(a0) && a0.length > 0 && Array.isArray(a1) && a0.every(value => a1.includes(value)) && ads === true && number(f0) && number(f1) && f1 < f0);
  check('native_target_damage', 'Live native fire consumes ammo, emits fire events and reduces health on the recorded native target.',
    [...at('native_target_before', 'target', 'target_health', 'ammo', 'server_fire_events'), ...at('native_target_after_live_fire', 'target', 'target_health', 'ammo', 'server_fire_events')],
    (t0, h0, a0, e0, t1, h1, a1, e1) => text(t0) && t0 === t1 && [h0, a0, e0, h1, a1, e1].every(number) && h1 < h0 && a1 < a0 && e1 > e0);

  check('standing_movement', 'Forward input moves the standing player.', at('standing_movement', 'speed', 'crouched'), (speed, crouched) => number(speed) && speed > 1 && crouched === false);
  check('shift_native_slow_walk', 'With the same gun, Shift activates bHoldingFastWalk but this snapshot scales speed to approximately 40% of ordinary standing movement, without sprint.',
    [...at('standing_movement', 'weapon', 'speed'), ...at('fast_walk_movement', 'weapon', 'speed', 'holding_fast_walk', 'sprinting')],
    (w0, speed0, w1, speed1, holding, sprinting) => w0 === w1 && number(speed0) && number(speed1) && speed0 > 1 && speed1 > 1 && Math.abs(speed1 / speed0 - 0.4) < 0.05 && holding === true && sprinting === false);
  check('crouched_movement', 'The player moves while crouched, at a lower speed than standing under the same loadout.',
    [...at('standing_movement', 'weapon', 'speed'), ...at('crouched_movement', 'weapon', 'speed', 'crouched')],
    (w0, s0, w1, s1, crouched) => w0 === w1 && number(s0) && number(s1) && s1 > 1 && s1 < s0 && crouched === true);
  check('free_lean_axis', 'Free lean is active and its native X/Z offset changes.', at('free_lean_axis', 'freelean_active', 'freelean_x', 'freelean_z'),
    (active, x, z) => active === true && number(x) && number(z) && Math.abs(x) + Math.abs(z) > 0.1);

  for (const [prefix, mode, expected] of [['single', 'FM_Single', 1], ['burst', 'FM_Burst', 3], ['auto', 'FM_Auto', null]]) {
    check(`mp5a2_${prefix}_held_fire`, `MP5A2 ${mode} emits ${expected ?? 'more than 3'} shots during the held-input sample, with matching ammo/event deltas.`,
      [...at(`${prefix}_before_hold`, 'weapon', 'fire_mode', 'ammo', 'server_fire_events', 'authored_burst_bullet_count'), ...at(`${prefix}_after_hold`, 'weapon', 'fire_mode', 'ammo', 'server_fire_events')],
      (w0, m0, a0, e0, burst, w1, m1, a1, e1) => {
        if (!text(w0) || !w0.endsWith('Primary_MP5A2.Primary_MP5A2_C') || w0 !== w1 || m0 !== mode || m1 !== mode || ![a0, a1, e0, e1].every(number)) return false;
        const shots = a0 - a1;
        return equal(shots, e1 - e0) && (expected === null ? shots > 3 : equal(shots, expected)) && (prefix !== 'burst' || equal(burst, 3));
      });
  }
  check('empty_reload', 'An actually empty MP5A2 reloads through native input; ammunition rises and one magazine is consumed.',
    [...at('before_empty_reload', 'weapon', 'ammo', 'magazines'), ...at('after_empty_reload', 'weapon', 'ammo', 'magazines', 'weapon_reloading')],
    (w0, a0, m0, w1, a1, m1, loading) => w0 === w1 && equal(a0, 0) && number(a1) && a1 > 0 && equal(m0 - m1, 1) && loading === false);
  check('speed_reload', 'The double-click reload branch replenishes the same partially loaded MP5A2 and consumes one reserve magazine.',
    [...at('before_speed_reload', 'weapon', 'ammo', 'magazines'), ...at('after_speed_reload', 'weapon', 'ammo', 'magazines', 'weapon_reloading', 'weapon_tactical_reload')],
    (w0, a0, m0, w1, a1, m1, loading, tactical) => w0 === w1 && number(a0) && number(a1) && a0 > 0 && a1 > a0 && equal(m0 - m1, 1) && loading === false && tactical === false);

  // Presentation/fixture checks deliberately remain missing until their actual
  // observation fields exist. A missing field is never interpreted as success.
  check('native_crosshair_present', 'The native reticle widget exists in the live viewport; this does not certify layout/size/usability.',
    at('baseline', 'native_crosshair_overlay_present'), present => present === true);
  check('light_toggle', 'The installed native light changes from off to on on the same weapon after the input action.',
    [...at('light_before_toggle', 'weapon', 'light_on'), ...at('attachment_toggle', 'weapon', 'light_on')],
    (w0, off, w1, on) => text(w0) && w0 === w1 && off === false && on === true);
  check('native_sr16_camera_oscillation', 'The SR16 authored oscillator shake becomes active while firing; this weapon does not reference a CameraAnim track.',
    at('fire_and_recoil', 'max_active_shakes', 'max_camera_oscillation_time_remaining'),
    (active, remaining) => number(active) && active > 0 && number(remaining) && remaining > 0);
  check('camera_recovery_fire_input', 'The authored-camera test fires the native M16A4 and consumes ammunition.',
    [...at('m16a4_equipped', 'weapon', 'ammo', 'server_fire_events'), ...at('recovered_camera_fire', 'weapon', 'ammo', 'server_fire_events')],
    (w0, a0, e0, w1, a1, e1) => w0?.endsWith('Primary_M16A4.Primary_M16A4_C') && w0 === w1 && [a0,a1,e0,e1].every(number) && a1 < a0 && e1 > e0);
  check('camera_recovery_playback', 'The M16A4 authored camera sequence is evaluated after firing and produces a nonzero sampled transform.',
    at('recovered_camera_fire', 'camera_recovery_track_count', 'camera_recovery_max_active_sequences', 'camera_recovery_max_playback_seconds', 'camera_recovery_max_translation_cm', 'camera_recovery_max_rotation_deg'),
    (tracks, active, time, position, rotation) => [tracks, active, time, position, rotation].every(number) && tracks > 0 && active > 0 && time > 0 && (position > 0.0001 || rotation > 0.0001));
  check('native_training_success', 'Live native fire consumes ammunition and causes the original training-target success delegate to fire.',
    [...at('training_target_before', 'weapon', 'training_success_events', 'ammo', 'server_fire_events'), ...at('training_target_after_live_fire', 'weapon', 'training_success_events', 'ammo', 'server_fire_events')],
    (w0, t0, a0, e0, w1, t1, a1, e1) => text(w0) && w0 === w1 && [t0, a0, e0, t1, a1, e1].every(number) && t1 > t0 && a1 < a0 && e1 > e0);
  check('native_glass_hit_forwarding', 'A live weapon shot reaches the same native glass actor and increases its native hit-event count.',
    [...at('native_glass_before', 'glass_actor_id', 'glass_native_hit_events', 'ammo', 'server_fire_events'), ...at('native_glass_after_live_fire', 'glass_actor_id', 'glass_native_hit_events', 'ammo', 'server_fire_events')],
    (g0, h0, a0, e0, g1, h1, a1, e1) => text(g0) && g0 === g1 && [h0, a0, e0, h1, a1, e1].every(number) && h1 > h0 && a1 < a0 && e1 > e0);
  check('native_glass_fracture', 'After a native hit, procedural fragment components increase on the same glass fixture; forwarding alone is not fracture.',
    [...at('native_glass_before', 'glass_actor_id', 'glass_procedural_components'), ...at('native_glass_after_live_fire', 'glass_actor_id', 'glass_procedural_components')],
    (g0, c0, g1, c1) => text(g0) && g0 === g1 && number(c0) && number(c1) && c1 > c0);

  const counts = { passed: 0, failed: 0, missing: 0 };
  for (const check of checks) ++counts[check.status];
  return {
    schema: 1,
    run_id_utc: receipt?.run_id_utc ?? null,
    map: receipt?.map ?? null,
    status: counts.failed ? 'failed' : counts.missing ? 'incomplete' : 'passed',
    counts,
    checks,
    not_certified: [
      'Subjective gunfeel or parity with a retail release.',
      'Audio heard correctly; audio_disabled_by_command_line=false only means the command-line flag was absent.',
      'Crosshair presentation quality from widget presence alone.',
      receipt?.native_loadout_ui_opened_by_probe
        ? 'Native loadout UI interaction: opening and rendering were exercised, but choosing items and normal Back/save navigation were not.'
        : 'Native loadout UI interaction: this run did not open it.',
      'Every weapon/attachment/ammunition combination, multiplayer, all door/material/armor or less-lethal behavior.'
    ],
    audio_disabled_by_command_line: receipt?.audio_disabled_by_command_line ?? null,
    native_loadout_ui_opened_by_probe: receipt?.native_loadout_ui_opened_by_probe ?? null
  };
}

function main() {
  const args = process.argv.slice(2);
  const receiptPath = args.shift();
  if (!receiptPath || receiptPath.startsWith('--')) throw new Error('Usage: node verify_native_experience.mjs <experience_receipt.json> [--output report.json] [--allow-missing]');
  let outputPath;
  let allowMissing = false;
  while (args.length) {
    const arg = args.shift();
    if (arg === '--output') { outputPath = args.shift(); if (!outputPath || outputPath.startsWith('--')) throw new Error('--output requires a file path'); }
    else if (arg === '--allow-missing') allowMissing = true;
    else throw new Error(`Unknown argument: ${arg}`);
  }
  const raw = fs.readFileSync(receiptPath);
  const report = verifyNativeExperience(JSON.parse(raw.toString('utf8').replace(/^\uFEFF/, '')));
  report.receipt_sha256 = crypto.createHash('sha256').update(raw).digest('hex');
  report.verified_at_utc = new Date().toISOString();
  if (outputPath) fs.writeFileSync(outputPath, `${JSON.stringify(report, null, 2)}\n`);
  console.log(JSON.stringify({ run_id_utc: report.run_id_utc, status: report.status, counts: report.counts,
    nonpassing: report.checks.filter(check => check.status !== 'passed').map(({ id, status }) => ({ id, status })), output: outputPath ?? null }, null, 2));
  if (report.counts.failed || (!allowMissing && report.counts.missing)) process.exitCode = 1;
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  try { main(); } catch (error) { console.error(error.message); process.exitCode = 2; }
}
