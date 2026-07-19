import overview from "./overview.js";
import sources from "./sources/index.js";
import rules from "./rules.js";
import agents from "./agents.js";
import devices from "./devices/index.js";
import brand from "./brand/index.js";
import settings from "./settings.js";

export const NAV = [
  { label: "Monitor", views: [overview, devices] },
  { label: "Configure", views: [brand, sources, rules, agents] },
  { label: "App", views: [settings] },
];

export default NAV.flatMap((group) => group.views);
