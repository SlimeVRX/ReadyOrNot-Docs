// Offline camera-format migration math; contains no authored game curves.
// Port of legacy_camera_math.py. UE4 CameraAnimInst::ApplyToView and
// UInterpTrackMove::GetKeyTransformAtTime semantics; UE rotation order.
// Units: centimeters, degrees, seconds. No Unreal or Python runtime dependency.

const radians = degrees => degrees * (Math.PI / 180);
const degrees = radiansValue => radiansValue * (180 / Math.PI);
const positive_modulo = (value, divisor) => {
  const remainder = value % divisor;
  return remainder < 0 ? remainder + divisor : remainder === 0 ? 0 : remainder;
};
const truthy = value => {
  if (value == null || value === false || value === 0 || value === '') return false;
  if (Array.isArray(value)) return value.length !== 0;
  if (typeof value === 'object') return Object.keys(value).length !== 0;
  return true;
};

export function normalize(q) {
  const n = Math.sqrt(q.reduce((sum, x) => sum + x * x, 0));
  if (n === 0) throw new Error('Cannot normalize a zero quaternion');
  return q.map(x => x / n);
}

export function quat(e) {
  const [sr, sp, sy] = e.map(x => Math.sin(radians(positive_modulo(x, 360)) * 0.5));
  const [cr, cp, cy] = e.map(x => Math.cos(radians(positive_modulo(x, 360)) * 0.5));
  return normalize([cr * sp * sy - sr * cp * cy, -cr * sp * cy - sr * cp * sy,
    cr * cp * sy - sr * sp * cy, cr * cp * cy + sr * sp * sy]);
}

export function multiply(a, b) {
  const [x, y, z, w] = a, [X, Y, Z, W] = b;
  return normalize([w * X + x * W + y * Z - z * Y, w * Y - x * Z + y * W + z * X,
    w * Z + x * Y - y * X + z * W, w * W - x * X - y * Y - z * Z]);
}

export function inverse(q) { return [-q[0], -q[1], -q[2], q[3]]; }

export function rotate(q, v) {
  const [x, y, z, w] = q;
  const tx = 2 * (y * v[2] - z * v[1]), ty = 2 * (z * v[0] - x * v[2]), tz = 2 * (x * v[1] - y * v[0]);
  return [v[0] + w * tx + y * tz - z * ty, v[1] + w * ty + z * tx - x * tz, v[2] + w * tz + x * ty - y * tx];
}

export function slerp(a, b, alpha) {
  let dot = a.reduce((sum, x, i) => sum + x * b[i], 0);
  if (dot < 0) { b = b.map(x => -x); dot = -dot; }
  if (dot > 0.9999) return normalize(a.map((x, i) => x + (b[i] - x) * alpha));
  const angle = Math.acos(Math.min(1.0, dot)), scale = Math.sin(angle);
  return normalize(a.map((x, i) => (x * Math.sin((1 - alpha) * angle) + b[i] * Math.sin(alpha * angle)) / scale));
}

export function euler(q) {
  const [x, y, z, w] = q;
  const singular = z * x - w * y;
  const yaw = degrees(Math.atan2(2 * (w * z + x * y), 1 - 2 * (y * y + z * z)));
  let pitch, roll;
  if (singular < -0.4999995) { pitch = -90.0; roll = -yaw - 2 * degrees(Math.atan2(x, w)); }
  else if (singular > 0.4999995) { pitch = 90.0; roll = yaw - 2 * degrees(Math.atan2(x, w)); }
  else {
    pitch = degrees(Math.asin(Math.max(-1.0, Math.min(1.0, 2 * singular))));
    roll = degrees(Math.atan2(-2 * (w * x + y * z), 1 - 2 * (x * x + y * y)));
  }
  return [positive_modulo(roll + 180, 360) - 180, pitch, yaw];
}

export function angular_error(a, b) {
  // Relative quaternion atan2 remains stable near identity.
  const d = multiply(inverse(a), b);
  return degrees(2 * Math.atan2(Math.sqrt(d.slice(0, 3).reduce((sum, x) => sum + x * x, 0)), Math.abs(d[3])));
}

export function interval(points, t) {
  // bisect_right: a knot belongs to the interval beginning at that knot.
  let low = 0, high = points.length;
  while (low < high) {
    const mid = Math.floor((low + high) / 2);
    if (t < points[mid].InVal) high = mid; else low = mid + 1;
  }
  return Math.max(0, Math.min(points.length - 2, low - 1));
}

