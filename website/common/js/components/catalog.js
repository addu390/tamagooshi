const svg = (paths) =>
  `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">${paths}</svg>`;

const ICONS = {
  imu: svg('<path d="m2 8 2 2-2 2 2 2-2 2"/><path d="m22 8-2 2 2 2-2 2 2 2"/><rect width="8" height="14" x="8" y="5" rx="1"/>'),
  mic: svg('<path d="M12 2a3 3 0 0 0-3 3v7a3 3 0 0 0 6 0V5a3 3 0 0 0-3-3Z"/><path d="M19 10v2a7 7 0 0 1-14 0v-2"/><line x1="12" x2="12" y1="19" y2="22"/>'),
  gamepad: svg('<line x1="6" x2="10" y1="11" y2="11"/><line x1="8" x2="8" y1="9" y2="13"/><line x1="15" x2="15.01" y1="12" y2="12"/><line x1="18" x2="18.01" y1="10" y2="10"/><path d="M17.32 5H6.68a4 4 0 0 0-3.978 3.59c-.006.052-.01.101-.017.152C2.604 9.416 2 14.456 2 16a3 3 0 0 0 3 3c1 0 1.5-.5 2-1l1.414-1.414A2 2 0 0 1 9.828 16h4.344a2 2 0 0 1 1.414.586L17 18c.5.5 1 1 2 1a3 3 0 0 0 3-3c0-1.545-.604-6.584-.685-7.258-.007-.05-.011-.1-.017-.151A4 4 0 0 0 17.32 5z"/>'),
  ir: svg('<path d="M4.9 19.1C1 15.2 1 8.8 4.9 4.9"/><path d="M7.8 16.2c-2.3-2.3-2.3-6.1 0-8.5"/><circle cx="12" cy="12" r="2"/><path d="M16.2 7.8c2.3 2.3 2.3 6.1 0 8.5"/><path d="M19.1 4.9C23 8.8 23 15.2 19.1 19.1"/>'),
};

const ITEM_ICONS = {
  galaxy: "imu",
  scream: "mic",
  level: "imu",
  pomodoro: "imu",
  controller: "gamepad",
  remote: "ir",
};

function item([id, label], n) {
  const row = document.createElement("div");
  row.className = "catitem";
  const num = document.createElement("span");
  num.className = "walk-n";
  num.textContent = String(n);
  const code = document.createElement("code");
  code.textContent = id;
  const span = document.createElement("span");
  span.className = "catitem-label";
  span.textContent = label.replace(/\s*\((imu|mic)\)\s*$/i, "");
  row.append(num, code, span);
  const key = ITEM_ICONS[id];
  if (key) {
    const icon = document.createElement("span");
    icon.className = "catitem-sensor";
    icon.title = key.toUpperCase();
    icon.innerHTML = ICONS[key];
    row.append(icon);
  }
  return row;
}

export function initCatalogLists(catalog) {
  document.querySelectorAll("[data-catalog]").forEach((grid) => {
    const entries = catalog[grid.dataset.catalog] || [];
    entries.forEach((entry, i) => grid.append(item(entry, i + 1)));
  });
  document.querySelectorAll("[data-count]").forEach((el) => {
    el.textContent = String((catalog[el.dataset.count] || []).length);
  });
}
