# Asset provenance

Baseline repository: https://github.com/emp0ry/cs2-ErScripts at `5f8778e2d7e2905d4dcaf454ef8107a1f3a86661`. Project source is MIT licensed; see the repository's `LICENSE.txt`. Existing captures include Counter-Strike artwork and Dear ImGui interfaces, whose respective rights remain with their owners.

| Website asset | Source and treatment |
| --- | --- |
| `site/media/menu-1.webp`, `menu-2.webp` | Existing `Image/menu1.png` (225×285) and `Image/menu2.png` (229×286), converted to WebP without cropping. |
| `site/media/keystrokes.mp4` and poster | Existing `Image/keystrokes.gif` (196×196), converted to muted H.264 MP4 with fast-start metadata; original first frame supplies the WebP poster. |
| `site/media/crosshair.mp4` and poster | Existing `Image/crosshair.gif` (110×88), converted in the same way, without invented frames or cropping. |
| `site/media/bombtimer.mp4` and poster | Existing `Image/bombtimer.gif` (180×98), converted in the same way. |
| `site/fonts/Geist.woff2` | Self-hosted Geist variable font from the local EMP0RY/MiruShin assets. Copyright 2024 The Geist Project Authors; SIL OFL 1.1 retained in `site/fonts/OFL.txt`. Upstream: https://github.com/vercel/geist-font. |
| `site/icon.svg`, `site/reticle.svg`, reticle source | Original mathematical composition, deterministic point sampling, looping coordinate planes, arcs, pointer deformation and independent waves. No imported 3D model or AI raster image. |
| `design/social-preview.svg`, `site/social-preview.png` | Original composition using outlined Geist and mathematical point geometry. SVG converted to a 1200×630 PNG. |
| `docs/img/icon.ico` | Existing published icon retained byte-for-byte, with its old URL available. |

Assets under `site/` are authored/copied into `src/public/site/` and emitted into `docs/site/` by the build. The source captures are small; the page preserves their full aspect ratios and does not represent them as HD screenshots.

Product and platform names remain their owners' trademarks. The website does not imply affiliation with or approval from Valve or other platform vendors.
