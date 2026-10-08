"""Native camera asset broker; numerical validation runs in isolated Node.
All authored data stays under local Saved/GunLab; original assets are untouched.
"""
import atexit
import datetime
import faulthandler
import hashlib
import json
import os
import runpy
import shutil
import subprocess
import sys
import traceback
import unreal

scripts = os.path.dirname(os.path.abspath(__file__))
private_dir = os.path.join(os.path.abspath(unreal.Paths.project_saved_dir()), 'GunLab')
os.makedirs(private_dir, exist_ok=True)
generated_utc = datetime.datetime.now(datetime.timezone.utc).isoformat()
fault_handle = open(os.path.join(private_dir, 'camera-conversion-fault.txt'), 'w', encoding='utf-8', buffering=1)
fault_handle.write('run='+generated_utc+' python='+sys.version+'\n')
faulthandler.enable(file=fault_handle, all_threads=True)
stage_handle = open(os.path.join(private_dir, 'camera-conversion-stages.jsonl'), 'w', encoding='utf-8', buffering=1)

def close_diagnostics():
    faulthandler.disable()
    if not fault_handle.closed:
        fault_handle.close()
    if not stage_handle.closed:
        stage_handle.close()
atexit.register(close_diagnostics)

def stage(name, package=''):
    stage_handle.write(json.dumps({'run':generated_utc,'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
                                   'stage':name,'source':package})+'\n')
    stage_handle.flush()

def require(condition, message):
    if not condition:
        raise RuntimeError(message)

def read_json(path):
    with open(path, encoding='utf-8') as handle:
        return json.load(handle)

def read_text(path):
    with open(path, encoding='utf-8') as handle:
        return handle.read()

def write_text(path, value):
    with open(path, 'w', encoding='utf-8') as handle:
        handle.write(value)


def inspect_factory_tracks(binding, reuse):
    # CameraAnimationSequenceFactoryNew delegates to InitializeSpawnable, which
    # creates an unbounded, constant-true Bool spawn track beside our transform.
    # Validate that exact factory graph rather than rejecting it or ignoring
    # arbitrary extra tracks on a saved sequence.
    transforms = []
    spawns = []
    for track in binding.get_tracks():
        if isinstance(track, unreal.MovieScene3DTransformTrack):
            transforms.append(track)
        elif isinstance(track, unreal.MovieSceneSpawnTrack):
            spawns.append(track)
        else:
            raise RuntimeError('Unsupported camera binding track: '+track.get_class().get_path_name())
    require(len(spawns)==1, 'Expected exactly one factory spawn track')
    spawn_sections = spawns[0].get_sections()
    require(len(spawn_sections)==1 and isinstance(spawn_sections[0],unreal.MovieSceneBoolSection), 'Unexpected factory spawn section')
    spawn_section = spawn_sections[0]
    require(not spawn_section.has_start_frame() and not spawn_section.has_end_frame(), 'Factory spawn section must be unbounded')
    spawn_channels = spawn_section.get_channels_by_type(unreal.MovieSceneScriptingBoolChannel)
    require(len(spawn_channels)==1, 'Expected one factory bool spawn channel')
    require(spawn_channels[0].get_default() is True and len(spawn_channels[0].get_keys())==0, 'Factory spawn channel must be constant true')
    require(len(transforms)==(1 if reuse else 0), 'Unexpected camera transform track count')
    return transforms[0] if reuse else None

node_binary = os.environ.get('RON_GUNLAB_CAMERA_NODE') or shutil.which('node') or ''
worker = os.path.join(scripts, 'camera_conversion_worker.mjs')
require(os.path.isfile(node_binary), 'Install Node 18+ or set RON_GUNLAB_CAMERA_NODE (Upgrade -CameraNode) to its executable.')
version_result = subprocess.run([node_binary,'--version'], capture_output=True, text=True,
                                creationflags=subprocess.CREATE_NO_WINDOW)
require(version_result.returncode == 0, 'Could not inspect standalone Node runtime')
numeric_runtime_version = version_result.stdout.strip()
require(int(numeric_runtime_version.lstrip('v').split('.')[0]) >= 18, 'Camera numeric worker requires Node 18+.')

def run_worker(*arguments):
    # No shell or Unreal child. Numerics use an independent V8 runtime.
    completed = subprocess.run([node_binary, worker, *arguments], capture_output=True, text=True,
                               creationflags=subprocess.CREATE_NO_WINDOW)
    if completed.returncode != 0:
        raise RuntimeError('Isolated camera worker failed ('+str(completed.returncode)+'): '+completed.stderr+completed.stdout)
    return completed.stdout.strip()

method_hash = hashlib.sha256()
method_hash.update(numeric_runtime_version.encode('ascii'))
for filename in (__file__, worker, os.path.join(scripts,'legacy_camera_math.mjs'),
                 os.path.join(scripts,'..','plugin','ReadyOrNotGunLab','Source','ReadyOrNotGunLab','Private','RonGunLabCameraRecovery.cpp')):
    with open(filename,'rb') as handle:
        method_hash.update(handle.read())
