/** Keep a destination in place when the viewport reflows, until the reader moves. */
export function mountAnchorNavigation(scrollport: HTMLElement) {
    const events = new AbortController();
    const options = { signal: events.signal };
    let destination: HTMLElement | null = null;
    let frame = 0;
    let initialNavigationPending = true;
    const resolve = (hash: string) => {
        let element: HTMLElement | null;
        try { element = document.getElementById(decodeURIComponent(hash.slice(1))); }
        catch { return null; }
        return element?.matches('section,main,article,h2,#top,#main') ? element : null;
    };
    const rememberInitialPosition = () => {
        const target = resolve(location.hash);
        if (!target) return;
        const padding = parseFloat(getComputedStyle(scrollport).scrollPaddingTop) || 0;
        const margin = parseFloat(getComputedStyle(target).scrollMarginTop) || 0;
        const top = scrollport.getBoundingClientRect().top + scrollport.clientTop;
        if (Math.abs(target.getBoundingClientRect().top - top - padding - margin) < 2) destination = target;
    };
    const release = () => { initialNavigationPending = false; destination = null; cancelAnimationFrame(frame); };
    // Manual reading always wins over the last hash, including scrollbar dragging.
    window.addEventListener('wheel', release, { ...options, passive: true });
    window.addEventListener('touchstart', release, { ...options, passive: true });
    window.addEventListener('pointerdown', release, options);
    window.addEventListener('keydown', event => {
        if (!['ArrowDown','ArrowUp','PageDown','PageUp','Home','End',' '].includes(event.key)) return;
        release();
        // Native scrolling works for focus inside the page layer. Also allow
        // page keys from the fixed header or the initial document focus.
        const target = event.target as Element;
        if (event.defaultPrevented || event.altKey || event.ctrlKey || event.metaKey ||
            scrollport.contains(target) || target.closest('input,select,textarea,[contenteditable],dialog') ||
            (event.key === ' ' && target.closest('button,[role="button"]')) ||
            scrollport.classList.contains('is-locked')) return;
        event.preventDefault();
        if (event.key === 'Home' || event.key === 'End') {
            scrollport.scrollTo({ top: event.key === 'Home' ? 0 : scrollport.scrollHeight });
        } else {
            const direction = ['ArrowUp','PageUp'].includes(event.key) || (event.key === ' ' && event.shiftKey) ? -1 : 1;
            const padding = parseFloat(getComputedStyle(scrollport).scrollPaddingTop) || 0;
            scrollport.scrollBy({ top: direction * (event.key.startsWith('Arrow') ? 40 : (scrollport.clientHeight - padding) * .9) });
        }
    }, options);
    document.addEventListener('click', event => {
        if (event.defaultPrevented || event.button !== 0 || event.metaKey || event.ctrlKey || event.shiftKey || event.altKey) return;
        const link = (event.target as Element).closest<HTMLAnchorElement>('a[href^="#"]');
        if (link) destination = resolve(link.hash);
    }, options);
    window.addEventListener('hashchange', () => { destination = resolve(location.hash); }, options);
    window.addEventListener('resize', () => {
        if (!destination) return;
        cancelAnimationFrame(frame);
        frame = requestAnimationFrame(() => destination?.scrollIntoView({ block: 'start', behavior: 'instant' }));
    }, options);
    const initializePosition = () => {
        const target = resolve(location.hash);
        // Some engines resolve the initial fragment before the fixed scrollport
        // is laid out. Respect a restored/manual position; repair only its origin.
        if (initialNavigationPending && scrollport.scrollTop === 0 && target) {
            destination = target;
            target.scrollIntoView({ block: 'start', behavior: 'instant' });
        } else rememberInitialPosition();
        initialNavigationPending = false;
    };
    if (document.readyState === 'complete') initializePosition();
    else window.addEventListener('load', initializePosition, { ...options, once: true });
    scrollport.addEventListener('scrollend', () => {
        if (!destination) rememberInitialPosition();
    }, options);
    rememberInitialPosition();
    return { destroy() { events.abort(); cancelAnimationFrame(frame); } };
}
