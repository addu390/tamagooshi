import { initChrome } from "../components/chrome.js";
import { initKeyHints } from "../components/keyhints.js";
import { initAccentPicker } from "../components/accents.js";
import { initScrollSpy } from "../components/scrollspy.js";
import { initSetupTabs } from "../components/tabs.js";
import { initExpandables } from "../components/expandable.js";
import { initPickRow } from "../components/chips.js";
import { INVADER, INVADER_MOODS, spriteSvg } from "../components/sprite.js";

initChrome();
initKeyHints();
initAccentPicker();
initScrollSpy();
initSetupTabs();
initExpandables();

document.querySelectorAll(".sm-face").forEach((face) => {
  face.innerHTML = spriteSvg(INVADER_MOODS[face.dataset.mood] || INVADER);
});

initPickRow("theme-picks", (d) => {
  const screen = document.getElementById("device-screen");
  screen.style.setProperty("--sm-bg", d.bg);
  screen.style.setProperty("--sm-ink", d.ink);
  screen.style.setProperty("--sm-hi", d.hi);
  document.getElementById("device-theme-name").textContent = d.theme;
});

initPickRow("mood-picks", (d) => {
  document.querySelector("#mood-screen .sm-face").innerHTML = spriteSvg(INVADER_MOODS[d.mood] || INVADER);
  document.getElementById("mood-word").textContent = d.mood;
});
