# Website maintenance

These instructions apply to `website/` and its published output in `docs/`.

- Author page changes in `website/src/`. Run the build there to generate GitHub Pages output. Do not replace `docs/index.html` with a separate template.
- Keep the established EMP0RY product system, shared with MiruShin and the other product websites. The header lockup is `emp0ry. / ErScripts`. `website/DESIGN.md`, `src/tokens.css` and `src/brand.css` define its typography, geometry and controls.
- Release updates change product facts, current captures and verified download URLs. Preserve the common header, dark outlined buttons, clipped page scroll layer, responsive menu and mathematical point artwork unless the user asks to redesign them.
- Preserve exact setup commands, published filenames, supported platforms, export limitations and automation cautions. Verify release assets before changing the download link. Do not invent analytics properties.
- Keep screenshot images complete, bounded in height and expandable. No screenshots or imitation native controls in the hero. Keep no-script content and direct download links useful.
- Do not use em dashes or en dashes in visible copy, metadata or accessibility labels. Preserve literal commands and filenames.
- Run `npm test`, `npm run build` and `npm run check:output` from `website/`. Review phone, tablet and desktop layouts when browser access is available, including section navigation and header clipping. Record actual checks and limitations in `website/verification/README.md`.

Native application changes and release packaging are outside this website guidance.
