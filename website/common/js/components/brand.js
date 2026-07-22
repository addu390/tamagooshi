export const REPO_URL = "https://github.com/addu390/tamagooshi";

export function brandLockup(brand = {}) {
  return (
    `<a class="brand" href="${brand.href || "/"}">` +
    `<span class="logo"${brand.accent ? ' style="background:var(--accent)"' : ""}>${brand.logo || "T"}</span> ` +
    `<span class="wordmark">${brand.pre || ""}Tamag<span class="o">o</span><span class="o o2">o</span>shi</span></a>`
  );
}

export function githubButton() {
  return `<a class="btn" href="${REPO_URL}" target="_blank" rel="noopener" data-key-hint="G">GitHub <kbd>G</kbd></a>`;
}
