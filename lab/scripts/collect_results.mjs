import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import { fileURLToPath } from 'node:url';
const repo = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');
const saved = process.argv[2] || path.resolve(repo, '../Ready Or Not/Saved/GunLab');
const out = path.join(repo,'lab/manifests');
const dll = path.resolve(saved,'../../Plugins/ReadyOrNotGunLab/Binaries/Win64/UnrealEditor-ReadyOrNotGunLab.dll');
const build = JSON.parse(fs.readFileSync(path.join(out,'build_receipt.json'),'utf8').replace(/^\uFEFF/,''));
const binaryHash = crypto.createHash('sha256').update(fs.readFileSync(dll)).digest('hex').toUpperCase();
if (binaryHash !== build.plugin_dll_sha256) throw new Error('Local DLL does not match build_receipt.json; record the verified build before collecting runtime evidence.');
fs.mkdirSync(out,{recursive:true});
for (const name of ['generation_receipt.json','coverage_receipt.json','equip_audit_receipt.json','action_probe_receipt.json']) {
  const input = path.join(saved,name);
  if (!fs.existsSync(input)) continue;
  const data = JSON.parse(fs.readFileSync(input,'utf8').replace(/^\uFEFF/,''));
  if (data.results) {
    if (fs.statSync(input).mtimeMs < fs.statSync(dll).mtimeMs) throw new Error(`${name} predates the current DLL; rerun before collecting it as current evidence.`);
    data.tested_binary_sha256 = binaryHash;
    data.build_receipt = 'build_receipt.json';
    data.collected_at_utc = new Date().toISOString();
  }
  fs.writeFileSync(path.join(out,name),JSON.stringify(data,null,2)+'\n');
  if (data.results) {
    const counts = {};
    for (const result of data.results) counts[result.status]=(counts[result.status]||0)+1;
    console.log(name, JSON.stringify({rows:data.results.length,candidates:data.candidate_count,counts}));
  }
}
