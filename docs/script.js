"use strict";

const galleryImage = document.querySelector("#gallery-image");
const galleryLink = document.querySelector("#gallery-link");
const shots = Array.from(document.querySelectorAll("[data-shot]"));

for (const shot of shots) {
  shot.addEventListener("click", event => {
    if (event.button || event.ctrlKey || event.metaKey || event.shiftKey || event.altKey) return;
    event.preventDefault();
    for (const other of shots) other.removeAttribute("aria-current");
    shot.setAttribute("aria-current", "true");
    galleryImage.src = shot.href;
    galleryImage.alt = `ErScripts 2.0.0 ${shot.dataset.title}`;
    galleryLink.href = shot.href;
    galleryLink.dataset.title = shot.dataset.title;
    document.querySelector("#gallery-title").textContent = shot.dataset.title;
    document.querySelector("#gallery-description").textContent = shot.dataset.description;
  });
}

const viewer = document.querySelector("#image-viewer");
const viewerImage = document.querySelector("#viewer-image");
const viewerTitle = document.querySelector("#viewer-title");
if (viewer && typeof viewer.showModal === "function") {
  for (const link of document.querySelectorAll(".screenshot")) {
    link.addEventListener("click", event => {
      if (event.button || event.ctrlKey || event.metaKey || event.shiftKey || event.altKey) return;
      event.preventDefault();
      viewerImage.src = link.href;
      viewerImage.alt = link.querySelector("img").alt;
      viewerTitle.textContent = link.dataset.title;
      viewer.showModal();
    });
  }
  document.querySelector(".viewer-close").addEventListener("click", () => viewer.close());
  viewer.addEventListener("click", event => {
    if (event.target !== viewer) return;
    const box = viewer.getBoundingClientRect();
    if (event.clientX < box.left || event.clientX > box.right || event.clientY < box.top || event.clientY > box.bottom) viewer.close();
  });
  viewer.addEventListener("close", () => viewerImage.removeAttribute("src"));
}

const copy = document.querySelector(".copy-button");
if (copy && navigator.clipboard?.writeText) {
  copy.hidden = false;
  copy.addEventListener("click", async () => {
    copy.disabled = true;
    try {
      await navigator.clipboard.writeText(document.querySelector("#launch-options").textContent);
      document.querySelector("#copy-status").textContent = "Launch options copied.";
    } catch {
      document.querySelector("#copy-status").textContent = "Select the launch options above and copy them.";
    } finally {
      copy.disabled = false;
    }
  });
}
