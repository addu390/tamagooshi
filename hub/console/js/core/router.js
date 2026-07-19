import { $, el } from "./dom.js";

let registry = [];
let current = null;
let onChange = null;

export function activeView() {
  return current;
}

export function initRouter(groups, listener) {
  registry = groups.flatMap((group) => group.views);
  onChange = listener;

  $("nav").replaceChildren(...groups.flatMap((group) => [
    el("div", "nav-label", group.label),
    ...group.views.map((v) => {
      const link = el("a", "", v.title);
      link.href = "#" + v.id;
      return link;
    }),
  ]));

  window.addEventListener("hashchange", navigate);
  navigate();
}

export function navigate() {
  const hash = location.hash.replace("#", "");
  const view = registry.find((v) => v.id === hash) || registry[0];
  current = view;

  $("page-title").textContent = view.title;
  for (const link of $("nav").querySelectorAll("a")) {
    link.classList.toggle("active", link.hash === "#" + view.id);
  }
  for (const v of registry) {
    $("view-" + v.id).classList.toggle("hide", v.id !== view.id);
  }

  onChange?.(view);
}
