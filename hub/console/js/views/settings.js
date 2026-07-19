import { $, el } from "../core/dom.js";
import { prefs, setPref } from "../core/prefs.js";
import { segControl, switchControl } from "../components/controls.js";

const SETTINGS = [
  { pref: "theme", group: "set-appearance", kind: "seg", title: "Theme",
    desc: "Follow the system, or force light or dark.",
    options: ["system", "light", "dark"] },
  { pref: "contrast", group: "set-appearance", kind: "switch",
    title: "High-contrast dark theme",
    desc: "Use a darker, near-black background when dark mode is on.",
    options: ["normal", "high"] },
  { pref: "font", group: "set-interface", kind: "seg", title: "Interface font",
    desc: "Gooshi pairs Fredoka headings with Inter.",
    options: ["gooshi", "system"], labels: { gooshi: "Gooshi" } },
  { pref: "text", group: "set-interface", kind: "seg", title: "Text size",
    desc: "Size of all dashboard text.",
    options: ["small", "medium", "large"] },
  { pref: "density", group: "set-interface", kind: "seg", title: "Density",
    desc: "Spacing of rows and navigation.",
    options: ["comfortable", "compact"] },
];

function optionLabel(setting, opt) {
  return (setting.labels || {})[opt] || opt.charAt(0).toUpperCase() + opt.slice(1);
}

function pick(key, value) {
  setPref(key, value);
  render();
}

function control(s) {
  if (s.kind === "switch") {
    const [off, on] = s.options;
    return switchControl(prefs[s.pref] === on,
                         () => pick(s.pref, prefs[s.pref] === on ? off : on));
  }
  return segControl(s.options, prefs[s.pref], (opt) => optionLabel(s, opt),
                    (opt) => pick(s.pref, opt));
}

function render() {
  $("set-appearance").replaceChildren();
  $("set-interface").replaceChildren();

  for (const s of SETTINGS) {
    const row = el("div", "set-row");
    const info = el("div", "set-info");
    info.append(el("div", "set-title", s.title), el("div", "set-desc", s.desc));

    row.append(info, control(s));
    $(s.group).append(row);
  }
}

export default { id: "settings", title: "Settings", init: render, load: () => null, render: () => {} };
