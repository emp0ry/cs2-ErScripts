import './styles.css';
import { mountAnchorNavigation } from './core/anchor-navigation';
import { mountReticle } from './core/reticle';
import { mountRelease } from './core/releases';

document.documentElement.classList.remove('no-js');
const motion = matchMedia('(prefers-reduced-motion: reduce)');
const reduced = motion.matches || new URLSearchParams(location.search).has('reduce-motion');
document.documentElement.classList.toggle('reduce-motion', reduced);
document.documentElement.dataset.motion = reduced ? 'reduced' : 'full';
const events = new AbortController(), options = { signal: events.signal };
const page = document.querySelector<HTMLElement>('.page-scroll')!;
if (location.pathname === '/' || location.pathname === '/index.html') {
  document.querySelectorAll<HTMLAnchorElement>('a[href^="/#"]').forEach(link => link.setAttribute('href', link.hash));
}
const navigation = mountAnchorNavigation(page);
motion.addEventListener('change', () => location.reload(), options);
document.querySelectorAll('[data-year]').forEach(node => node.textContent = String(new Date().getFullYear()));

const videos = [...document.querySelectorAll<HTMLVideoElement>('.capture-video')];
let playRequest = 0;
function playing(video: HTMLVideoElement, value: boolean) {
  const button = video.closest('.capture-panel')!.querySelector<HTMLButtonElement>('.play-capture')!;
  button.firstChild!.textContent = value ? 'Pause capture ' : 'Play capture ';
  button.querySelector('span')!.textContent = value ? 'Ⅱ' : '▷';
  button.setAttribute('aria-pressed', String(value));
}
function pauseCaptures() {
  playRequest++;
  for (const video of videos) { video.pause(); playing(video, false); }
}
for (const video of videos) {
  video.controls = false;
  const panel = video.closest<HTMLElement>('.capture-panel')!;
  panel.querySelector<HTMLButtonElement>('.play-capture')!.addEventListener('click', async () => {
    if (!video.paused) { pauseCaptures(); return; }
    pauseCaptures(); const request = playRequest;
    if (!video.getAttribute('src')) video.src = video.dataset.src!;
    try {
      await video.play();
      if (events.signal.aborted || request !== playRequest || panel.hidden || document.hidden) { video.pause(); return; }
      playing(video, true);
    } catch {
      playing(video, false);
      const fallback = panel.querySelector<HTMLElement>('.no-script-link')!;
      fallback.classList.add('is-visible');
      fallback.textContent = 'Open the original capture ↗';
    }
  }, options);
}
function checkCaptures() {
  for (const video of videos) {
    if (video.paused) continue;
    const rect = video.getBoundingClientRect();
    if (document.hidden || rect.bottom <= 0 || rect.top >= innerHeight) { pauseCaptures(); break; }
  }
}
page.addEventListener('scroll', checkCaptures, { ...options, passive: true });
document.addEventListener('visibilitychange', checkCaptures, options);
window.addEventListener('pagehide', pauseCaptures, options);

const tablist = document.querySelector('.capture-tabs');
if (tablist) {
  const tabs = [...tablist.querySelectorAll<HTMLButtonElement>('[role=tab]')];
  function select(index: number, focus = false) {
    pauseCaptures();
    tabs.forEach((tab, i) => {
      tab.setAttribute('aria-selected', String(i === index)); tab.tabIndex = i === index ? 0 : -1;
      document.getElementById(tab.getAttribute('aria-controls')!)!.hidden = i !== index;
    });
    if (focus) tabs[index].focus({ preventScroll: true });
  }
  tabs.forEach((tab, index) => {
    tab.addEventListener('click', () => select(index), options);
    tab.addEventListener('keydown', event => {
      let next = index;
      if (event.key === 'ArrowRight') next = (index + 1) % tabs.length;
      else if (event.key === 'ArrowLeft') next = (index + tabs.length - 1) % tabs.length;
      else if (event.key === 'Home') next = 0;
      else if (event.key === 'End') next = tabs.length - 1;
      else return;
      event.preventDefault(); select(next, true);
    }, options);
  });
  select(0);
}

