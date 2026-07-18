function initScrollSpy() {
  const links = [...document.querySelectorAll(".side a")];
  const map = new Map();
  links.forEach((a) => {
    const id = a.getAttribute("href").slice(1);
    const sec = document.getElementById(id);
    if (sec) map.set(sec, a);
  });
  const obs = new IntersectionObserver(
    (entries) => {
      entries.forEach((en) => {
        if (en.isIntersecting) {
          links.forEach((l) => l.classList.remove("active"));
          map.get(en.target)?.classList.add("active");
        }
      });
    },
    { rootMargin: "-84px 0px -70% 0px", threshold: 0 }
  );
  map.forEach((_, sec) => obs.observe(sec));
}

function initHeroRotor() {
  const rotor = document.getElementById("hero-rotor");
  if (!rotor || matchMedia("(prefers-reduced-motion: reduce)").matches) return;
  const words = ["Your mascot", "Your theme", "Your metrics", "Your alerts"];
  const probe = rotor.cloneNode();
  probe.style.cssText = "position:absolute;visibility:hidden;white-space:nowrap;";
  rotor.parentElement.appendChild(probe);
  const setWidth = () => {
    let max = 0;
    words.forEach((w) => { probe.textContent = w; max = Math.max(max, probe.offsetWidth); });
    rotor.style.minWidth = `${Math.ceil(max)}px`;
  };
  setWidth();
  if (document.fonts && document.fonts.ready) document.fonts.ready.then(setWidth);
  let i = 0, timer = null;
  const show = (idx) => {
    rotor.classList.add("flip-out");
    setTimeout(() => {
      i = idx % words.length;
      rotor.textContent = words[i];
      rotor.classList.remove("flip-out");
      rotor.classList.add("flip-in");
      void rotor.offsetWidth;
      rotor.classList.remove("flip-in");
    }, 220);
  };
  const stop = () => { if (timer) { clearInterval(timer); timer = null; } };
  const start = () => { stop(); timer = setInterval(() => show(i + 1), 2600); };
  window.addEventListener("tama:brandslide", (e) => { stop(); show(e.detail); });
  window.addEventListener("tama:mode", (e) => {
    if (e.detail === "brand") { stop(); if (i !== 0) show(0); }
    else start();
  });
  start();
}

function initAccentPicker() {
  const host = document.querySelector(".tab-colors");
  if (!host) return;
  const themes = [
    { id: "green", accent: "#0e9f6e", strong: "#047857", soft: "#ecfdf5", line: "#d1fae5", bg: "#f0fdf7" },
    { id: "blue",  accent: "#3563b0", strong: "#284b8a", soft: "#f0f4fb", line: "#d4dff2", bg: "#f5f8fd" },
    { id: "plum",  accent: "#7a4fa8", strong: "#5f3d85", soft: "#f7f3fb", line: "#e2d6f0", bg: "#faf7fd" },
    { id: "red",   accent: "#b0463c", strong: "#8a352e", soft: "#fbf0ef", line: "#f0d3cf", bg: "#fdf6f5" },
  ];
  let current = 0;
  const apply = (t) => {
    current = themes.indexOf(t);
    const root = document.documentElement.style;
    root.setProperty("--accent", t.accent);
    root.setProperty("--accent-strong", t.strong);
    root.setProperty("--accent-soft", t.soft);
    root.setProperty("--accent-line", t.line);
    root.setProperty("--accent-bg", t.bg);
    host.querySelectorAll(".swatch").forEach((b) => b.classList.toggle("on", b.dataset.theme === t.id));
    try { localStorage.setItem("tama-accent", t.id); } catch {}
  };
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
  let saved = null;
  try { saved = localStorage.getItem("tama-accent"); } catch {}
  apply(themes.find((t) => t.id === saved) || themes[0]);
}

function initExpandables() {
  document.querySelectorAll(".expandable > .expand-btn").forEach((btn) => {
    btn.addEventListener("click", () => {
      const collapsed = btn.parentElement.toggleAttribute("data-collapsed");
      btn.textContent = collapsed ? "Expand" : "Collapse";
      btn.setAttribute("aria-expanded", String(!collapsed));
    });
  });
}

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

initScrollSpy();
initHeroRotor();
initAccentPicker();
initExpandables();
