# EMP0RY design language

Version 19 · 23 October 2026. A portable contract for one ecosystem of separately maintained websites.

## Identity

Visually minimal. Technologically extraordinary. The identity comes from the weight of `emp0ry`, disciplined composition, and useful interactions. Software is the subject. Every project merits equal craft, regardless of platform or commercial model.

## Shared foundation

- Lowercase wordmark: self-hosted Geist 900, optical tracking approximately −0.065em, neutral white with a lavender period. Keep its original glyph shapes and real accessible text. The homepage may sample the loaded font into procedural points, as described below. Other navigation and footer wordmarks remain live text. Do not replace the identity with a downloaded image, glowing text, stretched letters, or unrelated glyphs.
- Geist 400–600 for body and controls, 550–650 for product names, with large feature titles at 600. Introductory body 16–18px; compact project descriptions 14–16px, with a 13px minimum in phone columns. Navigation and destination buttons 14px (13px in dense phone rows), with 44–48px minimum hit areas. Supporting text 11–13px; small operational labels 10–11px, letter spacing 0.10–0.16em; use these sparingly.
- Graphite background #09090D; surface #111116; raised #1A1922; text #F4F2F7; secondary #AAA6B5; lavender #B7A0E5. Low-contrast rules use rgba(199,188,218,.14). Secondary text must remain legible. Violet punctuates rather than fills.
- Twelve-column grid, maximum content width 1320px, fluid 20–72px gutters. Spacing follows 4, 8, 12, 16, 24, 32, 48, 64, 96, 128px. Group by proximity and alignment, not endless containers.
- Use horizontal editorial rules, strong labels, generous breathing room, and asymmetric compositions. Preview surfaces can have 10px corners; device outlines may have authentic larger corners. Do not wrap every project in a card.

## Homepage composition

The hero occupies exactly one dynamic viewport (`100dvh`, with a `100vh` fallback). Three grid rows hold the topline, flexible centered identity, and chapter dock. The dock stays at the viewport edge without absolute positioning or a pixel height cap. Header height, short-window spacing, and landscape type adapt independently.

Keep the document root stationary. Main content and the footer scroll natively inside one fixed, viewport-sized `.page-scroll` layer, with overflow containment and an explicit `inset(0)` paint clip. This prevents scrolled content from extending above the fixed header into Safari's toolbar region. The header and native dialog stay outside that layer; do not compensate by changing header opacity, blur, dimensions or placement. Apply the existing header scroll-padding to the actual scrollport. Keep native fragments/history, keyboard scrolling, reduced motion and directory scroll locking working with that same container.

Center the wordmark horizontally. Desktop size uses both viewport width and height, capped at 236px; it must not fill the whole horizontal field. Mobile keeps generous side clearance. Align the explanation and primary exploration link below it. Navigation stays available throughout the brief reveal.

The project catalog is content-sized. Every chapter starts with one compact line such as `01 / Media.`; the category is the actual h2, 18–22px. Do not add an ecosystem kicker or a second oversized category heading. Use 32–40px section padding and a 24–28px gap before the content. No chapter has a viewport-height minimum. Product names are 24–30px, with the single Media feature at 40–48px. Keep the description and destination close to the name.

Each chapter pairs real project information with one original mathematical point composition. Media has a point-built 新 mark adapted from MiruShin’s own icon; Desktop a projected spatial lattice beside three compact tool stories; Isaac a point-built portrait of the game character beside four projects; iPhone a mathematically outlined bitten-apple silhouette with a separate leaf beside six original product icons and concise descriptions. All fourteen descriptions and destinations remain visible in semantic HTML. Do not replace them with simulated apps, hidden panels, or settings controls.

The four studies are calculated from stable point coordinates, with slow continuous deformation, pointer-dependent perspective/lighting and reversible local displacement. A click, tap, Enter or Space adds an independent bounded pulse without resetting live waves; focused arrow keys steer the field. A shared Canvas2D scheduler draws only visible studies and stops when the document is hidden. Keep the hero’s sole WebGL2 renderer and approved base facet equations. Each study reserves its layout size, has an authored SVG fallback, and renders a fixed pose under reduced motion without a decorative activation control. Artwork uses the full reserved area: do not show pointer or pulse instructions underneath it. Retain screen-reader-only keyboard descriptions. Never add a model, external texture, noisy random-frame values, or a second full-screen background below the hero.

Product screenshots, gameplay captures, fake application interfaces, and working mini-apps do not appear on the homepage. Screenshots belong on the individual product websites. Original app icons and authored abstract symbols may identify projects. Captures from the earlier build are archived outside `public/` and must not ship in `dist/`.

