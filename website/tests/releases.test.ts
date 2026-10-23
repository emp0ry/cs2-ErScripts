import test from 'node:test';
import assert from 'node:assert/strict';
import { parseRelease } from '../src/core/releases.ts';
const data = { tag_name: 'v1.2.4', html_url: 'https://github.com/emp0ry/cs2-ErScripts/releases/tag/v1.2.4', assets: [{ name: 'ErScripts.exe', state: 'uploaded', size: 2228736, browser_download_url: 'https://github.com/emp0ry/cs2-ErScripts/releases/download/v1.2.4/ErScripts.exe' }] };
test('the official published executable resolves with its actual size', () => {
  assert.equal(parseRelease(data)?.asset?.size, 2228736);
  assert.equal(parseRelease(data)?.asset?.url, data.assets[0].browser_download_url);
});
test('a partial release keeps a usable release-page fallback', () => {
  assert.equal(parseRelease({ ...data, assets: [] })?.asset, null);
  assert.equal(parseRelease({ ...data, assets: [] })?.url, data.html_url);
});
test('foreign repositories, unfinished files, wrong tags, and arbitrary URLs are not downloads', () => {
  for (const override of [{ browser_download_url: 'https://evil.example/ErScripts.exe' }, { state: 'new' }, { size: 0 }, { name: 'other.exe' }, { browser_download_url: data.assets[0].browser_download_url.replace('v1.2.4', 'v1.2.3') }]) {
    assert.equal(parseRelease({ ...data, assets: [{ ...data.assets[0], ...override }] })?.asset, null);
  }
  assert.equal(parseRelease({ ...data, html_url: 'https://github.com/other/repo/releases/tag/v1.2.4' }), null);
  assert.equal(parseRelease({ ...data, prerelease: true }), null);
  assert.equal(parseRelease({ ...data, tag_name: '../main' }), null);
});
