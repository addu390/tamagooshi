import { initChrome } from "../../../common/js/components/chrome.js";
import { initKeyHints } from "../../../common/js/components/keyhints.js";
import { initAccentPicker } from "../../../common/js/components/accents.js";
import { initScrollSpy } from "../../../common/js/components/scrollspy.js";
import { initDocFigures } from "../../../common/js/components/figures.js";
import { initExpandables } from "../../../common/js/components/expandable.js";
import { initSetupTabs } from "../../../common/js/components/tabs.js";

initChrome();
initKeyHints();
initAccentPicker();
initScrollSpy();
initDocFigures();
initExpandables();
initSetupTabs();

const brandInput = document.querySelector("[data-brand-input]");
if (brandInput) {
  const paint = () => {
    const id = brandInput.value.trim() || "<id>";
    document.querySelectorAll("[data-brand-slot]").forEach((el) => {
      el.textContent = id;
    });
  };
  const stop = (e) => e.stopPropagation();
  brandInput.addEventListener("click", stop);
  brandInput.addEventListener("mousedown", stop);
  brandInput.addEventListener("keydown", stop);
  brandInput.addEventListener("input", paint);
  paint();
}