All internal navigation uses native anchors and history. The document has one shared scroll-padding equal to the actual header height. Chapter anchors have zero extra scroll margin; the chapter's top rule lands directly below the header, with its content inset below that rule. Individual project links have a modest 20–24px extra inset. Do not combine several independent header offsets. The compatibility anchors `#open-source-projects` and `#havoc-products` point to the corresponding catalog/chapter entry, rather than the bottom dock. Closing the project index through a link restores focus to that project; Escape restores focus to its opener.

Do not stretch and vertically center the MiruShin content within a viewport-height article: its anchor must lead directly to readable content on phones. Project entrances use opacity without translating anchor targets. While parked at a chosen destination, viewport resize may restore native start alignment; manual scrolling immediately releases that behavior. Do not pull a reader back to an old hash after they begin reading.

The production homepage retains its existing Google Analytics property, canonical domain, CSP, sitemap and social metadata. Link previews use an original 1200×630 PNG converted from the outlined Geist/point-facet SVG master. Favicons and app icons share its graphite and lavender identity. These metadata assets are independent of the animated page and never contain product screenshots.

## Motion

Hover/press feedback 120–180ms; state changes 220–360ms; entrances 600–900ms. Standard easing cubic-bezier(.22,1,.36,1). Animate transform and opacity. Entrance stagger remains subtle and never delays navigation. Preserve native scrolling. Pointer response uses inertia and interpolation, never direct snapping.

The hero is a **full-screen field of kinetic point facets**, approved in the local preview. Large triangular planes meet along clean illuminated folds. The surface opens and closes across the screen, with no radial axis or central cutout. Preserve this geometry and point density rather than returning to tubular surfaces, flowing ribbons, or a flat contour pattern. Keep random point spacing, gradual individual appearance, restrained lavender grazing light, and the clear foreground identity.

Define a 6×6 lattice of quadrilateral cells, each split into two triangles. Adjacent cells evaluate the same vertex equations so their edges meet exactly. Vertex height combines alternating column/row folds with a small smooth diagonal term: h(i,j) = (1−2(i mod 2))(0.65+0.4sin(phase)) + (1−2(j mod 2))(0.45+0.28cos(phase)) + 0.12sin(0.75(i+j)+phase). Row staggering also changes gently with the phase. Sample each face with barycentric coordinates; 18% of seeds lie on its creases for legibility. Points remain dots: the renderer draws no filled triangles. Use modest orientation and bounded perspective, with a 7-unit camera distance. Portrait screens turn the working composition by 90 degrees and preserve equal physical scale in both axes.

Light follows each face normal, so broad changes of tone describe the planes rather than a random cloud. Keep the current 1.8 CSS-pixel base diameter, ±4% seeded variation, and bounded perspective scaling. Center attenuation is 30%, without removing geometry. Mouse input adds reversible local repulsion and swirl. Do not add extra noise layers or frame-random values.

Store stable source coordinates, birth offset, and variation in one four-component attribute. Stratified sampling retains 640 longitudinal source cells and a base-two transverse sequence. Each point has a staggered 12-second life window, smooth 1.92-second fade-in, and a soft fade-out before an invisible reset. A small bounded sine/cosine drift moves it across its facet. Seeds remain stable through refresh and density changes. Do not allocate point objects in the render loop.

The WebGL background uses one `POINTS` batch: 65,536 points on ordinary desktop widths, 98,304 above 1900px, and 24,576 below 760px. The Canvas2D fallback evaluates the same equations at 16,384 desktop or 8,192 mobile points. These approved counts fill the facets clearly despite substantial offscreen overscan. The backing buffer remains capped at 1600×1000 and density 1.5. The wordmark adds a second point batch in the same context and animation loop. Keep the fallback's reduced square-mark opacity so it does not overpower the soft GPU sprites.

Ambient motion loops in **24 seconds**: the pose, lighting, and particle lifecycles return continuously to their start, without a global reset cut or fade. Birth offsets are distributed across the cycle so the whole field never pulses or disappears together. Mouse proximity creates local repulsion and a tangential swirl in aspect-correct screen coordinates. Smooth pointer position over 240ms and interaction strength over 350ms; ease the field back after pointer exit. Clicking or touching adds a 2.4-second expanding displacement pulse without intercepting navigation or scrolling. Successive clicks preserve prior waves in a shared pool of at most 32 live fronts; a full pool preserves its existing waves. Add their forces from the same base point, cap the total displacement and expire each wave independently. Keep GPU and Canvas2D behavior matched, without a new animation loop. Reduced motion freezes both geometry and lifecycles, and suppresses decorative reactions. Pause rendering offscreen and in hidden tabs. There is no visible pause control.

