# Local verification · 23 October 2026

Tested the production build at **http://127.0.0.1:4176/** in the available in-app browser. The native Windows executable was not installed or run.

## Completed

- Visually inspected the hero at 1440×900, 1280×720 and 390×844. Inspected capture and download layouts at 390px, and the download layout at 320×740. No horizontal overflow was observed in these views. Root scroll position remained 0, with content scrolling inside the clipped layer below the fixed header.
- Played all three original captures. Each reached readyState 4 and advanced; selecting the next tab paused the previous clip. Checked arrow-key/End tab navigation, settings-image expansion, Escape dismissal and focus restoration to the screenshot link. Captures do not autoplay.
- Checked the mobile menu, Escape focus restoration, scroll locking and menu navigation to setup. Copying launch options reported success; source/output checks preserve the exact command. The browser clipboard inspection did not return portable text, so clipboard contents are not independently asserted here.
- The Windows download resolved visibly to release v1.2.4, 2.2 MB, using the validated official asset URL. API failure and invalid/partial releases have usable release-page fallbacks and logic coverage. No binary download was executed.
- Observed successive click waves coexisting in the hero; observed the renderer pause offscreen. Hidden-document, disposal, reduced-motion and unavailable-canvas branches are covered by lifecycle tests. Remaining browser checks for these branches are listed below.
- Checked the support footer at phone width, with only `support@emp0ry.com`, and no horizontal overflow. The homepage uses `contact@emp0ry.com`, and MiruShin uses `support@emp0ry.com`.
- No errors or warnings were returned from the captured ErScripts QA tab. Static assets, exact CSP hashes, anchors, metadata, the 1200×630 image, original CNAME/icon and synchronization into `docs/` pass output checks.

## Measured rendering

Unthrottled localhost observations in the available desktop environment, with warm cache. These are not field Core Web Vitals or physical-phone performance results. The artwork deliberately renders at 30 fps.

| Layout | LCP | CLS | Draw p95 | Render interval median |
| --- | ---: | ---: | ---: | ---: |
| 1280×720 | 164 ms | 0 | 2.8 ms | 33.3 ms |
| 1440×900, settled | 100 ms | 0 | 4.5 ms | 33.3 ms |
| 390×844, resized session | 248 ms | 0 | 6.4 ms | 33.4 ms |
| 390×844, later fresh page | 152 ms | 0 | 2.7 ms | 33.3 ms |
| 320×740, download view | 116 ms | 0.0003 | offscreen | paused |

Raw initial observations and capture checks are in `browser-observations.json`. The mobile resized-session drawing used the desktop point population; a fresh phone load uses fewer points. Controls responded by the next observed browser state. A numeric INP measurement was not obtained.

## Automated checks

`npm test`: **13 passed**, covering anchor navigation, release validation, deterministic geometry, cyclic continuity and renderer lifecycle. Strict TypeScript/Vite build: **passed**. `npm run check:output`: **passed**. `git diff --check`: **passed**.

Native application code is unchanged. Existing domain and icon hashes are checked against `design/baseline.json`. Product copy and setup commands were checked against the repository and release documentation; disabled PixelTrigger functionality is not advertised.

## Evidence and limitations

Screenshots: `hero-1440.png`, `hero-390.png`, `setup-390.png`, `contact-390.png`. The setup screenshot records command interaction, rather than proving initial anchor alignment.

Browser access was disabled in Work settings before the final tablet, landscape, reduced-motion/fallback visual checks and broader section review could finish. These remain unverified in the browser. No-JavaScript access is checked from generated HTML. Physical iPhone Safari browser chrome, Android hardware, screen-reader announcements, text enlargement and other browser engines still need device testing. Production social-crawler caches and deployment delivery were not part of the local checks. No upstream design-review bundle or external certification is claimed.
