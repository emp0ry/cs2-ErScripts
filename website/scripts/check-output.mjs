import assert from 'node:assert/strict';
import { readFileSync, existsSync } from 'node:fs';
import { resolve } from 'node:path';
import { createHash } from 'node:crypto';

const read = path => readFileSync(path, 'utf8');
const hash = value => createHash('sha256').update(value).digest('hex');
const release = JSON.parse(read('verification/release-snapshot.json'));
assert.equal(release.tag, 'v2.0.0');
const executable = release.assets.find(asset => asset.name === 'ErScripts.exe');
assert.equal(executable.state, 'uploaded');
assert.equal(executable.size, 2391552);

for (const [path, expected] of Object.entries(JSON.parse(read('design/retained-assets.json')))) {
  assert.equal(hash(readFileSync(resolve('dist', path))), expected, `Retain release asset ${path}`);
  assert.equal(hash(readFileSync(resolve('../docs', path))), expected, `Retain published asset ${path}`);
}

const index = read('dist/index.html');
const ids = [...index.matchAll(/\bid="([^"]+)"/g)].map(match => match[1]);
assert.equal(ids.length, new Set(ids).size, 'Unique IDs');
for (const id of ['top','main','features','screenshots','motion','overlays','setup','details','how','responsibility','downloads']) {
  assert(ids.includes(id), `Stable #${id}`);
}
assert.equal((index.match(/role="tab"/g) || []).length, 7, 'All current menu captures');
assert.equal((index.match(/class="gallery-panel"/g) || []).length, 7);
assert.equal((index.match(/src="\/img\/screenshots\//g) || []).length, 9);
assert(!/<div class="gallery-panel"[^>]*\bhidden/.test(index), 'Screenshots survive script failure');
assert(index.includes(executable.url), 'Verified versioned download');
assert(index.includes('-conclearlog -condebug +bind scancode104 exec erscripts1'), 'Exact 2.0.0 setup');
for (const fact of ['120 FPS','Config export','Observer data is ignored','Hidden engine state cannot be exported','Loading a profile leaves them off','ErScripts is not affiliated with Valve']) {
  assert(index.includes(fact), `Current product fact: ${fact}`);
}
assert(!/<(?:form|input|video)\b/.test(index), 'No fake app, credential form or autoplay');
assert(!/class="hero-image/.test(index), 'Mathematical hero, gallery captures');

for (const page of ['index', '404']) {
  const html = read(`dist/${page}.html`);
  assert.equal((html.match(/<h1\b/g) || []).length, 1);
  assert(!/[\u2013\u2014]/.test(html), `${page}: no long dash in copy or metadata`);
  for (const meta of ['description','og:title','og:image','og:image:width','og:image:height','og:image:alt','twitter:card','theme-color']) {
    assert(html.includes(`="${meta}"`), `${page}: ${meta}`);
  }
  assert(html.includes('mailto:support@emp0ry.com'));
  assert(html.includes('https://erscripts.emp0ry.com/'));
  assert(!/fonts\.googleapis|fonts\.gstatic|googletagmanager/.test(html), 'Self-hosted font; no invented analytics');
  assert(!/unsafe-inline|unsafe-eval/.test(html), 'Restrictive CSP');
  assert(html.includes('Content-Security-Policy'));
  for (const [, asset] of html.matchAll(/(?:src|href)="(\/(?!\/)[^"?#]+)"/g)) {
    assert(existsSync(resolve('dist', asset.slice(1))), `${page}: local asset ${asset}`);
  }
  for (const [, id] of html.matchAll(/href="#([^"]+)"/g)) {
    assert(html.includes(`id="${id}"`), `${page}: local anchor ${id}`);
  }
  for (const [link] of html.matchAll(/<a\b[^>]*target="_blank"[^>]*>/g)) assert(link.includes('noopener'));
}

const png = readFileSync('dist/site/social-preview.png');
assert.equal(png.readUInt32BE(16), 1200);
assert.equal(png.readUInt32BE(20), 630);
for (const path of JSON.parse(read('pages-files.json'))) {
  assert(readFileSync(resolve('dist', path)).equals(readFileSync(resolve('../docs', path))), `Pages matches build: ${path}`);
}
const cssPath = index.match(/href="(\/site\/build\/[^"]+\.css)"/)[1];
const css = read(resolve('dist', cssPath.slice(1)));
assert(css.includes('-webkit-backdrop-filter:') && /(?:[;{])backdrop-filter:blur\(/.test(css), 'Both blur implementations survive build');
console.log('ErScripts: current release facts, 9 original captures, stable anchors, no-script content, metadata, CSP, retained assets and Pages output pass.');
