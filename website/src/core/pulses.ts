/** A bounded wave pool. Full pools preserve their existing waves. */
export const MAX_PULSES = 32;
export interface PulseFrame { data: Float32Array; count: number }

export function createPulseTrain(lifetime: number) {
    if (!Number.isFinite(lifetime) || lifetime <= 0) throw new RangeError('Pulse lifetime must be positive.');
    const frame: PulseFrame = { data: new Float32Array(MAX_PULSES*3), count: 0 };
    return {
        frame,
        push(x: number, y: number) {
            if (frame.count === MAX_PULSES || !Number.isFinite(x) || !Number.isFinite(y)) return false;
            const offset = frame.count++*3;
            frame.data[offset] = x; frame.data[offset+1] = y; frame.data[offset+2] = 0;
            return true;
        },
        advance(seconds: number) {
            if (!Number.isFinite(seconds) || seconds <= 0) return;
            let count = 0;
            for (let i = 0; i < frame.count; i++) {
                const source = i*3, age = frame.data[source+2]+seconds/lifetime;
                if (age >= 1) continue;
                const target = count++*3;
                frame.data[target] = frame.data[source]; frame.data[target+1] = frame.data[source+1]; frame.data[target+2] = age;
            }
            frame.count = count;
        },
        clear() { frame.count = 0; frame.data.fill(0); },
    };
}

/** Independent fronts use the same undeformed point; bounded sums avoid explosive bursts.
 * Writes normalized x/y displacement and the strongest local front into out[0..2].
 */
export function samplePulses(frame: PulseFrame, x: number, y: number, aspect: number,
    speed: number, width: number, amplitude: number, limit: number, out: Float32Array) {
    let forceX = 0, forceY = 0, light = 0;
    for (let i = 0; i < frame.count; i++) {
        const offset = i*3, age = frame.data[offset+2];
        if (age < 0 || age >= 1) continue;
        const dx = (x-frame.data[offset])*aspect, dy = y-frame.data[offset+1], radius = Math.hypot(dx,dy);
        const pulse = Math.exp(-(((radius-age*speed)/width)**2))*(1-age);
        forceX += dx/(radius+.02)*pulse*amplitude;
        forceY += dy/(radius+.02)*pulse*amplitude;
        light = Math.max(light,pulse);
    }
    const scale = Math.min(1,limit/(Math.hypot(forceX,forceY)+.000001));
    out[0] = forceX*scale/aspect; out[1] = forceY*scale; out[2] = light;
}
