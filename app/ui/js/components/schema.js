export const SKIP_FIELDS = new Set(["type", "enabled", "metrics", "fmt", "kind", "interval_secs"]);

export function propType(prop) {
  if (prop.type) return prop.type;
  return ((prop.anyOf || []).find((a) => a.type && a.type !== "null") || {}).type;
}

export function resolveItems(schema, prop) {
  const ref = prop.items?.$ref;
  if (!ref) return prop.items || {};
  return schema.$defs?.[ref.split("/").pop()] || {};
}

export function scalarProps(schema, { strict = false } = {}) {
  const out = [];
  for (const [name, prop] of Object.entries(schema.properties || {})) {
    if (SKIP_FIELDS.has(name)) continue;
    const type = propType(prop);
    if (type === "object" || type === "array") continue;
    if (strict && (!type || type === "boolean")) continue;
    out.push([name, prop]);
  }
  return out;
}

export function connectionProps(schema) {
  return scalarProps(schema, { strict: true }).filter(([name]) => !name.endsWith("_env"));
}

export function credentialProps(schema) {
  return scalarProps(schema).filter(([name]) => name.endsWith("_env"));
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
