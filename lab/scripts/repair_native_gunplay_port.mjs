// Restore native gunplay port paths and avoid loadout work during world teardown.
// Exact-match guards and local backups avoid editing unrelated game source.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';

const workspace = path.resolve(process.argv[2] || 'D:/Zone9Dev_RON');
const root = path.join(workspace, 'Ready Or Not');
const hash = text => crypto.createHash('sha256').update(text).digest('hex');
const stamp = new Date().toISOString().replaceAll(/[:.]/g, '-');
const backupDir = path.join(root, 'Saved/Codex/NativeGunplayPort', stamp);
const plans = [];
function edit(relative, replacements) {
  const filename = path.join(root, relative);
  const before = fs.readFileSync(filename, 'utf8');
  const eol = before.includes('\r\n') ? '\r\n' : '\n';
  let after = before;
  for (const [oldTemplate, newTemplate] of replacements) {
    const oldText = oldTemplate.replace(/\r?\n/g, eol), newText = newTemplate.replace(/\r?\n/g, eol);
    if (after.includes(newText)) {
      if (after.split(newText).length !== 2) throw new Error('Expected exactly one repaired source block: ' + relative);
      continue;
    }
    if (after.split(oldText).length !== 2) throw new Error('Expected exactly one guarded source match: ' + relative + ' / ' + oldText);
    after = after.replace(oldText, newText);
  }
  plans.push({ relative, filename, before, after });
}
edit('Source/ReadyOrNot/Actors/BaseMagazineWeapon.cpp', [[
  'bool bShouldSpall = false;',
  'bool bShouldSpall = UKismetMathLibrary::RandomBoolWithWeightFromStream(FRandomStream(Seed + 64), HitSurfaceSpallingChance);'
]]);
edit('Source/ReadyOrNot/Actors/TrainingTarget.h', [['FHitResult HitInfo,', 'const FHitResult& HitInfo,']]);
edit('Source/ReadyOrNot/Actors/TrainingTarget.cpp', [
  ['//OnTakePointDamage.AddDynamic(this, &ATrainingTarget::OnPointDamage);', 'OnTakePointDamage.AddDynamic(this, &ATrainingTarget::OnPointDamage);'],
  ['//OnTakeRadialDamage.AddDynamic(this, &ATrainingTarget::OnRadialDamage);', 'OnTakeRadialDamage.AddDynamic(this, &ATrainingTarget::OnRadialDamage);'],
  ['FHitResult HitInfo,', 'const FHitResult& HitInfo,']
]);
edit('Source/ReadyOrNot/HUD/Widgets/Loadout/V2/Loadout_V2.cpp', [
  ['#include "HUD/Widgets/Loadout/V2/Loadout_V2.h"', '#include "HUD/Widgets/Loadout/V2/Loadout_V2.h"\n#include "CoreGlobals.h"\n#include "Engine/World.h"'],
  ['void ULoadout_V2::NativeDestruct()\n{\n\tSuper::NativeDestruct();\n\tExitLoadout();\n}',
   'void ULoadout_V2::NativeDestruct()\n{\n\tSuper::NativeDestruct();\n\t// Shutdown must not save presets, spawn equipment or create HUD widgets.\n\tUWorld* World = GetWorld();\n\tif (World && !World->bIsTearingDown && !IsEngineExitRequested())\n\t{\n\t\tExitLoadout();\n\t}\n}']
]);
fs.mkdirSync(backupDir, { recursive: true });
const receipt = { schema: 1, utc: new Date().toISOString(), scope: 'Existing weighted spall and training target damage delegates restored for UE5 signatures; native loadout teardown skips save/equip during world or engine shutdown. Normal loadout exit still saves. Compile/runtime validation is a separate step.', files: [] };
for (const plan of plans) {
  const backup = path.join(backupDir, path.basename(plan.relative));
  if (plan.after !== plan.before) {
    fs.copyFileSync(plan.filename, backup);
    const originalMode = fs.statSync(plan.filename).mode;
    try { fs.chmodSync(plan.filename, originalMode | 0o200); fs.writeFileSync(plan.filename, plan.after); }
    finally { fs.chmodSync(plan.filename, originalMode); }
  }
  receipt.files.push({ path: plan.relative, changed: plan.after !== plan.before, before_sha256: hash(plan.before), after_sha256: hash(plan.after), backup: plan.after !== plan.before ? backup : null });
}
const receiptFile = path.join(root, 'Saved/GunLab/native-port-repair.json');
fs.mkdirSync(path.dirname(receiptFile), { recursive: true });
fs.writeFileSync(receiptFile, JSON.stringify(receipt, null, 2));
console.log(JSON.stringify({ receipt: receiptFile, changed_files: receipt.files.filter(f => f.changed).length }));
