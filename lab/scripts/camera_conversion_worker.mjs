// Isolated numerical migration/validation. Authored data stays in Saved/GunLab.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import { prepare, curve, angular_error, authored_rotation } from './legacy_camera_math.mjs';

const read = file => JSON.parse(fs.readFileSync(file, 'utf8').replace(/^\uFEFF/, ''));
const write = (file, value) => fs.writeFileSync(file, JSON.stringify(value, null, 2));
const writeAtomic = (file, value) => {
  const temporary=file+'.'+process.pid+'.tmp';
  write(temporary,value);fs.renameSync(temporary,file);
};
const requireCheck = (condition, message) => { if (!condition) throw Error(message); };
const norm = value => Math.sqrt(value.reduce((sum, x) => sum + x*x, 0));
const difference = (a,b) => norm(a.map((x,i) => x-b[i]));
const close = (a,b) => Math.abs(a-b) <= Math.max(1e-25, 1e-6*Math.max(Math.abs(a),Math.abs(b)));
const roundEven = value => { const lo=Math.floor(value), fraction=value-lo; return fraction<0.5?lo:fraction>0.5?lo+1:lo%2===0?lo:lo+1; };
function interpolation(mode) {
  if (mode==='CIM_Linear') return 'linear';
  if (mode==='CIM_Constant') return 'constant';
  if (['CIM_CurveAuto','CIM_CurveAutoClamped','CIM_CurveUser','CIM_CurveBreak'].includes(mode)) return 'cubic';
  throw Error('Unsupported interpolation '+mode);
}

