// ESP Web Tools plumbing, mirrored to website/docs/js/gen/wire/flasher.js by gen.

export const EWT_URL = "https://unpkg.com/esp-web-tools@10/dist/web/install-button.js?module";

export const variant = (manifest) =>
  ((manifest.device || {}).transports || {}).wifi ? "gooshi-wifi" : "gooshi";

export const imageName = (manifest, board) => `${variant(manifest)}-${board.asset}.bin`;
export const imageUrl = (release, manifest, board) => release + imageName(manifest, board);

const urls = [];

const objectUrl = (content, type) => {
  const url = URL.createObjectURL(new Blob([content], { type }));
  urls.push(url);
  return url;
};

export function revokeUrls() {
  while (urls.length) URL.revokeObjectURL(urls.pop());
}

export function installerManifest(chipFamily, parts) {
  return objectUrl(JSON.stringify({
    name: "Tamagooshi", version: "1", new_install_prompt_erase: true,
    builds: [{ chipFamily, parts }],
  }), "application/json");
}

export function configPart(blobBytes, offset) {
  return { path: objectUrl(blobBytes, "application/octet-stream"), offset };
}

export function installButton(label, cls, manifestUrl) {
  const wrap = document.createElement("esp-web-install-button");
  wrap.setAttribute("manifest", manifestUrl);

  const activate = document.createElement("button");
  activate.type = "button";
  activate.className = cls;
  activate.textContent = label;
  activate.setAttribute("slot", "activate");

  const note = (slot, text) => {
    const span = document.createElement("span");
    span.className = "flash-note";
    span.setAttribute("slot", slot);
    span.textContent = text;
    return span;
  };

  wrap.append(activate,
              note("unsupported",
                   "This browser can't flash over USB. Use Chrome or Edge on desktop."),
              note("not-allowed",
                   "Serial access was blocked. Reconnect the device and allow it."));
  return wrap;
}
