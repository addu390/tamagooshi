import { INVADER } from "./sprite.js";

function greet() {
  const art = INVADER
    .map((row) => [...row].map((ch) => (ch === "#" ? "\u2588\u2588" : "  ")).join(""))
    .join("\n");
  console.log(
    `%c\n${art}\n\n%c TAMAGOOSHI %c https://github.com/addu390/tamagooshi\n`,
    "color:#0e9f6e; line-height:1.1",
    "background:#18181b; color:#fff; font-weight:700; padding:2px 6px; border-radius:3px",
    "color:#8b8b93",
  );
}

export function initChrome() {
  greet();
  const cfg = window.TAMA_PAGE || {};
  const chrome = (window.TAMA_PRESET || {}).chrome || {};

  const navInner = document.querySelector("header.nav .nav-inner");
  if (navInner) {
    const brand = Object.assign({}, chrome, cfg.brand);
    const links = (cfg.nav || [])
      .map(([href, label]) => `<a href="${href}">${label}</a>`)
      .join("");
    navInner.innerHTML =
      `<a class="brand" href="${brand.href || "./"}">` +
      `<span class="logo"${brand.accent ? ' style="background:var(--accent)"' : ""}>${brand.logo || "T"}</span> ` +
      `<span class="wordmark">${brand.pre || ""}Tamag<span class="o">o</span><span class="o o2">o</span>shi</span></a>` +
      (brand.note ? `<span class="nav-note">${brand.note}</span>` : "") +
      `<nav class="nav-links">${links}` +
      `<a class="btn" href="https://github.com/addu390/tamagooshi" target="_blank" rel="noopener" data-key-hint="G">GitHub <kbd>G</kbd></a></nav>`;
  }

  const footWrap = document.querySelector("footer .wrap");
  if (footWrap) {
    footWrap.innerHTML =
      "<span>&copy; 2026 Tamagooshi.</span>" +
      `<span>${cfg.footer || chrome.footer || "Built with PlatformIO, LVGL &amp; a lot of pixels."}</span>`;
  }

  document.querySelectorAll('[data-mock="builder"]').forEach((card) => {
    const fields = [["Brand", 72], ["Theme", 54], ["Packs", 64], ["Rules", 46]]
      .map(([label, w]) =>
        `<span class="buildmock-field"><b>${label}</b><i style="width:${w}%"></i></span>`)
      .join("");
    const lines = [44, 68, 58, 74, 50].map((w) => `<i style="width:${w}%"></i>`).join("");
    card.innerHTML =
      `<span class="hostmock-bar"><i></i>${card.dataset.bar || "builder"}</span>` +
      `<span class="buildmock">` +
      `<span class="buildmock-form">${fields}</span>` +
      `<span class="buildmock-yaml"><em>config.yaml</em>${lines}</span></span>` +
      `<span class="buildmock-foot"><span class="appmock-cta">Flash my config</span></span>`;
  });
}
