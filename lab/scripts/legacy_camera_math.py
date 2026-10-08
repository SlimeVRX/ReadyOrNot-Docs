"""Offline camera-format migration math; contains no game curves.

Legacy semantics are from Epic's CameraAnimInst::ApplyToView and
UInterpTrackMove::GetKeyTransformAtTime (UE4 archived source). Rotation order
matches UE5 Core/Private/Math/UnrealMath.cpp. Units: cm, degrees, seconds.
"""
import bisect
import math


def normalize(q):
    n = math.sqrt(sum(x*x for x in q))
    return tuple(x/n for x in q)


def quat(e):
    sr, sp, sy = [math.sin(math.radians(x % 360)*0.5) for x in e]
    cr, cp, cy = [math.cos(math.radians(x % 360)*0.5) for x in e]
    return normalize((cr*sp*sy-sr*cp*cy, -cr*sp*cy-sr*cp*sy,
                      cr*cp*sy-sr*sp*cy, cr*cp*cy+sr*sp*sy))


def multiply(a, b):
    x,y,z,w = a
    X,Y,Z,W = b
    return normalize((w*X+x*W+y*Z-z*Y, w*Y-x*Z+y*W+z*X,
                      w*Z+x*Y-y*X+z*W, w*W-x*X-y*Y-z*Z))


def inverse(q):
    return (-q[0], -q[1], -q[2], q[3])


def rotate(q, v):
    x,y,z,w = q
    tx,ty,tz = 2*(y*v[2]-z*v[1]), 2*(z*v[0]-x*v[2]), 2*(x*v[1]-y*v[0])
    return [v[0]+w*tx+y*tz-z*ty, v[1]+w*ty+z*tx-x*tz, v[2]+w*tz+x*ty-y*tx]


def slerp(a, b, alpha):
    dot = sum(x*y for x,y in zip(a,b))
    if dot < 0:
        b = tuple(-x for x in b)
        dot = -dot
    if dot > 0.9999:
        return normalize(tuple(x+(y-x)*alpha for x,y in zip(a,b)))
    angle = math.acos(min(1.0, dot))
    scale = math.sin(angle)
    return normalize(tuple((x*math.sin((1-alpha)*angle)+y*math.sin(alpha*angle))/scale for x,y in zip(a,b)))


def euler(q):
    x,y,z,w = q
    singular = z*x-w*y
    yaw = math.degrees(math.atan2(2*(w*z+x*y), 1-2*(y*y+z*z)))
    if singular < -0.4999995:
        pitch, roll = -90.0, -yaw-2*math.degrees(math.atan2(x,w))
    elif singular > 0.4999995:
        pitch, roll = 90.0, yaw-2*math.degrees(math.atan2(x,w))
    else:
        pitch = math.degrees(math.asin(max(-1.0,min(1.0,2*singular))))
        roll = math.degrees(math.atan2(-2*(w*x+y*z), 1-2*(x*x+y*y)))
    return [(roll+180)%360-180, pitch, yaw]


def angular_error(a,b):
    # Relative quaternion atan2 is more stable than acos(dot) near identity.
    d = multiply(inverse(a), b)
    return math.degrees(2*math.atan2(math.sqrt(sum(x*x for x in d[:3])), abs(d[3])))


def interval(points, t):
    return max(0, min(len(points)-2, bisect.bisect_right([p['InVal'] for p in points], t)-1))


def curve(points, t):
    if t <= points[0]['InVal']:
        return points[0]['OutVal']
    if t >= points[-1]['InVal']:
        return points[-1]['OutVal']
    i = interval(points,t)
    a,b = points[i:i+2]
    dt = b['InVal']-a['InVal']
    u = (t-a['InVal'])/dt
    mode = a['InterpMode']
    if mode == 'CIM_Constant':
        return a['OutVal']
    if mode == 'CIM_Linear':
        return [x+(y-x)*u for x,y in zip(a['OutVal'],b['OutVal'])]
    if mode not in ('CIM_CurveAuto','CIM_CurveAutoClamped','CIM_CurveUser','CIM_CurveBreak'):
        raise ValueError('Unsupported interpolation '+str(mode))
    return [(2*u**3-3*u*u+1)*a['OutVal'][axis]+(u**3-2*u*u+u)*dt*a.get('LeaveTangent',[0]*3)[axis]
            +(-2*u**3+3*u*u)*b['OutVal'][axis]+(u**3-u*u)*dt*b.get('ArriveTangent',[0]*3)[axis] for axis in range(3)]


