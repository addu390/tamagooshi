import { initChrome } from "../components/chrome.js";
import { initKeyHints } from "../components/keyhints.js";
import { initAccentPicker } from "../components/accents.js";
import { initScrollSpy } from "../components/scrollspy.js";
import { initHeroRotor } from "../components/rotor.js";
import { initSetupTabs } from "../components/tabs.js";
import { initExpandables } from "../components/expandable.js";
import { initHeroDevice } from "../iso/device.js";
import { initDemo } from "../iso/demo.js";

initChrome();
initKeyHints();
initAccentPicker();
initScrollSpy();
initSetupTabs();
initExpandables();
initHeroRotor();
initHeroDevice();
initDemo();
