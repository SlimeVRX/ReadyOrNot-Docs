import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
const repo = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');
const saved = process.argv[2] || path.resolve(repo, '../Ready Or Not/Saved/GunLab');
const out = path.join(repo,'lab/manifests');
fs.mkdirSync(out,{recursive:true});
for (const name of ['generation_receipt.json','coverage_receipt.json','equip_audit_receipt.json','action_probe_receipt.json']) {
  const input = path.join(saved,name);
  if (!fs.existsSync(input)) continue;
  const data = JSON.parse(fs.readFileSync(input,'utf8').replace(/^\uFEFF/,''));
  fs.writeFileSync(path.join(out,name),JSON.stringify(data,null,2)+'\n');
  if (data.results) {
    const counts = {};
    for (const result of data.results) counts[result.status]=(counts[result.status]||0)+1;
    console.log(name, JSON.stringify({rows:data.results.length,candidates:data.candidate_count,counts}));
  }
}
