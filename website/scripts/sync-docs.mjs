import { readdir, readFile, mkdir, copyFile, writeFile, rm } from 'node:fs/promises';
import { resolve, dirname, join } from 'node:path';
const output = resolve('dist'), destination = resolve('../docs');
async function files(directory, prefix = '') {
  const result = [];
  for (const entry of await readdir(directory, { withFileTypes: true })) {
    const name = join(prefix, entry.name);
    if (entry.isDirectory()) result.push(...await files(join(directory, entry.name), name));
    else result.push(name);
  }
  return result;
}
const next = await files(output);
let previous = [];
try { previous = JSON.parse(await readFile('pages-files.json', 'utf8')); } catch {}
for (const file of previous) {
  if (!file.startsWith('site/') && !['index.html','404.html','robots.txt','sitemap.xml','site.webmanifest'].includes(file)) throw new Error('Unmanaged output path');
  if (!next.includes(file)) await rm(resolve(destination, file), { force: true });
}
for (const file of next) {
  const target = resolve(destination, file);
  if (!target.startsWith(destination + '/')) throw new Error('Invalid output path');
  await mkdir(dirname(target), { recursive: true });
  await copyFile(resolve(output, file), target);
}
await writeFile('pages-files.json', JSON.stringify(next.sort(), null, 2) + '\n');
await mkdir(resolve(output, 'img'), { recursive: true });
await copyFile(resolve(destination, 'CNAME'), resolve(output, 'CNAME'));
await copyFile(resolve(destination, 'img/icon.ico'), resolve(output, 'img/icon.ico'));
console.log(`Synced ${next.length} generated website files; original domain and icon retained.`);
