export const RELEASE_PAGE = 'https://github.com/emp0ry/cs2-ErScripts/releases/latest';
export const RELEASE_API = 'https://api.github.com/repos/emp0ry/cs2-ErScripts/releases/latest';
export interface Release { label: string; url: string; asset: { url: string; size: number } | null }

export function parseRelease(value: unknown): Release | null {
  if (!value || typeof value !== 'object') return null;
  const data = value as Record<string, unknown>;
  if (typeof data.tag_name !== 'string' || !/^v?\d+(?:\.\d+){1,3}$/.test(data.tag_name) || data.draft === true || data.prerelease === true) return null;
  const url = `https://github.com/emp0ry/cs2-ErScripts/releases/tag/${data.tag_name}`;
  if (data.html_url !== url) return null;
  const binary = `https://github.com/emp0ry/cs2-ErScripts/releases/download/${data.tag_name}/ErScripts.exe`;
  let asset: Release['asset'] = null;
  if (Array.isArray(data.assets)) for (const entry of data.assets) {
    if (!entry || typeof entry !== 'object') continue;
    if (entry.name === 'ErScripts.exe' && entry.state === 'uploaded' && entry.browser_download_url === binary && Number.isSafeInteger(entry.size) && entry.size > 0) {
      asset = { url: binary, size: entry.size }; break;
    }
  }
  return { label: data.tag_name, url, asset };
}

export function mountRelease(root: HTMLElement) {
  const lifetime = new AbortController(); let requested = false;
  async function resolve() {
    if (requested) return; requested = true;
    const status = root.querySelector<HTMLElement>('[data-release-status]')!;
    try {
      const response = await fetch(RELEASE_API, { headers: { Accept: 'application/vnd.github+json' }, signal: AbortSignal.any([lifetime.signal, AbortSignal.timeout(6500)]) });
      if (!response.ok) throw new Error('Release unavailable');
      const release = parseRelease(await response.json());
      if (!release) throw new Error('Invalid release');
      if (lifetime.signal.aborted) return;
      root.querySelector('[data-release-label]')!.textContent = release.label;
      const link = root.querySelector<HTMLAnchorElement>('[data-download]')!;
      link.href = release.asset?.url ?? release.url;
      link.querySelector('span')!.textContent = release.asset ? '↓' : '↗';
      link.setAttribute('aria-label', release.asset ? `Download ErScripts.exe ${release.label}` : `View the official ${release.label} release`);
      status.textContent = release.asset ? `Latest stable · ${release.label} · ${(release.asset.size / 1_000_000).toFixed(1)} MB` : `Latest stable · ${release.label} · Open the release for available files.`;
      root.dataset.release = 'resolved';
    } catch {
      if (!lifetime.signal.aborted) { status.textContent = 'Live check unavailable. The button opens the official GitHub release page.'; root.dataset.release = 'fallback'; }
    }
  }
  const observer = new IntersectionObserver(entries => { if (entries.some(entry => entry.isIntersecting)) { observer.disconnect(); void resolve(); } }, { root: root.closest('.page-scroll'), rootMargin: '500px' });
  observer.observe(root);
  return { destroy() { lifetime.abort(); observer.disconnect(); } };
}
