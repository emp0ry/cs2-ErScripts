import test from 'node:test';
import assert from 'node:assert/strict';
import { createReticlePoints, sampleReticle, type ReticleFrame } from '../src/core/reticle-field.ts';
import { createPulseTrain } from '../src/core/pulses.ts';
test('reticle samples are stable and include three coordinate planes', () => {
  const points = createReticlePoints(48);
  assert.deepEqual(points, createReticlePoints(48));
  assert.deepEqual(new Set(Array.from(points).filter((_, i) => i % 4 === 2)), new Set([0,1,2]));
});
test('the mathematical pose closes its loop and stays finite under overlapping waves', () => {
  const points = createReticlePoints(24), pulses = createPulseTrain(2.4), a = new Float32Array(4), b = new Float32Array(4);
  const frame: ReticleFrame = { phase: 0, intro: 1, pointerX: 0, pointerY: 0, strength: 0, waves: pulses.frame };
  for (let i = 0; i < points.length; i += 4) {
    sampleReticle(points, i, frame, a); frame.phase = 1; sampleReticle(points, i, frame, b); frame.phase = 0;
    for (let n = 0; n < 4; n++) assert(Math.abs(a[n] - b[n]) < 1e-5);
  }
  pulses.push(0,0); pulses.advance(.2); pulses.push(.2,.1); assert.equal(pulses.frame.count, 2);
  frame.strength = 1; frame.intro = .7;
  for (let i = 0; i < points.length; i += 4) {
    sampleReticle(points, i, frame, a); assert(a.every(Number.isFinite)); assert(Math.abs(a[0]) < 2 && Math.abs(a[1]) < 2); assert(a[2] >= 0 && a[2] <= 1);
  }
});
