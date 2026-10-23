# ErScripts website

Production: https://erscripts.emp0ry.com/

This source restores the established EMP0RY product system around the current ErScripts 2.0.0 release. The native application and all release screenshots remain unchanged.

## Develop and publish

Node 24 or newer is required. Dependencies are pinned in the lockfile.

```sh
npm ci
npm run dev
npm test
npm run build
npm run check:output
npm run preview
```

Development: http://127.0.0.1:5176/. Production preview: http://127.0.0.1:4176/. An already-running preview server can serve `dist/` after each build. GitHub Pages serves `../docs`, which `npm run build` synchronizes from the source. Commit the source and generated output together.

## Maintenance

- `src/brand.css` is the shared EMP0RY header, typography, buttons, dialogs and contact system. `src/tokens.css` contains the graphite palette, Geist typography and spacing tokens. Keep these consistent with the other product websites.
- `src/product.css` contains only ErScripts composition and screenshot sizing.
- Update product text in `src/index.html`. Shared header, footer and dialogs use explicit HTML partials. Essential descriptions, all screenshots and direct download links are present without JavaScript.
- Keep the verified version, asset filename and screenshots aligned. `verification/release-snapshot.json` records the published release used for this revision; downloads point directly to its uploaded executable.
- Keep exact launch options and current risk/export limitations. No simulated game actions or native installation occur on this website.
- Preserve `#top`, `#features`, `#screenshots`, `#motion`, `#overlays`, `#setup`, `#details`, `#how`, `#responsibility` and `#downloads`.
- Public screenshot/icon/domain assets are copied from the 2.0.0 release without changes. Their SHA-256 values are recorded in `design/retained-assets.json`; `check:output` verifies them. Old `docs/styles.css` and `docs/script.js` remain available for already-cached 2.0.0 HTML, while the rebuilt page uses hashed assets.

## Artwork and credits

The mathematical point reticle and original SVG fallback were authored for ErScripts in the previous EMP0RY redesign (source reference `c21c27c`). It has a 24-second mathematical cycle, eased pointer interaction, keyboard controls and independent click pulses. Work pauses offscreen or while hidden. Reduced motion draws a fixed pose; unavailable canvas retains SVG.

Authentic ErScripts 2.0.0 captures: `src/public/img/screenshots/`, from repository release commit `ff60026`. Interface artwork and product copy belong to the project. Geist is self-hosted under the [SIL Open Font License](src/public/site/fonts/OFL.txt). The existing product icon, social preview, manifest and domain are retained. No analytics property or external font service is added.

The locally inspected frontend-design and motion workflows informed the original EMP0RY design; this revision follows the established specification. No advertised upstream review or design-document bundle is claimed.

See [verification/README.md](verification/README.md) for actual checks, screenshots and remaining limitations.