Maintain the explicit layer contract: fallback canvas at z=0, primary canvas at z=1, shading at z=2. Promote WebGL only after its first draw and reveal Canvas2D immediately on context loss. Never run two animation loops concurrently. The renderer owns cleanup of buffers, contexts, observers, input listeners, and scheduled frames.

The central wordmark is an interactive point composition sampled from the real Geist 900 glyphs. Preserve its current font size, spacing, baseline, and footprint. Wait for the preloaded local font, measure the existing text ranges and baseline, and sample a temporary canvas mask. The period retains a lavender marker. Keep the semantic heading in place. Hold its opacity at zero during bounded font preparation to avoid flashing solid text before assembly, and make its color transparent only after the point renderer presents a valid layer. Font/mask/graphics failures restore live text; reduced motion keeps the normal heading and omits the decorative activation control.

On initial enhancement, points gather from a small surrounding cloud into the exact glyph positions over approximately 1.4 seconds. Replay only on a fresh mount; resizing and graphics restoration preserve the completed entrance. A moving cursor gently repels and swirls nearby points by at most about 24 CSS pixels, then the glyphs recover. A native, focus-visible overlay button lets pointer, touch, Enter, or Space briefly disperse and regroup the word. Preserve a screen-reader hint and native keyboard behavior. A slow lighting pass keeps the settled letters alive without drifting their outlines. Limit wordmark sampling to 16k points and keep all animation in the existing material loop.

## Components and interaction

- Header is compact and consistently placed. The all-projects dialog uses four horizontal category rows, short project links with directional arrows, a 44px circular SVG close button and no footer or project-count labels. Its supported-browser material uses a 46% graphite tint over 32px backdrop blur, a restrained grazing-light gradient and a lightly dimmed exterior backdrop. Use a 92% graphite tint when backdrop filters are unsupported. Use a centered 1000px maximum with 20px corners (16px on phones), independent interior scrolling for short viewports and 44px minimum targets. Opening focuses its heading; Tab/Shift+Tab stay inside; closing restores its opener or the selected project. A brief 200ms opacity/8px entrance honors reduced motion. Product pages retain their own repositories and URLs.
- Buttons have a real hover, pressed, disabled, and focus state. Distinguish navigation anchors from action buttons. Use at least 44px touch areas. Avoid nesting interactive controls inside links.
- The homepage introduces projects through concise descriptions, original product icons, authored abstract symbols, and one prominent “Visit project” destination per project. Do not embed working mini-apps, simulated settings, terminal fixtures, fake device controls, or product rehearsals. This supersedes the original thirteen-demo brief.
- Copy stays factual and concise. Put platform/compatibility details near the relevant product or link to its authoritative website. Never invent testimonials, metrics, prices, safety guarantees, or download statistics.
- Keep product screenshots on the individual project websites. Load the homepage’s original product icons lazily with reserved dimensions and retain their attribution.

## Accessibility and responsive behavior

Use semantic landmarks, ordered headings, a skip link, native controls, descriptive labels, and visible 2px lavender focus indicators. Meet WCAG AA text contrast. Announce actionable changes through restrained live regions. Every navigation control works with the keyboard. The pointer is never customized globally.

Mobile layouts are composed independently: single-column reading, compact navigation, simplified spatial diagrams, usable controls, and no horizontal overflow. Do not shrink a desktop canvas. User motion preferences override decorative choreography. Essential information and outbound links work without JavaScript; project descriptions and destinations do not depend on enhancement.

## Project variation contract

Keep the wordmark, palette foundation, font family, spacing logic, control semantics, accessibility, and motion restraint shared. A project may vary its composition, local accent, image composition, screenshot framing, and interaction on its own website. Product-specific accents are subordinate to the graphite foundation. Each future repository may copy this document and tokens, record its version, and add a clearly named project supplement; no shared backend or monorepo is required.

## Forbidden patterns

Generic SaaS grids; repeated project cards; excessive rounded containers; thin wordmarks; glowing text; neon; rainbow gradients; chrome typography; decorative planets, rocks, or purple spheres; random noise blobs; marketing filler; meaningless counters; fake metrics; fake native behavior; static graphics replacing procedural rendering; loading gates; scroll hijacking; global custom cursors; expensive hidden animation.

## Provenance and review

Research references: OpenDesign's brand-inspired Linear, Vercel, Raycast, and Apple systems. They are quality references, not templates or official component libraries. OpenDesign integration uses the inspected frontend-design and emilkowalski-motion instruction packages; other advertised upstream bundles are not assumed available. Review every project for equal quality, distinct composition, accurate claims, and coherent behavior at 1440, 1280, 768, and 390px.
