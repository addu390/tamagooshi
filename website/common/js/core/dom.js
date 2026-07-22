export function el(tag, attrs, kids) {
  const n = document.createElement(tag);

  if (attrs) for (const k in attrs) {
    if (k === "class") n.className = attrs[k];
    else if (k === "text") n.textContent = attrs[k];
    else n.setAttribute(k, attrs[k]);
  }

  (kids || []).forEach((c) => n.appendChild(c));
  return n;
}