const toggle = document.querySelector<HTMLButtonElement>('.menu-toggle')!;
const menu = document.querySelector<HTMLDialogElement>('#site-menu')!;
const viewer = document.querySelector<HTMLDialogElement>('#image-viewer')!;
let previousFocus: HTMLElement | null = null, navigationTarget: HTMLElement | null = null;
function open(dialog: HTMLDialogElement) {
  previousFocus = document.activeElement as HTMLElement;
  pauseCaptures(); dialog.showModal(); page.classList.add('is-locked');
  dialog.querySelector<HTMLElement>('h2')!.focus({ preventScroll: true });
  if (dialog === menu) toggle.setAttribute('aria-expanded', 'true');
}
for (const dialog of [menu, viewer]) {
  dialog.querySelector('[data-close]')!.addEventListener('click', () => dialog.close(), options);
  dialog.addEventListener('close', () => {
    page.classList.remove('is-locked');
    if (dialog === menu) toggle.setAttribute('aria-expanded', 'false');
    if (navigationTarget) { navigationTarget.setAttribute('tabindex', '-1'); navigationTarget.focus({ preventScroll: true }); navigationTarget = null; }
    else previousFocus?.focus({ preventScroll: true });
  }, options);
  dialog.addEventListener('click', event => {
    if (event.target !== dialog) return;
    const r = dialog.getBoundingClientRect();
    if (event.clientX < r.left || event.clientX > r.right || event.clientY < r.top || event.clientY > r.bottom) dialog.close();
  }, options);
  dialog.addEventListener('keydown', event => {
    if (event.key !== 'Tab') return;
    const controls = [...dialog.querySelectorAll<HTMLElement>('button,a[href]')], first = controls[0], last = controls.at(-1)!;
    if (event.shiftKey && (document.activeElement === first || document.activeElement === dialog.querySelector('h2'))) { event.preventDefault(); last.focus(); }
    else if (!event.shiftKey && document.activeElement === last) { event.preventDefault(); first.focus(); }
  }, options);
}
toggle.addEventListener('click', () => open(menu), options);
menu.querySelectorAll<HTMLAnchorElement>('a').forEach(link => link.addEventListener('click', () => {
  if (link.origin === location.origin && link.pathname === location.pathname && link.hash) navigationTarget = document.getElementById(decodeURIComponent(link.hash.slice(1)));
  page.classList.remove('is-locked'); menu.close();
}, options));
document.querySelectorAll<HTMLAnchorElement>('.screenshot-link').forEach(link => link.addEventListener('click', event => {
  if (event.button || event.metaKey || event.ctrlKey || event.shiftKey || event.altKey) return;
  event.preventDefault();
  const large = document.querySelector<HTMLImageElement>('#viewer-image')!;
  large.src = link.href; large.alt = link.querySelector('img')!.alt;
  document.querySelector('#image-title')!.textContent = link.dataset.imageTitle ?? 'ErScripts settings';
  open(viewer);
}, options));
document.querySelector<HTMLButtonElement>('.copy-button')?.addEventListener('click', async event => {
  const button = event.currentTarget as HTMLButtonElement, status = document.querySelector<HTMLElement>('.copy-status')!;
  try {
    await navigator.clipboard.writeText(document.querySelector('#launch-options')!.textContent!);
    if (events.signal.aborted) return;
    button.firstChild!.textContent = 'Copied '; status.textContent = 'CS2 launch options copied.';
  } catch { status.classList.remove('sr-only'); status.textContent = 'Select and copy the launch options above.'; }
}, options);

const host = document.querySelector<HTMLElement>('#reticle-field');
const reticle = host ? mountReticle(host, reduced) : null;
const download = document.querySelector<HTMLElement>('#downloads');
const release = download ? mountRelease(download) : null;
const metrics: PerformanceObserver[] = []; let cls = 0;
document.documentElement.dataset.cls = '0';
try { const observer = new PerformanceObserver(list => { for (const entry of list.getEntries()) document.documentElement.dataset.lcp = entry.startTime.toFixed(1); }); observer.observe({ type: 'largest-contentful-paint', buffered: true }); metrics.push(observer); } catch {}
try { const observer = new PerformanceObserver(list => { for (const entry of list.getEntries()) { const shift = entry as PerformanceEntry & { hadRecentInput: boolean; value: number }; if (!shift.hadRecentInput) cls += shift.value; } document.documentElement.dataset.cls = cls.toFixed(4); }); observer.observe({ type: 'layout-shift', buffered: true }); metrics.push(observer); } catch {}
if (import.meta.hot) import.meta.hot.dispose(() => { pauseCaptures(); events.abort(); reticle?.destroy(); release?.destroy(); navigation.destroy(); metrics.forEach(observer => observer.disconnect()); for (const dialog of [menu,viewer]) if (dialog.open) dialog.close(); page.classList.remove('is-locked'); });
