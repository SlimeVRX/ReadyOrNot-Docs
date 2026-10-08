// Read-only extraction of authored UE4 CameraAnim tagged curves; no game asset is rewritten.
// Curve output is proprietary study data and MUST remain outside the documentation repository.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';

const workspace = path.resolve(process.argv[2] || 'D:/Zone9Dev_RON');
const content = path.join(workspace, 'Ready Or Not/Content');
const output = path.join(workspace, 'Ready Or Not/Saved/GunLab/legacy-camera-tracks.json');
const versionsText = fs.readFileSync(path.join(workspace, 'Engine/Source/Runtime/Core/Public/UObject/ObjectVersion.h'), 'utf8');
const versionBlock = versionsText.slice(versionsText.indexOf('VER_UE4_OLDEST_LOADABLE_PACKAGE'), versionsText.indexOf('VER_UE4_AUTOMATIC_VERSION_PLUS_ONE'));
const versions = {};
let value = 213;
for (const match of versionBlock.replace(/\/\/[^\n]*/g, '').matchAll(/\b((?:VER|VAR)_UE4_\w+)\s*(?:=\s*(\d+))?\s*,/g)) {
  value = match[2] ? Number(match[2]) : value + 1;
  versions[match[1]] = value;
}
function assert(ok, message) { if (!ok) throw new Error(message); }
function files(dir) { return fs.readdirSync(dir, { withFileTypes: true }).flatMap(e => e.isDirectory() ? files(path.join(dir, e.name)) : e.name.endsWith('.uasset') ? [path.join(dir, e.name)] : []); }

