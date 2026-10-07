// Export only references and authored numeric/text metadata, never asset bytes.
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
const repo = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');
const saved = process.argv[2] || path.resolve(repo, '../Ready Or Not/Saved/GunLab');
const data = JSON.parse(fs.readFileSync(path.join(saved, 'native_inspection.json'), 'utf8').replace(/^\uFEFF/, ''));
const coverage = JSON.parse(fs.readFileSync(path.join(saved, 'coverage_receipt.json'), 'utf8').replace(/^\uFEFF/, ''));
const group = p => p.includes('/WeaponsRevised/Primary_') ? 0 : p.includes('/WeaponsRevised/Secondary_') ? 1 : p.includes('/WeaponsRevised/') ? 2 : p.includes('/WeaponsSuspect/') ? 3 : 4;
const order = [...data.weapons].sort((a,b) => group(a.asset_path)-group(b.asset_path) || (a.asset_path < b.asset_path ? -1 : a.asset_path > b.asset_path ? 1 : 0));
const labIndices = new Map(order.map((item,i) => [item.class_path, i+1]));
const result = {
  schema: 1,
  engine: data.engine,
  scope: 'NativeParentClass registry scan across /Game, with loaded BaseMagazineWeapon Blueprint CDOs. Includes player, suspect, variant, nonlethal, launcher and performance-test entries. Candidate status does not certify playable status.',
  game_asset_count: data.game_asset_count,
  asset_class_counts: data.asset_class_counts,
  registry_candidate_count: data.registry_candidate_count,
  exclusions: ['AMeleeWeapon hierarchy is melee, not a firearm.', 'Native base/abstract C++ classes are not authored Blueprint gun entries.', 'Runtime rejects abstract/deprecated or missing animation/mesh and records the reason.'],
  other_weapon_blueprints: coverage.other_weapon_blueprints,
  weapons: data.weapons.map(item => ({...item, asset_name: item.asset_path.split('/').at(-1), lab_index: labIndices.get(item.class_path)}))
};
fs.mkdirSync(path.join(repo,'06-Catalogs'), {recursive:true});
fs.writeFileSync(path.join(repo,'06-Catalogs/weapons.json'), JSON.stringify(result,null,2)+'\n');
fs.mkdirSync(path.join(repo,'lab/manifests'), {recursive:true});
fs.writeFileSync(path.join(repo,'lab/manifests/selection_order.json'), JSON.stringify(order.map((item,i)=>({index:i+1,class_path:item.class_path,name:item.values.item_name})),null,2)+'\n');
console.log(`Exported ${result.weapons.length} CDO entries; ${result.game_asset_count} /Game registry assets.`);
