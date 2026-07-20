export function initKeyHints() {
  window.addEventListener("keydown", (e) => {
    if (e.metaKey || e.ctrlKey || e.altKey) return;

    const tag = (e.target.tagName || "").toLowerCase();
    if (tag === "input" || tag === "textarea") return;

    const el = document.querySelector(`[data-key-hint="${e.key.toUpperCase()}"]`);
    if (el) {
      e.preventDefault();
      el.click();
    }
  });
}
