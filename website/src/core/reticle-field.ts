import { samplePulses, type PulseFrame } from './pulses.ts';

export const LOOP_SECONDS = 24;
export interface ReticleFrame {
  phase: number; intro: number; pointerX: number; pointerY: number; strength: number; waves: PulseFrame;
}
const wave = new Float32Array(3);
function hash(x: number) { return ((Math.sin(x * 127.1 + 311.7) * 43758.5453) % 1 + 1) % 1; }

/** Three related coordinate studies, sampled once. No random values in the frame loop. */
export function createReticlePoints(detail = 100): Float32Array {
  const points: number[] = [];
  for (let plane = 0; plane < 3; plane++) for (let row = 0; row < detail; row++) for (let column = 0; column < detail; column++) {
    const id = plane * detail * detail + row * detail + column;
    const u = ((column + .5 + (hash(id) - .5) * .3) / detail - .5) * 2;
    const v = ((row + .5 + (hash(id + 913) - .5) * .3) / detail - .5) * 2;
    const border = Math.max(Math.abs(u), Math.abs(v));
    const keep = plane === 0 ? Math.abs(u) < .16 || Math.abs(v) < .16
      : plane === 1 ? border > .72 && border < .91
      : Math.min(Math.abs(u - v), Math.abs(u + v)) < .065 && border < .86;
    if (keep) points.push(u, v, plane, hash(id + 71));
  }
  return new Float32Array(points);
}

/** Writes projected position, opacity and point diameter to out[0..3]. */
export function sampleReticle(points: Float32Array, offset: number, frame: ReticleFrame, out: Float32Array) {
  const u = points[offset], v = points[offset + 1], plane = points[offset + 2], seed = points[offset + 3];
  const phase = frame.phase * Math.PI * 2;
  const entrance = Math.min(1, Math.max(0, frame.intro * 1.25 - seed * .25));
  const settle = 1 - (1 - entrance) ** 3;
  let x = u, y = v, z = 0;
  if (plane === 0) z = .10 * Math.sin(u * 3 + phase) * Math.cos(v * 3 - phase);
  if (plane === 1) { x *= 1.06; y *= 1.06; z = -.36 + .22 * Math.sin(v * 2 + phase); }
  if (plane === 2) { x *= .91; y *= .91; z = .34 + .13 * Math.cos(u * 3 - phase); }
  const tiltX = -.28 + .10 * Math.sin(phase), tiltY = .42 + .14 * Math.cos(phase), turn = -.29 + .10 * Math.sin(phase);
  const y1 = y * Math.cos(tiltX) - z * Math.sin(tiltX), z1 = y * Math.sin(tiltX) + z * Math.cos(tiltX);
  const x1 = x * Math.cos(tiltY) + z1 * Math.sin(tiltY), z2 = -x * Math.sin(tiltY) + z1 * Math.cos(tiltY);
  x = x1 * Math.cos(turn) - y1 * Math.sin(turn); y = x1 * Math.sin(turn) + y1 * Math.cos(turn);
  const perspective = 4.8 / (4.8 - z2);
  x *= perspective; y *= perspective;
  const dx = x - frame.pointerX, dy = y - frame.pointerY, distance = Math.hypot(dx, dy);
  const force = Math.exp(-distance * distance / .20) * frame.strength * .12;
  samplePulses(frame.waves, x, y, 1, 2.8, .18, .15, .20, wave);
  out[0] = x * (1.12 - settle * .12) + dx / (distance + .05) * force - dy * force * .35 + wave[0];
  out[1] = y * (1.12 - settle * .12) + dy / (distance + .05) * force + dx * force * .35 + wave[1];
  const graze = .5 + .5 * Math.cos(u * 2 - v * 2 + phase + plane);
  out[2] = Math.min(.92, (.34 + graze * .30 + (z2 + .6) * .18 + wave[2] * .3) * entrance);
  out[3] = (1.42 + seed * .18) * perspective;
}