def authored_rotation(points, t, use_quat):
    if not use_quat:
        return quat(curve(points,t))
    if t <= points[0]['InVal']:
        return quat(points[0]['OutVal'])
    if t >= points[-1]['InVal']:
        return quat(points[-1]['OutVal'])
    i = interval(points,t)
    a,b = points[i:i+2]
    return slerp(quat(a['OutVal']),quat(b['OutVal']),(t-a['InVal'])/(b['InVal']-a['InVal']))


def prepare(camera):
    movement = camera['movement']
    if movement.get('SubTracks') or movement.get('bDisableMovement') or movement.get('RotMode', 'IMR_Keyframed') != 'IMR_Keyframed':
        raise ValueError('Unsupported move-track mode/subtracks')
    if any(p.get('GroupName','None') != 'None' for p in movement.get('LookupTrack',{}).get('Points',[])):
        raise ValueError('External Matinee lookup-group dependency')
    pp,rp = movement['PosTrack']['Points'],movement['EulerTrack']['Points']
    use_quat = bool(movement.get('bUseQuatInterpolation',False))
    relative = camera['camera'].get('bRelativeToInitialTransform',True)
    # Verified UCameraAnim constructor defaults, rather than inferring length
    # from the last key (tracks may extend past the animation's playback range).
    duration = camera['camera'].get('AnimLength',3.0)
    p0 = curve(pp,0) if relative else [0,0,0]
    qi = inverse(authored_rotation(rp,0,use_quat)) if relative else (0,0,0,1)
    positions = []
    for p in pp:
        v = dict(p)
        v['OutVal'] = rotate(qi,[x-y for x,y in zip(p['OutVal'],p0)])
        v['ArriveTangent'] = rotate(qi,p.get('ArriveTangent',[0]*3))
        v['LeaveTangent'] = rotate(qi,p.get('LeaveTangent',[0]*3))
        positions.append(v)
    def rotation(t):
        return multiply(qi,authored_rotation(rp,t,use_quat))
    knots = sorted(set([0.0,duration]+[p['InVal'] for p in rp if 0 < p['InVal'] < duration]))
    values = {t: rotation(t) for t in knots}
    tolerance = 0.002  # degrees; later independent readback sampling must pass
    def subdivide(a,b,depth=0):
        if depth > 20:
            raise ValueError('Could not bound sampled Euler-to-quaternion migration error')
        probes = [a+(b-a)*f for f in (0.25,0.5,0.75)]
        error = max(angular_error(rotation(t),slerp(values[a],values[b],(t-a)/(b-a))) for t in probes)
        if error > tolerance or (not use_quat and b-a > 1/240):
            mid = (a+b)*0.5
            values[mid] = rotation(mid)
            subdivide(a,mid,depth+1)
            subdivide(mid,b,depth+1)
    for a,b in zip(knots,knots[1:]):
        subdivide(a,b)
    rotations = [{'InVal':t,'OutVal':euler(q),'InterpMode':'CIM_Linear'} for t,q in sorted(values.items())]
    return positions, rotations, duration, rotation, {'relative_to_initial_transform':relative,
        'legacy_quaternion_interpolation':use_quat, 'rotation_conversion':'transformed authored quaternion keys' if use_quat else 'adaptive quaternion samples of authored Euler polynomial',
        'adaptive_probe_tolerance_degrees':tolerance,'legacy_default_duration_seconds':3.0}
