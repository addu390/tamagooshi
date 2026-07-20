export function selectTab(tabs, key, id) {
  tabs.forEach((tab) => {
    const on = tab.dataset[key] === id;
    tab.classList.toggle("active", on);
    tab.setAttribute("aria-selected", String(on));
  });
}

export function initSetupTabs() {
  document.querySelectorAll(".setup").forEach((box) => {
    const tabs = [...box.querySelectorAll(".tabbar .tab")];
    const panes = [...box.querySelectorAll(".setup-pane")];

    const select = (id) => {
      selectTab(tabs, "setup", id);
      panes.forEach((p) => { p.hidden = p.dataset.pane !== id; });
    };
    tabs.forEach((tab) => tab.addEventListener("click", () => select(tab.dataset.setup)));

    box.querySelectorAll("[data-setup-link]").forEach((link) =>
      link.addEventListener("click", (e) => { e.preventDefault(); select(link.dataset.setupLink); }));
  });
}
