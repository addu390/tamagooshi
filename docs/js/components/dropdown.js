import { el } from "../core/dom.js";

const normalize = (opts) => opts.map((o) => (Array.isArray(o) ? o : [o, o]));

export const closeMenus = () =>
  document.querySelectorAll(".ms.open").forEach((w) => w.classList.remove("open"));

document.addEventListener("click", (e) => { if (!e.target.closest(".ms")) closeMenus(); });
document.addEventListener("keydown", (e) => { if (e.key === "Escape") closeMenus(); });

const optionButton = (val, txt) => {
  const opt = el("button", { class: "ms-opt", type: "button" }, [
    el("span", { class: "ms-check" }),
    el("span", { class: "ms-opt-l", text: val }),
  ]);

  if (txt && txt !== val) opt.appendChild(el("small", { class: "ms-opt-d", text: txt }));
  return opt;
};

function shell(variant, ariaLabel) {
  const label = el("span", { class: "ms-label" });
  const trigger = el("button", { class: "ms-trigger", type: "button" }, [label, el("span", { class: "ms-caret" })]);
  if (ariaLabel) trigger.setAttribute("aria-label", ariaLabel);

  const list = el("div", { class: "ms-list" });
  const wrap = el("div", { class: "ms" + (variant ? " " + variant : "") }, [trigger, el("div", { class: "ms-panel" }, [list])]);

  return { wrap, trigger, label, list };
}

function wireTrigger(wrap, trigger, onOpen) {
  trigger.addEventListener("click", (e) => {
    e.preventDefault();
    e.stopPropagation();

    const open = wrap.classList.contains("open");
    closeMenus();

    if (!open) {
      if (onOpen) onOpen();
      wrap.classList.add("open");
    }
  });
}

export function singleSelect(optsFn, get, set, { variant, ariaLabel } = {}) {
  const { wrap, trigger, label, list } = shell(variant, ariaLabel);

  const build = () => {
    list.innerHTML = "";
    const cur = get();
    let curTxt = "";

    normalize(optsFn()).forEach(([val, txt]) => {
      if (val === cur) curTxt = txt;

      const opt = optionButton(val, txt);
      opt.classList.toggle("on", val === cur);
      opt.addEventListener("click", (e) => {
        e.preventDefault();
        set(val);
        wrap.classList.remove("open");
        build();
      });

      list.appendChild(opt);
    });

    label.textContent = curTxt || cur || "";
  };
  build();

  wireTrigger(wrap, trigger, build);
  wrap._fill = build;
  return wrap;
}

export function multiSelect(items, getSelected, toggle, { variant, ariaLabel } = {}) {
  const norm = normalize(items);
  const { wrap, trigger, label, list } = shell(variant, ariaLabel);

  const summary = () => {
    const picked = norm.map((o) => o[0]).filter((v) => getSelected().includes(v));

    if (!picked.length) return "None selected";
    if (picked.length <= 3) return picked.join(", ");
    return picked.length + " selected";
  };

  norm.forEach(([val, desc]) => {
    const opt = optionButton(val, desc);
    const sync = () => opt.classList.toggle("on", getSelected().includes(val));
    sync();

    opt.addEventListener("click", (e) => {
      e.preventDefault();
      toggle(val);
      sync();
      label.textContent = summary();
    });

    list.appendChild(opt);
  });

  wireTrigger(wrap, trigger);
  label.textContent = summary();
  return wrap;
}
