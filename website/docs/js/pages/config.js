import { initChrome } from "../../../common/js/components/chrome.js";
import { initKeyHints } from "../../../common/js/components/keyhints.js";
import { initAccentPicker } from "../../../common/js/components/accents.js";
import { initScrollSpy } from "../../../common/js/components/scrollspy.js";
import { initSetupTabs } from "../../../common/js/components/tabs.js";
import { initExpandables } from "../../../common/js/components/expandable.js";
import { initPickRow } from "../../../common/js/components/chips.js";
import { INVADER, INVADER_MOODS, SQUIRCLE, spriteSvg } from "../../../common/js/components/sprite.js";

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

const MASCOT_SPRITES = { gooshi: INVADER, shapey: SQUIRCLE };

initPickRow("mascot-picks", (d) => {
  document.querySelector("#device-screen .sm-face").innerHTML = spriteSvg(MASCOT_SPRITES[d.mascot] || INVADER);
  document.querySelector("#device-screen .sm-mascot").textContent = d.mascot.toUpperCase();
});

initPickRow("mood-picks", (d) => {
  document.querySelector("#mood-screen .sm-face").innerHTML = spriteSvg(INVADER_MOODS[d.mood] || INVADER);
  document.getElementById("mood-word").textContent = d.mood;
});
