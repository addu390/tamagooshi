export function propType(prop) {
  if (prop.type) return prop.type;
  return ((prop.anyOf || []).find((a) => a.type && a.type !== "null") || {}).type;
}

export function readFields(root) {
  const out = {};
  for (const input of root.querySelectorAll("input.field[data-name], select.field[data-name]")) {
    if (!input.value) continue;
    out[input.dataset.name] = input.dataset.kind === "number"
      ? Number(input.value) : input.value;
  }
  return out;
}
