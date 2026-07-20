export function initAccentPicker() {
  if (window.TAMA_PRESET) return;

  const themes = [
    { id: "green", accent: "#0e9f6e", strong: "#047857", soft: "#ecfdf5", line: "#d1fae5", bg: "#f0fdf7" },
    { id: "blue",  accent: "#3563b0", strong: "#284b8a", soft: "#f0f4fb", line: "#d4dff2", bg: "#f5f8fd" },
    { id: "plum",  accent: "#7a4fa8", strong: "#5f3d85", soft: "#f7f3fb", line: "#e2d6f0", bg: "#faf7fd" },
    { id: "red",   accent: "#b0463c", strong: "#8a352e", soft: "#fbf0ef", line: "#f0d3cf", bg: "#fdf6f5" },
  ];
  const host = document.querySelector(".tab-colors");

  let current = 0;
  const apply = (t, persist = true) => {
    current = themes.indexOf(t);

    const root = document.documentElement.style;
    root.setProperty("--accent", t.accent);
    root.setProperty("--accent-strong", t.strong);
    root.setProperty("--accent-soft", t.soft);
    root.setProperty("--accent-line", t.line);
    root.setProperty("--accent-bg", t.bg);

    host?.querySelectorAll(".swatch").forEach((b) => b.classList.toggle("on", b.dataset.theme === t.id));
    if (persist) try { localStorage.setItem("tama-accent", t.id); } catch {}
  };

  if (host) {
    themes.forEach((t) => {
      const b = document.createElement("button");
      b.type = "button";
      b.className = "swatch";
      b.dataset.theme = t.id;
      b.style.background = t.accent;
      b.title = t.id;
      b.setAttribute("aria-label", `${t.id} accent`);
      b.addEventListener("click", () => apply(t));
      host.appendChild(b);
    });
    window.addEventListener("tama:accent-next", () => apply(themes[(current + 1) % themes.length]));
  }

  let saved = null;
  try { saved = localStorage.getItem("tama-accent"); } catch {}
  apply(themes.find((t) => t.id === saved) || themes.find((t) => t.id === "blue"), false);
}
