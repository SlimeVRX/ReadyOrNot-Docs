import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import { fileURLToPath } from 'node:url';
import { verifyNativeExperience } from './verify_native_experience.mjs';
const repo = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');
const saved = process.argv[2] || path.resolve(repo, '../Ready Or Not/Saved/GunLab');
const out = path.join(repo,'lab/manifests');
const dll = path.resolve(saved,'../../Plugins/ReadyOrNotGunLab/Binaries/Win64/UnrealEditor-ReadyOrNotGunLab.dll');
const gameDll = path.resolve(saved,'../../Binaries/Win64/UnrealEditor-ReadyOrNot.dll');
const map = path.resolve(saved,'../../Content/ReadyOrNot/Level/Study/ReadyOrNot_GunLab.umap');
const cameraFile = path.join(saved, 'legacy-camera-recovery-map.json');
const hash = filename => crypto.createHash('sha256').update(fs.readFileSync(filename)).digest('hex').toUpperCase();
const build = JSON.parse(fs.readFileSync(path.join(out,'build_receipt.json'),'utf8').replace(/^\uFEFF/,''));
const binaryHash = hash(dll);
const gameHash = hash(gameDll);
if (binaryHash !== build.plugin_dll_sha256) throw new Error('Local DLL does not match build_receipt.json; record the verified build before collecting runtime evidence.');
if (gameHash !== build.host_game_dll_sha256) throw new Error('Host gameplay DLL does not match the verified build receipt.');
fs.mkdirSync(out,{recursive:true});
for (const name of ['generation_receipt.json','coverage_receipt.json','equip_audit_receipt.json','action_probe_receipt.json','rendered_confirmation_receipt.json','native_upgrade_receipt.json','procedural_glass_restore_receipt.json','native_upgrade_pipeline.json','experience_receipt.json','experience_verification.json']) {
  const input = path.join(saved,name);
  if (!fs.existsSync(input)) continue;
  let data = JSON.parse(fs.readFileSync(input,'utf8').replace(/^\uFEFF/,''));
  const runtime = !!data.results || name.startsWith('experience_');
  if (runtime) {
    const cameraChangedAt = fs.existsSync(cameraFile) ? fs.statSync(cameraFile).mtimeMs : 0;
    if (fs.statSync(input).mtimeMs < Math.max(fs.statSync(dll).mtimeMs, fs.statSync(gameDll).mtimeMs, fs.statSync(map).mtimeMs, cameraChangedAt)) throw new Error(`${name} predates the current DLLs, map or camera mapping; rerun before collecting it as current evidence.`);
    // The public observation file gains provenance fields below. Verify those
    // exact published bytes so its report hash remains directly reproducible.
    if (name === 'experience_verification.json') {
      const published = path.join(out, 'experience_receipt.json');
      data = verifyNativeExperience(JSON.parse(fs.readFileSync(published, 'utf8')));
      data.receipt_sha256 = hash(published).toLowerCase();
      data.verified_at_utc = new Date().toISOString();
    }
    data.source_receipt_sha256 = hash(input);
    data.tested_binary_sha256 = binaryHash;
    data.tested_host_game_binary_sha256 = gameHash;
    data.tested_map_sha256 = hash(map);
    if (fs.existsSync(cameraFile)) data.tested_camera_mapping_sha256 = hash(cameraFile);
    data.build_receipt = 'build_receipt.json';
    data.collected_at_utc = new Date().toISOString();
  }
  // Local backups are useful on the user's machine, not as published file links.
  if (data.backup) data.backup = path.basename(data.backup.replaceAll('\\', '/'));
  fs.writeFileSync(path.join(out,name),JSON.stringify(data,null,2)+'\n');
  if (data.results) {
    const counts = {};
    for (const result of data.results) counts[result.status]=(counts[result.status]||0)+1;
    console.log(name, JSON.stringify({rows:data.results.length,candidates:data.candidate_count,counts}));
  }
}
const profileFile = path.join(saved, 'profile_preservation.json');
if (fs.existsSync(profileFile)) {
  const profile = JSON.parse(fs.readFileSync(profileFile, 'utf8').replace(/^\uFEFF/, ''));
  const summary = {
    schema: 1, started_at_utc: profile.started_at_utc, finished_at_utc: profile.finished_at_utc,
    status: profile.status, before_exists: profile.before_exists, restored_exists: profile.restored_exists,
    exact_bytes_restored: profile.status === 'restored' && profile.before_exists === profile.restored_exists && profile.before_sha256 === profile.restored_sha256,
    process_changed_profile_bytes: profile.before_exists !== profile.after_process_exists || profile.before_sha256 !== profile.after_process_sha256,
    scope: 'Optional native UI inspection only. Profile contents, identifying hashes and backup paths stay local; this does not describe earlier unbacked runs.'
  };
  fs.writeFileSync(path.join(out, 'profile_preservation_summary.json'), JSON.stringify(summary, null, 2) + '\n');
}
if (fs.existsSync(cameraFile)) {
  const camera = JSON.parse(fs.readFileSync(cameraFile, 'utf8'));
  const summary = {
    schema: 1, generated_utc: camera.generated_utc,
    converter_fingerprint: camera.converter_fingerprint,
    conversion_failure_count: camera.conversion_failure_count,
    validated_transform_semantics: camera.validated_transform_semantics,
    accepted_sequences: camera.sequences.length, enabled_shake_mappings: camera.mappings.length,
    rejected_entries: camera.rejected,
    mapping_manifest_sha256: hash(cameraFile),
    scope: 'Metadata only. Authored curves, generated assets and full sequence measurements remain local. Runtime playback is tested separately in experience_receipt.json.'
  };
  fs.writeFileSync(path.join(out, 'camera_recovery_summary.json'), JSON.stringify(summary, null, 2) + '\n');
}
