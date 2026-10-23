# ErScripts 2.0.0 design verification

Date: 23 October 2026.

## Scope

Restored the established EMP0RY product design after release `ff60026` replaced the website template. Current 2.0.0 facts, nine original PNG captures, direct executable download, setup command, export limitations and automation notice remain. Native application code and release artifacts were not changed.

`src/brand.css` matches the current Hide Icons canonical product component sheet byte for byte. Shared components retain the MiruShin/EMP0RY wordmark, header geometry, outlined controls, focus states, dialogs and contact treatment. Release updates now have a documented source/build workflow and scoped maintenance instructions.

## Automated checks completed

- `npm ci` with the pinned lockfile, Node 26.7.0.
- `npm test`: 10 passing tests. Anchor navigation, resize/manual-reading behavior, finite loop geometry, overlapping waves, offscreen/hidden pause, cleanup, reduced motion and unavailable canvas.
- `npm run build`: TypeScript and Vite production build pass.
- `npm run check:output`: all release-owned asset hashes, exact launch options, current product facts, seven menu panels/nine captures, semantic no-script content, stable anchors, metadata, restrictive CSP, local files, font license and matching Pages output pass.
- `git diff --check`: no whitespace errors.
- Published v2.0.0 release verified through the repository API. `ErScripts.exe` is uploaded, 2,391,552 bytes; its exact versioned URL is retained. See `release-snapshot.json`.

## Browser checks completed

Tested the actual production build at `http://127.0.0.1:4176/` through the in-app browser.

| Viewport | Result |
| --- | --- |
| 1920 x 1080 | No page overflow, header collision or section containment failure |
| 1440 x 900 | Shared header/buttons/type reviewed; hero and gallery reviewed |
| 1280 x 720 | Compact desktop hierarchy and overview reviewed |
| 1024 x 768 | Layout containment and navigation reflow pass |
| 768 x 1024 | Tablet hero/header reviewed; layout containment pass |
| 390 x 844 | Phone hero, gallery, image viewer, setup and navigation sheet reviewed |
| 320 x 740 | Narrow layout containment and anchor alignment pass |
| 844 x 390 | Short landscape header and containment pass |

Section navigation remained at the scrollport's 24px inset after viewport reflow. The header remains outside the fixed, clipped scrolling layer; no page content is painted above it. The gallery tab strip intentionally scrolls horizontally on narrow screens, while the page width stays equal to the viewport.

All seven gallery tabs were activated. Each image loaded, retained its original aspect ratio and remained inside its stage. Config full-size view opened on desktop and phone; Escape closed it and restored focus. Home/End keys selected the first/last gallery tabs, including scrolling the last tab into view on phones. The mobile navigation sheet opened, Escape restored focus/`aria-expanded`, and its Inside link navigated and dismissed correctly. The launch-options control reported successful copying. A direct responsibility link opens its complete answer.

Five rapid keyboard activations produced five concurrent waves rather than restarting one. The hero paused offscreen. `?reduce-motion` displayed a static pose with no interaction control; `?no-canvas` kept the loaded SVG fallback and hid its inert control. The 404 retained the same header, controls and footer. No browser warning/error logs were observed during these checks.

Local unthrottled observation: LCP 132ms, CLS 0.0000, animation median interval 33.3ms (30fps cap), sampled draw p95 about 4.3ms. These are measurements of this local environment, not production-network or physical-device performance claims.

Screenshots are in `screenshots/`.

## Limitations

Viewport testing uses the in-app browser, not every physical device or browser engine. Physical iOS Safari, low-end hardware, throttled-network performance and the native Windows application were not exercised in this website-only revision. No-script availability was checked in generated HTML; this browser did not expose a JavaScript-disable control. The clipboard permission-denied fallback was retained but not forced in this browser.