function prepareAll(sourcePath, contentPath, planDir, fingerprint, generatedUtc) {
  const source=read(sourcePath);
  fs.mkdirSync(planDir,{recursive:true});
  const index={source_path:sourcePath,fingerprint,generated_utc:generatedUtc,numeric_runtime:'Node '+process.version,cameras:[]};
  let sampleCount=0;
  for (const camera of source.cameras) {
    requireCheck(camera.extra_tracks.length===0,'Additional legacy tracks need another converter');
    const disk=path.join(contentPath,camera.package.replace(/^\/Game\//,'')+'.uasset');
    requireCheck(crypto.createHash('sha256').update(fs.readFileSync(disk)).digest('hex')===camera.sha256,'Source changed after extraction');
    const [positions,rotations,end,,semantics]=prepare(camera), tracks=[positions,rotations];
    requireCheck(end>0,'Invalid duration');
    const allEnd=Math.max(end,...tracks.map(track=>track.at(-1).InVal));
    const rate=Math.min(2**29,2**Math.floor(Math.log2((2**31-1024)/allEnd)));
    requireCheck(rate>0,'Unsupported duration');
    const channels=[];
    let maxTimeError=0;
    for (const points of tracks) for(let axis=0;axis<3;axis++) {
      const keys=[];
      for (const point of points) {
        const tick=roundEven(point.InVal*rate);
        requireCheck(keys.length===0 || tick>keys.at(-1)[0],'Time quantization merges keys');
        maxTimeError=Math.max(maxTimeError,Math.abs(tick/rate-point.InVal));
        keys.push([tick,point.OutVal[axis],(point.ArriveTangent??[0,0,0])[axis]/rate,
                   (point.LeaveTangent??[0,0,0])[axis]/rate,interpolation(point.InterpMode)]);
      }
      channels.push(keys);
    }
    const samples=new Set([0,end]);
    for(let i=0;i<=Math.ceil(end*960);i++) if(i/960<=end) samples.add(i/960);
    for(let i=0;i<channels[3].length-1;i++) {
      const a=channels[3][i][0]/rate,b=channels[3][i+1][0]/rate;
      for(const f of [0.125,0.375,0.625,0.875]) samples.add(a+(b-a)*f);
    }
    const sampleTimes=[...samples].sort((a,b)=>a-b);
    const nativeTimes=[...new Set(sampleTimes.filter(t=>t<end-0.0001).map(Math.fround))].sort((a,b)=>a-b);
    const prefix=path.join(planDir,camera.sha256);
    write(prefix+'.plan.json',{camera,tracks,end,rate,semantics,channels,max_time_error:maxTimeError,sample_times:sampleTimes,native_times:nativeTimes});
    write(prefix+'.channels.json',{channels}); write(prefix+'.times.json',{times:nativeTimes});
    index.cameras.push({source:camera.package,source_sha256:camera.sha256,prefix,end,rate,asset_name:'CA_'+camera.package.split('/').at(-1)+'_'+camera.sha256.slice(0,8)});
    sampleCount+=nativeTimes.length;
  }
  write(path.join(planDir,'index.json'),index);
  console.log(JSON.stringify({prepared:index.cameras.length,native_samples:sampleCount,numeric_runtime:index.numeric_runtime}));
}

export function verifyOne(prefix) {
  const plan=read(prefix+'.plan.json'), channelReply=read(prefix+'.readback.json'), poseReply=read(prefix+'.poses.json');
  requireCheck(!channelReply.error,'Native channel bridge: '+channelReply.error);
  requireCheck(!poseReply.error,'Native evaluator bridge: '+poseReply.error);
  requireCheck(JSON.stringify(channelReply.defaults)===JSON.stringify([0,0,0,0,0,0,1,1,1]),'Transform defaults mismatch');
  const readback=channelReply.channels;
  requireCheck(readback.length===9 && readback.slice(6).every(keys=>keys.length===0),'Unexpected channels/scale keys');
  for(let axis=0;axis<6;axis++) {
    const actual=readback[axis],expected=plan.channels[axis];
    requireCheck(actual.length===expected.length,'Key count mismatch');
    for(let i=0;i<actual.length;i++) {
      const key=actual[i],[tick,value,arrive,leave,mode]=expected[i];
      requireCheck(key[0]===tick && key[1]===value,'Key timestamp/value mismatch');
      requireCheck(key[4]===mode && key[5]===0 && key[6]===2,'Interpolation/tangent mode mismatch');
      if(mode==='cubic') requireCheck(close(key[2],arrive) && close(key[3],leave),'Cubic tangent mismatch');
    }
  }
  const [positions,,end,referenceRotation,semantics]=prepare(plan.camera), stored=[];
  for(let group=0;group<2;group++) {
    stored.push(readback[group*3].map((key,i)=>({InVal:key[0]/plan.rate,
      OutVal:[0,1,2].map(a=>readback[group*3+a][i][1]),ArriveTangent:[0,1,2].map(a=>readback[group*3+a][i][2]*plan.rate),
      LeaveTangent:[0,1,2].map(a=>readback[group*3+a][i][3]*plan.rate),InterpMode:plan.tracks[group][i].InterpMode})));
  }
  let maxPositionError=0,maxRotationError=0;
  for(const t of plan.sample_times) {
    maxPositionError=Math.max(maxPositionError,difference(curve(stored[0],t),curve(positions,t)));
    maxRotationError=Math.max(maxRotationError,angular_error(authored_rotation(stored[1],t,true),referenceRotation(t)));
  }
  requireCheck(maxPositionError<0.001 && maxRotationError<0.005,'Readback pose exceeds original tolerances');
  requireCheck(poseReply.poses.length===plan.native_times.length,'Native sample count mismatch');
  let pError=0,qError=0,maxTranslation=0,maxRotation=0;
  for(let i=0;i<plan.native_times.length;i++) {
    const t=plan.native_times[i],pose=poseReply.poses[i];
    requireCheck(pose.length===7 && pose.every(Number.isFinite),'Invalid native pose');
    const p=pose.slice(0,3),q=pose.slice(3);
    maxTranslation=Math.max(maxTranslation,norm(p));maxRotation=Math.max(maxRotation,angular_error(q,[0,0,0,1]));
    pError=Math.max(pError,difference(p,curve(positions,t)));qError=Math.max(qError,angular_error(q,referenceRotation(t)));
  }
  requireCheck(pError<0.001 && qError<0.005,'Native pose exceeds original tolerances');
  const result={source:plan.camera.package,source_sha256:plan.camera.sha256,numeric_runtime:'Node '+process.version,
    keys:plan.channels.reduce((sum,keys)=>sum+keys.length,0),duration_seconds:end,
    duration_source:'AnimLength' in plan.camera.camera?'serialized AnimLength':'UE4 CameraAnim constructor default (3.0 seconds)',
    tick_rate:plan.rate,max_key_time_error_seconds:plan.max_time_error,quaternion_interpolation:true,semantics,
    key_value_and_used_cubic_tangent_readback:true,validation_samples:plan.sample_times.length,
    max_measured_position_error_cm:maxPositionError,max_measured_rotation_error_degrees:maxRotationError,
    native_evaluator_samples:plan.native_times.length,native_max_position_error_cm:pError,native_max_rotation_error_degrees:qError,
    native_max_translation_cm:maxTranslation,native_max_rotation_degrees:maxRotation,
    validation_method:'Isolated Node numeric checks of actual native bulk-channel readback and native sequence evaluator JSON; 960Hz plus independent intermediate probes; not an all-time bound or full camera-stack test.'};
  write(prefix+'.verified.json',result); return result;
}

function finish(indexPath,statusPath,privateReceipt,publicReceipt) {
  const index=read(indexPath),status=read(statusPath),source=read(index.source_path),sequences=[],paths=new Map();
  const rejected=[...source.failures,...status.rejected];
  for(const entry of status.accepted) {
    const result=read(entry.prefix+'.verified.json');
    requireCheck(result.numeric_runtime===index.numeric_runtime,'Mixed numeric runtimes in one conversion run');
    Object.assign(result,{sequence:entry.sequence,reused_validated_asset:entry.reused_validated_asset});
    sequences.push(result);paths.set(result.source,entry.sequence);
  }
  const mappings=[];
  for(const shake of source.shakes) {
    if(paths.has(shake.camera_package)) mappings.push({shake_class:shake.shake_class,sequence:paths.get(shake.camera_package),source:shake.camera_package});
    else rejected.push({shake_class:shake.shake_class,reason:'Camera curve was not converted'});
  }
  const failures=source.cameras.length-sequences.length;
  const common={schema:4,generated_utc:index.generated_utc,converter_fingerprint:index.fingerprint,numeric_runtime:index.numeric_runtime,
    validated_transform_semantics:mappings.length>0,conversion_failure_count:failures};
  const receipt={...common,sequences,mappings,rejected,scope:'Original authored motion, native format migration with numerical validation; no invented gunplay.'};
  const maximum=field=>sequences.length?Math.max(...sequences.map(s=>s[field])):null;
  const aggregate={...common,source_camera_count:source.cameras.length,source_parse_rejections:source.failures.length,
    converted_sequence_count:sequences.length,reused_sequence_count:sequences.filter(s=>s.reused_validated_asset).length,shake_mapping_count:mappings.length,
    native_evaluator_sample_count:sequences.reduce((sum,s)=>sum+s.native_evaluator_samples,0),
    max_native_position_error_cm:maximum('native_max_position_error_cm'),max_native_rotation_error_degrees:maximum('native_max_rotation_error_degrees'),
    max_key_time_error_seconds:maximum('max_key_time_error_seconds'),position_tolerance_cm:0.001,rotation_tolerance_degrees:0.005,
    method:'UE4 initial-relative transform, exact position basis conversion, adaptive authored Euler-to-quaternion migration; native bulk channel/readback/evaluator JSON with numeric validation in an isolated Node process.',
    limitations:'Measured 960Hz and independent intermediate samples, not bit-exact or all-time bound; malformed original camera excluded. Gameplay camera stack needs separate runtime verification.'};
  if(failures!==0 || sequences.length===0) {
    writeAtomic(privateReceipt+'.failed-attempt.json',{receipt,aggregate});
    throw Error('Camera conversion incomplete: '+failures+' parsed sources failed; last-good active/public receipts preserved');
  }
  // Only complete validated results become the active runtime map and public
  // aggregate. Failed attempts must never disable previously recovered cameras.
  writeAtomic(privateReceipt,receipt);writeAtomic(publicReceipt,aggregate);
  console.log(JSON.stringify({sequences:sequences.length,mappings:mappings.length,reused:aggregate.reused_sequence_count}));
}

const mode=process.argv[2],args=process.argv.slice(3);
if(mode==='prepare') prepareAll(...args);
else if(mode==='verify') verifyOne(...args);
else if(mode==='verify-all') {
  const results=read(args[0]).cameras.map(entry=>{
    try { return verifyOne(entry.prefix); }
    catch(error) { throw new Error(entry.source+': '+error.message,{cause:error}); }
  });
  console.log(JSON.stringify({verified:results.length,native_samples:results.reduce((sum,row)=>sum+row.native_evaluator_samples,0),
    max_position_error_cm:Math.max(...results.map(row=>row.native_max_position_error_cm)),max_rotation_error_degrees:Math.max(...results.map(row=>row.native_max_rotation_error_degrees)),numeric_runtime:'Node '+process.version}));
} else if(mode==='finish') finish(...args);
else if(mode) throw Error('Unknown worker mode '+mode);
