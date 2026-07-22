export function initHeroRotor() {
  const rotor = document.getElementById("hero-rotor");
  if (!rotor || matchMedia("(prefers-reduced-motion: reduce)").matches) return;

  const words = ["Your mascot", "Your theme", "Your metrics", "Your alerts"];
  const modeWords = { games: "Your arcade", ai: "Your agents" };

  const probe = rotor.cloneNode();
  probe.style.cssText = "position:absolute;visibility:hidden;white-space:nowrap;";
  rotor.parentElement.appendChild(probe);
  const setWidth = () => {
    let max = 0;
    words.concat(Object.values(modeWords)).forEach((w) => { probe.textContent = w; max = Math.max(max, probe.offsetWidth); });
    rotor.style.minWidth = `${Math.ceil(max)}px`;
  };
  setWidth();
  if (document.fonts && document.fonts.ready) document.fonts.ready.then(setWidth);

  let i = 0, timer = null;
  const swap = (word) => {
    rotor.classList.add("flip-out");
    setTimeout(() => {
      rotor.textContent = word;
      rotor.classList.remove("flip-out");
      rotor.classList.add("flip-in");
      void rotor.offsetWidth;
      rotor.classList.remove("flip-in");
    }, 220);
  };
  const show = (idx) => { i = idx % words.length; swap(words[i]); };
  const stop = () => { if (timer) { clearInterval(timer); timer = null; } };
  const start = () => { stop(); timer = setInterval(() => show(i + 1), 2600); };

  window.addEventListener("tama:brandslide", (e) => { stop(); show(e.detail); });
  window.addEventListener("tama:mode", (e) => {
    const fixed = modeWords[e.detail];
    if (e.detail === "brand") { stop(); if (i !== 0) show(0); }
    else if (fixed) { stop(); swap(fixed); }
    else start();
  });
  start();
}
