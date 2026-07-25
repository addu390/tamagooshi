function scopeStyles(cssText, scopeSelector) {
  return cssText.replace(/([^{}]+)\{([^{}]*)\}/g, (_, selectors, body) => {
    const cleanedBody = body
      .split(";")
      .map((d) => d.trim())
      .filter((d) => d && !/^--(?:accent|on-accent)\s*:/.test(d))
      .join("; ");
    const scoped = selectors
      .split(",")
      .map((s) => s.trim())
      .filter(Boolean)
      .map((s) => (s === ":root" || s === "svg" ? scopeSelector : `${scopeSelector} ${s}`))
      .join(", ");
    return `${scoped} { ${cleanedBody} }`;
  });
}

function prefixIds(svg, prefix) {
  svg.querySelectorAll("[id]").forEach((el) => {
    el.id = prefix + el.id;
  });
  const refAttrs = ["marker-start", "marker-mid", "marker-end", "fill", "stroke", "clip-path", "mask", "filter"];
  svg.querySelectorAll("*").forEach((el) => {
    refAttrs.forEach((attr) => {
      const v = el.getAttribute(attr);
      if (v && v.includes("url(#")) {
        el.setAttribute(attr, v.replace(/url\(#/g, `url(#${prefix}`));
      }
    });
  });
}

export function initDocFigures() {
  const imgs = document.querySelectorAll("img.doc-figure[src$='.svg']");
  imgs.forEach((img, i) => {
    const src = img.getAttribute("src");
    if (!src) return;
    fetch(src)
      .then((r) => (r.ok ? r.text() : Promise.reject(new Error(String(r.status)))))
      .then((text) => {
        const doc = new DOMParser().parseFromString(text, "image/svg+xml");
        if (doc.querySelector("parsererror")) return;
        const svg = doc.documentElement;
        if (!svg || svg.nodeName.toLowerCase() !== "svg") return;

        const scopeId = `docfig-${i}`;
        svg.setAttribute("id", scopeId);
        prefixIds(svg, `${scopeId}-`);

        svg.querySelectorAll("style").forEach((style) => {
          style.textContent = scopeStyles(style.textContent, `#${scopeId}`);
        });

        svg.removeAttribute("width");
        svg.removeAttribute("height");
        svg.setAttribute("class", "doc-figure");
        svg.setAttribute("role", "img");
        if (img.alt && !svg.getAttribute("aria-label")) svg.setAttribute("aria-label", img.alt);

        img.replaceWith(svg);
      })
      .catch(() => {});
  });
}
