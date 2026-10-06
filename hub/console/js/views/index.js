import rules from "./rules.js";
import devices from "./devices/index.js";
import brand from "./brand/index.js";
import settings from "./settings.js";

export const NAV = [
  { label: "Monitor", views: [devices] },
  { label: "Configure", views: [brand, rules] },
  { label: "App", views: [settings] },
];

export default NAV.flatMap((group) => group.views);