export function curve(points, t) {
  if (t <= points[0].InVal) return points[0].OutVal;
  if (t >= points.at(-1).InVal) return points.at(-1).OutVal;
  const i = interval(points, t), a = points[i], b = points[i + 1];
  const dt = b.InVal - a.InVal, u = (t - a.InVal) / dt, mode = a.InterpMode;
  if (mode === 'CIM_Constant') return a.OutVal;
  if (mode === 'CIM_Linear') return a.OutVal.map((x, axis) => x + (b.OutVal[axis] - x) * u);
  if (!['CIM_CurveAuto', 'CIM_CurveAutoClamped', 'CIM_CurveUser', 'CIM_CurveBreak'].includes(mode))
    throw new Error('Unsupported interpolation ' + String(mode));
  return [0, 1, 2].map(axis => (2 * u ** 3 - 3 * u * u + 1) * a.OutVal[axis]
    + (u ** 3 - 2 * u * u + u) * dt * (a.LeaveTangent ?? [0, 0, 0])[axis]
    + (-2 * u ** 3 + 3 * u * u) * b.OutVal[axis]
    + (u ** 3 - u * u) * dt * (b.ArriveTangent ?? [0, 0, 0])[axis]);
}

export function authored_rotation(points, t, use_quat) {
  if (!use_quat) return quat(curve(points, t));
  if (t <= points[0].InVal) return quat(points[0].OutVal);
  if (t >= points.at(-1).InVal) return quat(points.at(-1).OutVal);
  const i = interval(points, t), a = points[i], b = points[i + 1];
  return slerp(quat(a.OutVal), quat(b.OutVal), (t - a.InVal) / (b.InVal - a.InVal));
}

export function prepare(camera) {
  const movement = camera.movement;
  if (truthy(movement.SubTracks) || truthy(movement.bDisableMovement) || (movement.RotMode ?? 'IMR_Keyframed') !== 'IMR_Keyframed')
    throw new Error('Unsupported move-track mode/subtracks');
  if ((movement.LookupTrack?.Points ?? []).some(p => (p.GroupName ?? 'None') !== 'None'))
    throw new Error('External Matinee lookup-group dependency');
  const pp = movement.PosTrack.Points, rp = movement.EulerTrack.Points;
  const use_quat = truthy(movement.bUseQuatInterpolation ?? false);
  const relative = camera.camera.bRelativeToInitialTransform ?? true;
  const duration = camera.camera.AnimLength ?? 3.0;
  const p0 = relative ? curve(pp, 0) : [0, 0, 0];
  const qi = relative ? inverse(authored_rotation(rp, 0, use_quat)) : [0, 0, 0, 1];
  const positions = pp.map(p => ({ ...p,
    OutVal: rotate(qi, p.OutVal.map((x, i) => x - p0[i])),
    ArriveTangent: rotate(qi, p.ArriveTangent ?? [0, 0, 0]),
    LeaveTangent: rotate(qi, p.LeaveTangent ?? [0, 0, 0]) }));
  const rotation = t => multiply(qi, authored_rotation(rp, t, use_quat));
  const knots = [...new Set([0.0, duration, ...rp.filter(p => p.InVal > 0 && p.InVal < duration).map(p => p.InVal)])].sort((a, b) => a - b);
  const values = new Map(knots.map(t => [t, rotation(t)]));
  const tolerance = 0.002;
  function subdivide(a, b, depth = 0) {
    if (depth > 20) throw new Error('Could not bound sampled Euler-to-quaternion migration error');
    const probes = [0.25, 0.5, 0.75].map(f => a + (b - a) * f);
    const error = Math.max(...probes.map(t => angular_error(rotation(t), slerp(values.get(a), values.get(b), (t - a) / (b - a)))));
    if (error > tolerance || (!use_quat && b - a > 1 / 240)) {
      const mid = (a + b) * 0.5;
      values.set(mid, rotation(mid));
      subdivide(a, mid, depth + 1); subdivide(mid, b, depth + 1);
    }
  }
  for (let i = 0; i < knots.length - 1; ++i) subdivide(knots[i], knots[i + 1]);
  const rotations = [...values.entries()].sort(([a], [b]) => a - b).map(([t, q]) => ({ InVal: t, OutVal: euler(q), InterpMode: 'CIM_Linear' }));
  return [positions, rotations, duration, rotation, {
    relative_to_initial_transform: relative,
    legacy_quaternion_interpolation: use_quat,
    rotation_conversion: use_quat ? 'transformed authored quaternion keys' : 'adaptive quaternion samples of authored Euler polynomial',
    adaptive_probe_tolerance_degrees: tolerance,
    legacy_default_duration_seconds: 3.0
  }];
}
