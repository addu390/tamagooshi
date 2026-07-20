function initPicks(rowId, apply) {
  const row = document.getElementById(rowId);
  if (!row) return;
  const chips = [...row.querySelectorAll(".pickchip")];
  chips.forEach((chip) => chip.addEventListener("click", () => {
    chips.forEach((c) => c.classList.toggle("on", c === chip));
    apply(chip.dataset);
  }));
}

initPicks("theme-picks", (d) => {
  const screen = document.getElementById("device-screen");
  screen.style.setProperty("--sm-bg", d.bg);
  screen.style.setProperty("--sm-ink", d.ink);
  screen.style.setProperty("--sm-hi", d.hi);
  document.getElementById("device-theme-name").textContent = d.theme;
});

initPicks("mood-picks", (d) => {
  document.querySelector("#mood-screen .sm-face").textContent = d.face;
  document.getElementById("mood-word").textContent = d.mood;
});