function readPackage(file) {
  const b = fs.readFileSync(file);
  let p = 0;
  const i32 = () => { assert(p + 4 <= b.length, 'read past end'); const n = b.readInt32LE(p); p += 4; return n; };
  const byte = () => b[p++];
  const str = () => { const n = i32(); assert(Math.abs(n) < 100000, 'invalid FString'); const count = n < 0 ? -n * 2 : n; const s = b.toString(n < 0 ? 'utf16le' : 'utf8', p, p + count).replace(/\0$/, ''); p += count; return s; };
  assert(i32() === -1641380927, 'invalid package tag');
  const legacy = i32();
  assert(legacy === -6 || legacy === -7, 'unsupported legacy version ' + legacy);
  i32();
  const ue4 = i32();
  assert(ue4 >= 504 && ue4 <= 522, 'unsupported UE4 version ' + ue4);
  i32();
  const customCount = i32(); assert(customCount >= 0 && customCount < 100, 'invalid custom versions'); p += customCount * 20;
  const totalHeader = i32(); str(); const flags = i32();
  const nameCount = i32(), nameOffset = i32();
  if (!(flags & 0x80000000) && ue4 >= versions.VER_UE4_ADDED_PACKAGE_SUMMARY_LOCALIZATION_ID) str();
  i32(); i32(); // Gatherable text data count / offset.
  const exportCount = i32(), exportOffset = i32(), importCount = i32(), importOffset = i32(), dependsOffset = i32();
  const names = []; p = nameOffset;
  for (let n = 0; n < nameCount; n++) { names.push(str()); p += 4; }
  const fname = () => { const idx = i32(), num = i32(); assert(idx >= 0 && idx < names.length && num >= 0 && num < 100000, 'invalid FName at ' + (p - 8)); return names[idx] + (num ? '_' + (num - 1) : ''); };
  const imports = []; p = importOffset;
  for (let n = 0; n < importCount; n++) {
    const item = { package: fname(), kind: fname(), outer: i32(), name: fname() };
    if (!(flags & 0x80000000) && ue4 >= versions.VER_UE4_NON_OUTER_PACKAGE_IMPORT) item.packageName = fname();
    imports.push(item);
  }
  const exports = [];
  const stride = (dependsOffset - exportOffset) / exportCount;
  assert(Number.isInteger(stride) && stride >= 64 && stride <= 120, 'unsupported export layout');
  for (let n = 0; n < exportCount; n++) {
    p = exportOffset + stride * n;
    const cls = i32(); i32();
    if (ue4 >= versions.VER_UE4_TemplateIndex_IN_COOKED_EXPORTS) i32();
    const outer = i32(), name = fname(); i32();
    let size, offset;
    if (ue4 >= versions.VER_UE4_64BIT_EXPORTMAP_SERIALSIZES) { size = Number(b.readBigInt64LE(p)); p += 8; offset = Number(b.readBigInt64LE(p)); p += 8; }
    else { size = i32(); offset = i32(); }
    assert(offset >= totalHeader && offset + size <= b.length, 'invalid export data bounds');
    exports.push({ cls: cls < 0 ? imports[-cls - 1].name : cls, outer, name, size, offset });
  }
  const isCamera = exports.some(e => e.cls === 'CameraAnim');
  if (!isCamera && !imports.some(i => i.kind === 'CameraAnim')) return null;

  function tag() {
    const name = fname(); if (name === 'None') return { name };
    const type = fname(), size = i32(), arrayIndex = i32();
    assert(size >= 0 && size <= b.length && arrayIndex === 0, 'unsupported property size/index');
    let struct, inner, bool, enumName;
    if (type === 'StructProperty') { struct = fname(); p += 16; }
    else if (type === 'ArrayProperty') inner = fname();
    else if (type === 'ByteProperty' || type === 'EnumProperty') enumName = fname();
    else if (type === 'BoolProperty') bool = byte() !== 0;
    if (ue4 >= versions.VER_UE4_PROPERTY_GUID_IN_PROPERTY_TAG && byte()) p += 16;
    return { name, type, size, arrayIndex, struct, inner, bool, enumName, start: p };
  }
  function props(end) {
    const out = {};
    while (p < end) {
      const t = tag(); if (t.name === 'None') return out;
      const next = t.start + t.size; assert(next <= end, 'property exceeds parent: ' + t.name);
      let result;
      if (t.type === 'FloatProperty') result = b.readFloatLE(p);
      else if (t.type === 'BoolProperty') result = t.bool;
      else if (t.type === 'ObjectProperty' || t.type === 'IntProperty') result = b.readInt32LE(p);
      else if (t.type === 'NameProperty' || (t.type === 'ByteProperty' && t.size === 8)) result = fname();
      else if (t.type === 'ByteProperty') result = byte();
      else if (t.type === 'StructProperty') {
        if (t.struct === 'Vector' || t.struct === 'Rotator') result = [b.readFloatLE(p), b.readFloatLE(p + 4), b.readFloatLE(p + 8)];
        else if (['InterpCurveVector', 'InterpCurveFloat', 'InterpLookupTrack'].includes(t.struct)) result = props(next);
        else result = { unsupported_struct: t.struct, bytes: t.size };
      } else if (t.type === 'ArrayProperty') {
        const count = i32(); assert(count >= 0 && count < 100000, 'invalid array length');
        result = [];
        if (t.inner === 'StructProperty') {
          const innerTag = tag(); assert(innerTag.type === 'StructProperty', 'invalid array struct tag');
          for (let i = 0; i < count; i++) result.push(props(next));
          assert(p === next, 'array byte count mismatch');
        } else if (t.inner === 'ObjectProperty') for (let i = 0; i < count; i++) result.push(i32());
        else result = { unsupported_array: t.inner, count };
      } else result = { unsupported_type: t.type, bytes: t.size };
      out[t.name] = result; p = next;
    }
    throw new Error('missing property terminator');
  }
  for (const e of exports) { if (['CameraAnim', 'InterpGroupCamera', 'InterpTrackMove', 'InterpTrackFloatProp'].includes(e.cls) || (!isCamera && e.name.startsWith('Default__'))) { p = e.offset; e.properties = props(e.offset + e.size); } }
  const packageName = '/Game/' + path.relative(content, file).replaceAll('\\', '/').replace(/\.uasset$/, '');
  if (!isCamera) {
    const cdo = exports.find(e => e.name.startsWith('Default__') && e.properties?.Anim);
    if (!cdo) return null;
    const imp = imports[-cdo.properties.Anim - 1];
    assert(imp?.kind === 'CameraAnim' && imp.outer < 0, 'invalid legacy camera import');
    return { shake_class: packageName + '.' + path.basename(packageName) + '_C', camera_package: imports[-imp.outer - 1].name, properties: cdo.properties };
  }
  const camera = exports.find(e => e.cls === 'CameraAnim');
  const tracks = exports.filter(e => e.cls === 'InterpTrackMove');
  assert(tracks.length === 1, 'expected exactly one movement track');
  const movement = tracks[0].properties;
  const position = movement.PosTrack?.Points || [], rotation = movement.EulerTrack?.Points || [];
  assert(position.length && rotation.length, 'missing movement keys');
  for (const points of [position, rotation]) for (let i = 0; i < points.length; i++) {
    const k = points[i]; assert(Number.isFinite(k.InVal) && k.OutVal?.length === 3 && k.OutVal.every(Number.isFinite), 'invalid movement key');
    if (i) assert(k.InVal > points[i - 1].InVal, 'non-monotonic keys');
  }
  return { package: packageName, sha256: crypto.createHash('sha256').update(b).digest('hex'), ue4, camera: camera.properties, movement, extra_tracks: exports.filter(e => /^InterpTrack/.test(e.cls) && e.cls !== 'InterpTrackMove') };
}
const cameras = [], shakes = [], failures = [];
for (const file of files(path.join(content, 'Blueprints/Camera'))) {
  try { const result = readPackage(file); if (result?.shake_class) shakes.push(result); else if (result) cameras.push(result); }
  catch (error) { failures.push({ path: path.relative(content, file).replaceAll('\\', '/'), reason: error.message }); }
}
fs.mkdirSync(path.dirname(output), { recursive: true });
fs.writeFileSync(output, JSON.stringify({ schema: 1, generated_utc: new Date().toISOString(), scope: 'Private authored curve data; do not publish. Read-only extraction; unsupported schemas rejected.', cameras, shakes, failures }, null, 2));
console.log(JSON.stringify({ output, cameras: cameras.length, shakes: shakes.length, failures: failures.length, failure_examples: failures.slice(0, 10), modes: [...new Set(cameras.flatMap(c => [...c.movement.PosTrack.Points, ...c.movement.EulerTrack.Points].map(p => p.InterpMode)))] }, null, 2));
