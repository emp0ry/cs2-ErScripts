# ErScripts website

The product website for **erscripts.emp0ry.com**, built with the EMP0RY graphite, Geist and lavender design system. The native Windows application remains independent of this website.

Source directory: `/Users/emp0ry/Projects/С++/cs2-ErScripts/website`

Production preview: **http://127.0.0.1:4176/**

## Run

Requires Node.js 24 or newer and npm. From this directory:

```sh
npm ci
npm run dev
npm test
npm run build
npm run check:output
npm run preview
```

Development binds to localhost on port 5176; the production preview uses 4176. Dependencies and their lockfile are pinned. The build type-checks the source, writes `dist/`, and synchronizes managed output into `../docs/` for the existing GitHub Pages configuration. Commit the generated HTML and `docs/site/` assets together. The original `docs/CNAME`, ICO URL and legacy files remain available. No server runtime is needed on Pages.

## Edit

- `src/index.html` contains semantic product content, original app captures, Windows release fallback and the exact launch-options command.
- `src/header.html`, `footer.html`, `dialogs.html` and `meta.html` are shared page fragments.
- `src/styles.css` and `tokens.css` define the authored responsive layout and brand tokens.
- `src/main.ts` manages accessible capture tabs, on-demand clips, image expansion, mobile navigation and command copying.
- `src/core/reticle-field.ts` calculates the original looping mathematical artwork. `reticle.ts` renders it at 30 fps with capped DPR, bounded input, overlapping waves, offscreen/hidden pausing and cleanup.
- `src/core/releases.ts` validates a near-viewport request to the official GitHub release API. Failure leaves a useful link to GitHub Releases.
- `scripts/sync-docs.mjs` maintains only files recorded in `pages-files.json`; it does not alter application sources or the retained domain/icon.

See [DESIGN.md](DESIGN.md), [design/EMP0RY.md](design/EMP0RY.md), [ASSETS.md](ASSETS.md) and [verification/REPORT.md](verification/REPORT.md).

The original website contained no analytics property, so this rebuild does not invent one. Canonical, Open Graph, Twitter, structured application metadata, CSP, robots, sitemap and a manifest are included. Public product support uses `support@emp0ry.com`. The hero is artwork, and the captures are recordings rather than working browser replicas of the application.

## Artwork and failure states

Normal builds use checked-in artwork. To regenerate the social SVG/1200×630 PNG, install Python `fonttools[woff]` and ImageMagick, then run `python3 scripts/generate-brand.py` and rebuild.

Use `/?reduce-motion` to request a static pose and `/?no-canvas` to inspect the SVG fallback. Core copy, setup instructions, all captures and official release links remain available in the HTML without JavaScript. Full browser/device coverage and the remaining checks are documented in the verification report.
