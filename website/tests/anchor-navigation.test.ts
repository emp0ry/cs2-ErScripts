import test from 'node:test';
import assert from 'node:assert/strict';
import { mountAnchorNavigation } from '../src/core/anchor-navigation.ts';

function environment(hash = '#media', targetTop = 90) {
    const frames = new Map<number, FrameRequestCallback>();
    let nextFrame = 0, alignments = 0;
    const scrolls: { kind: string; top?: number }[] = [];
    const classes = new Set<string>();
    const body = { closest: () => null };
    const headerLink = { closest: () => null };
    const control = { closest: () => ({}) };
    const button = { closest: (selector: string) => selector.includes('button') ? {} : null };
    const inside = { closest: () => null };
    const target = {
        matches: () => true,
        getBoundingClientRect: () => ({ top: targetTop }),
        scrollIntoView: () => { alignments++; },
    };
    const scrollport = Object.assign(new EventTarget(), {
        clientTop: 2, clientHeight: 800, scrollHeight: 4000, scrollTop: 0,
        getBoundingClientRect: () => ({ top: 8 }),
        contains: (element: unknown) => element === inside,
        classList: { contains: (name: string) => classes.has(name) },
        scrollBy: ({ top }: ScrollToOptions) => scrolls.push({ kind: 'by', top }),
        scrollTo: ({ top }: ScrollToOptions) => scrolls.push({ kind: 'to', top }),
    });
    const window = new EventTarget();
    const document = Object.assign(new EventTarget(), { readyState: 'loading', getElementById: (id: string) => id === 'media' ? target : null });
    const globals = {
        document, window, location: { hash },
        getComputedStyle: (element: unknown) => ({ scrollPaddingTop: element === scrollport ? '80px' : '999px', scrollMarginTop: '0px' }),
        requestAnimationFrame: (callback: FrameRequestCallback) => { frames.set(++nextFrame, callback); return nextFrame; },
        cancelAnimationFrame: (id: number) => frames.delete(id),
    };
    const previous = Object.fromEntries(Object.keys(globals).map(key => [key, Object.getOwnPropertyDescriptor(globalThis, key)]));
    Object.entries(globals).forEach(([key, value]) => Object.defineProperty(globalThis, key, { configurable: true, writable: true, value }));
    return {
        window, scrollport, scrolls, classes, body, headerLink, control, button, inside, target, frames,
        get alignments() { return alignments; },
        setTop(value: number) { targetTop = value; },
        flush() { const pending = [...frames.values()]; frames.clear(); pending.forEach(callback => callback(0)); },
        key(key: string, target: unknown = body, extra: Record<string, unknown> = {}) {
            const event = new Event('keydown', { cancelable: true });
            Object.defineProperties(event, Object.fromEntries(Object.entries({ key, target, ...extra }).map(([key, value]) => [key, { value }])));
            window.dispatchEvent(event);
            return event;
        },
        mount: () => mountAnchorNavigation(scrollport as unknown as HTMLElement),
        restore() {
            for (const [key, descriptor] of Object.entries(previous)) {
                if (descriptor) Object.defineProperty(globalThis, key, descriptor);
                else Reflect.deleteProperty(globalThis, key);
            }
        },
    };
}

test('anchor alignment uses the clipped scrollport, and manual scrolling releases resize alignment', () => {
    const e = environment(), navigation = e.mount();
    try {
        e.window.dispatchEvent(new Event('resize')); e.flush();
        assert.equal(e.alignments, 1, 'header padding is measured on the actual scroller');
        e.window.dispatchEvent(new Event('wheel'));
        e.window.dispatchEvent(new Event('resize')); e.flush();
        assert.equal(e.alignments, 1, 'resizing must not pull the reader back to an old hash');
        e.scrollport.dispatchEvent(new Event('scrollend'));
        e.window.dispatchEvent(new Event('resize')); e.flush();
        assert.equal(e.alignments, 2, 'scrollend on the inner scrollport can remember a newly aligned anchor');
        navigation.destroy();
        e.window.dispatchEvent(new Event('resize')); e.flush();
        assert.equal(e.alignments, 2);
    } finally { navigation.destroy(); e.restore(); }
});

test('page keys scroll from the fixed header and document focus without intercepting content or controls', () => {
    const e = environment('', 90), navigation = e.mount();
    try {
        assert(e.key('PageDown', e.headerLink).defaultPrevented);
        assert(e.key(' ', e.body, { shiftKey: true }).defaultPrevented);
        assert(e.key('Home', e.headerLink).defaultPrevented);
        assert(e.key('End', e.headerLink).defaultPrevented);
        assert.deepEqual(e.scrolls, [{ kind: 'by', top: 648 }, { kind: 'by', top: -648 }, { kind: 'to', top: 0 }, { kind: 'to', top: 4000 }]);
        assert(!e.key('PageDown', e.inside).defaultPrevented);
        assert(!e.key(' ', e.control).defaultPrevented);
        assert(!e.key('Home', e.body, { metaKey: true }).defaultPrevented);
        assert(!e.key(' ', e.button).defaultPrevented, 'Space still activates header buttons');
        assert(e.key('PageDown', e.button).defaultPrevented, 'page keys also work from header buttons');
        e.classes.add('is-locked');
        assert(!e.key('PageDown').defaultPrevented);
        assert.equal(e.scrolls.length, 5, 'an open directory must keep the page in place');
        navigation.destroy();
        assert(!e.key('PageDown').defaultPrevented);
    } finally { navigation.destroy(); e.restore(); }
});

test('a hash away from its target does not override restored reading position on resize', () => {
    const e = environment('#media', -200), navigation = e.mount();
    try {
        e.window.dispatchEvent(new Event('resize')); e.flush();
        assert.equal(e.alignments, 0);
    } finally { navigation.destroy(); e.restore(); }
});

test('initial fragments are repaired only at the untouched scroll origin', () => {
    for (const state of ['origin', 'restored', 'manual']) {
        const e = environment('#media', 900), navigation = e.mount();
        try {
            if (state === 'restored') e.scrollport.scrollTop = 350;
            if (state === 'manual') e.window.dispatchEvent(new Event('touchstart'));
            e.window.dispatchEvent(new Event('load'));
            assert.equal(e.alignments, state === 'origin' ? 1 : 0, state);
        } finally { navigation.destroy(); e.restore(); }
    }
});
