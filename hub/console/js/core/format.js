const SEVERITY = { warn: "warn", warning: "warn", crit: "crit", critical: "crit" };
const OP = { lt: "<", lte: "≤", gt: ">", gte: "≥", eq: "=", ne: "≠" };

export const TREND_ARROW = { up: "▲", down: "▼", flat: "—" };

export function severityClass(s) {
  return SEVERITY[String(s || "").toLowerCase()] || "warn";
}

export function opSymbol(op) {
  return OP[op] || op;
}

export function ago(epoch) {
  if (!epoch) return "never";
  const s = Math.max(0, Math.floor(Date.now() / 1000 - epoch));
  if (s < 60) return s + "s ago";
  if (s < 3600) return Math.floor(s / 60) + "m ago";
  return Math.floor(s / 3600) + "h ago";
}

export function plural(n, word) {
  if (n === 1) return `1 ${word}`;
  return `${n} ${word.endsWith("y") ? word.slice(0, -1) + "ies" : word + "s"}`;
}

export function condition(when) {
  return `${when.metric} ${opSymbol(when.op)} ${when.value}`;
}
