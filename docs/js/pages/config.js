import { initChrome } from "../components/chrome.js";
import { initKeyHints } from "../components/keyhints.js";
import { initAccentPicker } from "../components/accents.js";
import { initScrollSpy } from "../components/scrollspy.js";
import { initSetupTabs } from "../components/tabs.js";
import { initExpandables } from "../components/expandable.js";
import { initPickRow } from "../components/chips.js";

initChrome();
initKeyHints();
initAccentPicker();
initScrollSpy();
initSetupTabs();
initExpandables();

initPickRow("theme-picks", (d) => {
  const screen = document.getElementById("device-screen");
  screen.style.setProperty("--sm-bg", d.bg);
  screen.style.setProperty("--sm-ink", d.ink);
  screen.style.setProperty("--sm-hi", d.hi);
  document.getElementById("device-theme-name").textContent = d.theme;
});

initPickRow("mood-picks", (d) => {
  document.querySelector("#mood-screen .sm-face").textContent = d.face;
  document.getElementById("mood-word").textContent = d.mood;
});
