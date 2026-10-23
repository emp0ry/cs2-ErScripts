# ErScripts 2.0.0 · EMP0RY product design · 23 October 2026

Authority: existing EMP0RY v19, MiruShin and ErScripts product design specifications.

Graphite #09090d, surface #111116, readable off-white #f4f2f7, muted #aaa6b5, restrained lavender #b7a0e5. Self-hosted Geist 400–600 for UI, 900 for the optical lowercase wordmark. 1320px content maximum, fluid 20–72px gutters, four-pixel spacing rhythm. Dark translucent buttons, thin lavender rules, five-pixel corners, 44px minimum controls and visible keyboard focus. Fixed shared header outside a clipped native scroll layer; page content cannot appear above it.

ErScripts keeps a technical composition: three related crosshair planes made from mathematical points in the hero, followed by a concise feature overview, a tabbed gallery of authentic 2.0.0 menu captures, actual overlay captures, setup, factual FAQs and the verified executable. The artwork responds to pointer and keyboard input with independent waves. Product screenshots stay in the gallery, complete and expandable. No imitation native controls or game actions are added to the hero. Composition and point geometry may differ from MiruShin and other product sites; the shared navigation, typography and controls stay aligned.

Hero type 64–88px on desktop and 40–48px on phones, section type 32–48px, readable 14–18px UI/body. Natural content-sized sections, no empty full-screen product panels or repeated oversized titles. Captures remain complete and modestly sized; expand them in a focus-managed viewer. Installation choices and download formats remain semantic and available without script.

Motion: one calm 24-second point cycle, bounded pointer easing, 160–220ms control transitions. Render at 30fps with capped DPR and fewer phone points; stop offscreen/hidden, clean up listeners/observers/frames, preserve static reduced-motion pose and original SVG fallback. A text entrance must never gate content availability. No visible move/click hints, background pause buttons, custom cursors, noisy full-page particle clouds or imported 3D models.

Accessibility: semantic heading order, native anchors, accessible mobile dialog with Escape/focus restoration, touch-safe controls, gallery tabs with arrow/Home/End keys, descriptive screenshot alt text, one product-support address. Preserve compatibility, cautions, attribution, original domains and public anchors. Do not invent testimonials, installations, versions, platform builds, analytics properties or software guarantees.

The previously inspected frontend-design and emilkowalski-motion workflows inform implementation; the EMP0RY specification is the brand authority. No advertised upstream review/document bundle is claimed. Verify the production build in a browser at phone, tablet, desktop and short-landscape dimensions. Record actual results and distinguish local measurements from physical-device and production-network performance.


## Layout and compiled-style regressions

Content sections always keep their natural height. A fragment ID on a hero must not inherit a zero-height marker rule. Keep the header outside the clipped scroll layer, and test both section containment and header-link collisions after reflow. Put vendor-prefixed backdrop-filter declarations before the standard declaration so the production CSS retains both engine paths. Mobile visibility rules must come after base display rules.


## Canonical product components

`src/brand.css` carries the established ErScripts product components, with matching MiruShin rendering checked in the browser. The source reference is `cs2-ErScripts` commit `42eda6bdf7e496bc72602549e751a7816ef1c44f`, `website/src/styles.css`. The existing homepage shares the desktop wordmark foundation.

- The header uses a full-width three-column grid, fluid gutters, an 80px desktop height, 68px phone height and 64px short-landscape height. Do not add a content-max wrapper inside the header.
- Wordmark: Geist 900, 25px desktop, 23px at 850px and below, 22px at 370px and below, letter spacing -0.07em. The dot inherits the same spacing and uses the shared lavender.
- Product name: Geist 500, 14px desktop and 13px on compact widths. Divider: weight 300, 22px desktop and 18px on phones. Identity gaps are 18px, 12px and 10px at the reference breakpoints.
- Buttons retain the reference dark translucent surface, 8px blur, thin lavender border, 5px radius, weight 400, 14px base text and the exact compact variants. Hero typography, helper copy, tabs, public contact links and navigation controls follow the same system.
- Product variation belongs in composition, point geometry and factual content. Shared header geometry, optical wordmark spacing and controls must remain consistent.
- Do not use em dashes or en dashes in website copy, document titles, metadata or accessibility labels. Use punctuation appropriate to the sentence and write numeric ranges with “to”. Keep exact commands and published filenames intact.

`src/captures.css` reserves native widths for compact lazy-loaded screenshots. A hash navigation must not shift after those images acquire their intrinsic dimensions.
