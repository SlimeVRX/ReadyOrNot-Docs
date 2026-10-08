"""Preflight authored camera migration without launching Unreal."""
import json
import os
import time
from legacy_camera_math import prepare, angular_error, quat

saved = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../../Ready Or Not/Saved/GunLab'))
data = json.load(open(os.path.join(saved,'legacy-camera-tracks.json'),encoding='utf-8'))
result=[]
started=time.time()
for camera in data['cameras']:
    try:
        p,r,d,f,s=prepare(camera)
        result.append({'source':camera['package'],'keys':len(r),'baseline_degrees':angular_error(f(0),(0,0,0,1)),
                       'roundtrip':max(angular_error(quat(k['OutVal']),f(k['InVal'])) for k in r)})
    except Exception as ex:
        result.append({'source':camera['package'],'error':str(ex)})
with open(os.path.join(saved,'camera-math-preflight.json'),'w',encoding='utf-8') as handle:
    json.dump(result,handle,indent=2)
print(json.dumps({'cameras':len(result),'seconds':time.time()-started,'failures':[x for x in result if 'error' in x],
                  'max_roundtrip_degrees':max(x.get('roundtrip',0) for x in result)}))