fingerprint = method_hash.hexdigest()
plan_dir = os.path.join(private_dir,'CameraConversion','V_'+fingerprint[:12])
destination = '/Game/ReadyOrNot/Level/Study/CameraRecovery/V_'+fingerprint[:12]
stage('prepare_in_isolated_node')
run_worker('prepare',os.path.join(private_dir,'legacy-camera-tracks.json'),
           os.path.abspath(unreal.Paths.project_content_dir()),plan_dir,fingerprint,generated_utc)
index_path = os.path.join(plan_dir,'index.json')
index = read_json(index_path)
unreal.EditorAssetLibrary.make_directory(destination)
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
status = {'accepted':[],'rejected':[]}

for entry in index['cameras']:
    package = entry['source']
    phase = 'load_or_create_sequence'
    try:
        stage(phase,package)
        asset_path = destination+'/'+entry['asset_name']
        sequence = unreal.EditorAssetLibrary.load_asset(asset_path) if unreal.EditorAssetLibrary.does_asset_exist(asset_path) else None
        reuse = sequence is not None
        if reuse:
            require(unreal.EditorAssetLibrary.get_metadata_tag(sequence,'GunLab.SourceSHA256')==entry['source_sha256'], 'Existing source provenance mismatch')
            require(unreal.EditorAssetLibrary.get_metadata_tag(sequence,'GunLab.ConverterFingerprint')==fingerprint, 'Existing converter fingerprint mismatch')
        else:
            sequence = asset_tools.create_asset(entry['asset_name'],destination,unreal.CameraAnimationSequence,unreal.CameraAnimationSequenceFactoryNew())
        require(sequence is not None,'Camera sequence factory failed')
        rate,end = entry['rate'],entry['end']
        if not reuse:
            sequence.set_display_rate(unreal.FrameRate(30,1))
            sequence.set_tick_resolution_directly(unreal.FrameRate(rate,1))
            sequence.set_playback_start_seconds(0.0)
            sequence.set_playback_end_seconds(end)
        actual_rate = sequence.get_tick_resolution()
        require(actual_rate.numerator==rate and actual_rate.denominator==1,'Tick resolution mismatch')
        require(sequence.get_playback_start_seconds()==0.0,'Playback start mismatch')
        require(abs(sequence.get_playback_end_seconds()-end)<1e-6,'Playback end mismatch')
        bindings = sequence.get_bindings()
        require(len(bindings)==1,'Expected one native camera binding')
        existing_transform = inspect_factory_tracks(bindings[0],reuse)
        if reuse:
            sections = existing_transform.get_sections()
            require(len(sections)==1,'Unexpected existing sections')
            section = sections[0]
        else:
            transform = bindings[0].add_track(unreal.MovieScene3DTransformTrack)
            section = transform.add_section()
            section.set_start_frame_bounded(False)
            section.set_end_frame_bounded(False)
            section.set_editor_property('use_quaternion_interpolation',True)
        require(not section.has_start_frame() and not section.has_end_frame(),'Expected unbounded transform section')
        require(section.get_editor_property('use_quaternion_interpolation'),'Quaternion interpolation disabled')
        prefix = entry['prefix']
        phase = 'native_channel_json'
        stage(phase,package)
        write_text(prefix+'.readback.json', unreal.RonGunLabCameraValidation.author_and_read_camera_channels(
            section, read_text(prefix+'.channels.json'), not reuse))
        phase = 'native_evaluator_json'
        stage(phase,package)
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        write_text(prefix+'.poses.json',unreal.RonGunLabCameraValidation.sample_native_camera_sequence_json(
            world,sequence,read_text(prefix+'.times.json')))
        phase = 'verify_in_isolated_node'
        stage(phase,package)
        run_worker('verify',prefix)
        phase = 'save_validated_sequence'
        stage(phase,package)
        if not reuse:
            unreal.EditorAssetLibrary.set_metadata_tag(sequence,'GunLab.SourceSHA256',entry['source_sha256'])
            unreal.EditorAssetLibrary.set_metadata_tag(sequence,'GunLab.SourceCamera',package)
            unreal.EditorAssetLibrary.set_metadata_tag(sequence,'GunLab.ConverterFingerprint',fingerprint)
            require(unreal.EditorAssetLibrary.save_loaded_asset(sequence,False),'Could not save sequence')
        status['accepted'].append({'prefix':prefix,'sequence':asset_path+'.'+entry['asset_name'],'reused_validated_asset':reuse})
        unreal.log('GUNLAB_CAMERA_RECOVERED '+package)
        stage('completed',package)
    except Exception as exc:
        diagnostic = traceback.format_exc()
        status['rejected'].append({'source':package,'stage':phase,'exception_type':type(exc).__name__,'reason':str(exc),'traceback':diagnostic})
        unreal.log_warning('GUNLAB_CAMERA_REJECTED '+package+' stage='+phase+' '+diagnostic)

status_path = os.path.join(plan_dir,'status.json')
write_text(status_path,json.dumps(status,indent=2))
stage('finish_in_isolated_node')
summary = run_worker('finish',index_path,status_path,os.path.join(private_dir,'legacy-camera-recovery-map.json'),
                     os.path.abspath(os.path.join(scripts,'..','manifests','camera_migration_receipt.json')))
helper = os.path.join(scripts,'inspect_range_tables.py')
if os.path.isfile(helper):
    runpy.run_path(helper)
unreal.log('GUNLAB_CAMERA_RECOVERY_COMPLETE '+summary)
stage('pipeline_complete')
close_diagnostics()
