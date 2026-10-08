// Synthetic receipts only. These tests never create runtime evidence or run UE.
import assert from 'node:assert/strict';
import test from 'node:test';
import { createGunLabReport } from './report_gunlab_results.mjs';

const prefix = '/Game/Blueprints/Items/WeaponsRevised/';
const classPath = name => `${prefix}${name}.${name}_C`;
function fixture() {
  const paths = [classPath('Primary_SR16'), classPath('Primary_S590_Beanbag_V2'),
    '/Game/Blueprints/Items/WeaponsSuspect/Secondary_Flaregun.Secondary_Flaregun_C',
    classPath('Launcher_M320_Bang'), classPath('Launcher_M320')];
  const selection = paths.map((value, index) => ({ index: index + 1, name: `Synthetic ${index + 1}`, class_path: value }));
  const provenance = { map: 'ReadyOrNot_GunLab', candidate_count: paths.length,
    tested_binary_sha256: '1'.repeat(64), tested_host_game_binary_sha256: '2'.repeat(64),
    tested_map_sha256: '3'.repeat(64), tested_camera_mapping_sha256: '4'.repeat(64), collected_at_utc: '2026-01-01T00:00:00Z' };
  const equip = { ...provenance, mode: 'equip_audit', run_id_utc: 'synthetic-equip', results: paths.map((value, index) => ({
    index, class_path: value, status: index === 4 ? 'class_rejected' : 'equipped',
    detail: index === 4 ? 'Abstract base class' : 'Synthetic fixture',
    selection_route: index === 0 ? 'native_loadout' : 'catalog_asset',
    authored_ammunition_type_count: index === 0 ? 1 : 0
  })) };
  const probe = { ...provenance, mode: 'action_probe', run_id_utc: 'synthetic-headless', results: equip.results.slice(0, 4).flatMap(row => [
    { ...row }, { ...row, status: 'action_probe', ammo_before: 1, ammo_after_native_fire: 0, ammo: 1, magazines: 4,
      ammo_consumed: true, native_aiming_state_before_fire: true, native_reload_requested: true,
      native_can_reload_before_request: true, native_reload_replenished: true }
  ]) };
  const build = { build_exit_code: 0, plugin_dll_sha256: provenance.tested_binary_sha256, host_game_dll_sha256: provenance.tested_host_game_binary_sha256 };
  return { selection, equip, probe, build };
}

test('empty ammo rows do not classify native projectile launchers as incomplete', () => {
  const report = createGunLabReport(fixture());
  assert.equal(report.summary.groups.incomplete.total, 2);
  assert.equal(report.summary.groups.catalog.total, 1);
  assert.equal(report.summary.groups.native.total, 1);
  assert.equal(report.summary.groups.abstract.total, 1);
  assert.equal(report.summary.completeCoverage, true);
  assert.match(report.markdown, /Native GrenadeLauncher dùng cấu hình projectile/);
  // Even observed ammo consumption in the flare prototype is not certification.
  assert.equal(report.summary.groups.incomplete.fired, 2);
});

test('rendered subset preserves full observations and reports differing reload separately', () => {
  const source = fixture();
  source.rendered = { ...structuredClone(source.probe), run_id_utc: 'synthetic-capture', runner_mode: 'Capture',
    renderer_enabled: true, audio_disabled_by_command_line: true,
    results: structuredClone(source.probe.results.filter(row => row.index === 0)) };
  const headless = source.probe.results.find(row => row.index === 0 && row.status === 'action_probe');
  headless.ammo = 0; headless.native_reload_replenished = false;
  const original = JSON.stringify(source.probe);
  const report = createGunLabReport(source);
  assert.equal(JSON.stringify(source.probe), original);
  assert.equal(report.summary.completedActions, 4);
  assert.equal(report.summary.renderedConfirmation.completedActions, 1);
  assert.equal(report.summary.renderedConfirmation.headlessNoReloadRenderedReload, 1);
  for (const field of ['tested_binary_sha256', 'tested_host_game_binary_sha256', 'tested_map_sha256', 'tested_camera_mapping_sha256']) {
    const bad = structuredClone(source); bad.rendered[field] = 'f'.repeat(64);
    assert.throws(() => createGunLabReport(bad), new RegExp(field));
  }
});

