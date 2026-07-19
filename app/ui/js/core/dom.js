export const $ = (id) => document.getElementById(id);

export function el(tag, cls, text) {
  const node = document.createElement(tag);
  if (cls) node.className = cls;
  if (text !== undefined) node.textContent = text;
  return node;
}

export function renderList(root, emptyEl, items, build) {
  root.replaceChildren(...items.map(build));
  emptyEl?.classList.toggle("hide", items.length > 0);
}

export function showResult(node, text, isError) {
  node.textContent = text;
  node.classList.remove("hide");
  node.classList.toggle("err", Boolean(isError));
}
