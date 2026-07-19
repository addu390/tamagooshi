const PREF_DEFAULTS = { theme: "system", contrast: "normal", font: "gooshi",
                        text: "medium", density: "comfortable" };

export const prefs = { ...PREF_DEFAULTS, ...JSON.parse(localStorage.getItem("prefs") || "{}") };
export const darkQuery = window.matchMedia("(prefers-color-scheme: dark)");

export function applyPrefs() {
  const root = document.documentElement;
  const dark = prefs.theme === "dark" || (prefs.theme === "system" && darkQuery.matches);
  root.dataset.theme = dark ? "dark" : "light";
  root.dataset.contrast = prefs.contrast;
  root.dataset.font = prefs.font;
  root.dataset.text = prefs.text;
  root.dataset.density = prefs.density;
}

export function setPref(key, value) {
  prefs[key] = value;
  localStorage.setItem("prefs", JSON.stringify(prefs));
  applyPrefs();
}