test('class mismatches, duplicate observations, stale builds and contradictory ammo reject', () => {
  const wrong = fixture(); wrong.probe.results[0].class_path = classPath('wrong');
  assert.throws(() => createGunLabReport(wrong), /class mismatch/);
  const duplicate = fixture(); duplicate.probe.results.push({ ...duplicate.probe.results[1] });
  assert.throws(() => createGunLabReport(duplicate), /duplicate/);
  const stale = fixture(); stale.build.plugin_dll_sha256 = 'a'.repeat(64);
  assert.throws(() => createGunLabReport(stale), /fingerprint/);
  const contradictory = fixture(); contradictory.probe.results[1].ammo_consumed = false;
  assert.throws(() => createGunLabReport(contradictory), /inconsistent ammo_consumed/);
});

test('missing action is displayed as missing and reduces coverage', () => {
  const source = fixture(); source.probe.results = source.probe.results.filter(row => row.index !== 0 || row.status !== 'action_probe');
  const report = createGunLabReport(source);
  assert.equal(report.summary.completeCoverage, false);
  assert.equal(report.summary.completedActions, 3);
  assert.equal(report.summary.groups.native.fired, 0);
  assert.match(report.markdown, /Chưa có hàng action_probe/);
});

function multipartFixture() {
  const source = fixture(), original = source.probe;
  const ids = ['20260101-010101', '20260101-010202'];
  source.probe = { ...original, run_id_utc: 'multipart-synthetic', multipart: true, complete: true,
    world_reset_between_runs: true, requested_indices_one_based: [1, 2, 3, 4], completed_indices_zero_based: [0, 1, 2, 3],
    results: original.results.map(row => ({ ...row, source_run_id_utc: ids[row.index < 2 ? 0 : 1] })) };
  source.probe.source_runs = ids.map((id, part) => ({ run_id_utc: id, receipt_file: `action_probe_${id}.json`, receipt_sha256: '5'.repeat(64),
    source_kind: part === 0 ? 'provided_native_resume_receipt' : 'wrapper_started_native_session', process_exit_code: part === 0 ? null : 0,
    completed_indices_zero_based: part === 0 ? [0, 1] : [2, 3], requested_indices_one_based: part === 0 ? [1, 2, 3, 4] : [3, 4],
    incomplete_for_requested_subset: part === 0, accepted_row_count: 4, source_row_count: 4,
    tested_binary_sha256: original.tested_binary_sha256, tested_host_game_binary_sha256: original.tested_host_game_binary_sha256,
    tested_map_sha256: original.tested_map_sha256, tested_camera_mapping_sha256: original.tested_camera_mapping_sha256 }));
  return source;
}

test('multipart report identifies separate worlds and retains rendered comparisons', () => {
  const source = multipartFixture();
  source.rendered = { ...fixture().probe, run_id_utc: '20260101-020202', runner_mode: 'Capture', renderer_enabled: true,
    audio_disabled_by_command_line: true, results: fixture().probe.results.filter(row => row.index === 0) };
  const report = createGunLabReport(source);
  assert.equal(report.summary.sourceRunCount, 2);
  assert.equal(report.summary.worldResetBetweenRuns, true);
  assert.equal(report.summary.completedActions, 4);
  assert.equal(report.summary.renderedConfirmation.completedActions, 1);
  assert.match(report.markdown, /không phải một lần chơi liên tục/);
  assert.match(report.markdown, /Chưa ghi trong receipt nguồn/);
});

test('multipart membership, completeness, explicit process failure and input drift reject', () => {
  const missing = multipartFixture(); delete missing.probe.results[0].source_run_id_utc;
  assert.throws(() => createGunLabReport(missing), /source row counts|source_run_id/);
  const drift = multipartFixture(); drift.probe.source_runs[1].tested_map_sha256 = 'a'.repeat(64);
  assert.throws(() => createGunLabReport(drift), /source input mismatch/);
  const failed = multipartFixture(); failed.probe.source_runs[1].process_exit_code = 1;
  assert.throws(() => createGunLabReport(failed), /did not exit zero/);
  const partial = multipartFixture(); partial.probe.complete = false;
  assert.throws(() => createGunLabReport(partial), /completed aggregate/);
  const noReset = multipartFixture(); noReset.probe.world_reset_between_runs = false;
  assert.throws(() => createGunLabReport(noReset), /world-reset/);
  const changedIndex = multipartFixture(); changedIndex.probe.source_runs[0].completed_indices_zero_based = [0, 2];
  assert.throws(() => createGunLabReport(changedIndex), /unrelated|membership/);
});
