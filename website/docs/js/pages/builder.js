import CATALOG from "../gen/catalog.js";
import { el } from "../../../common/js/core/dom.js";
import { singleSelect, multiSelect } from "../../../common/js/components/dropdown.js";
import { initChrome } from "../../../common/js/components/chrome.js";
import { initAccentPicker } from "../../../common/js/components/accents.js";
import { initKeyHints } from "../../../common/js/components/keyhints.js";

initChrome();
initAccentPicker();
initKeyHints();

const root = document.getElementById("cfg-root");

const THEMES = CATALOG.themes.map((t) => t[0]);
const TYPEFACES = CATALOG.typefaces;
const GAMES = CATALOG.games;
const APPS = CATALOG.apps;
const AGENTS = CATALOG.agents || [];
const PACKS = CATALOG.packs;
const BOARDS = CATALOG.boards;
const DEFAULT_LOGO = (window.TAMA_PRESET || {}).logo || CATALOG.logo || null;

const TRANSPORTS = CATALOG.transports || { links: [["ble", ""]], protocols: { ble: [["gatt", ""]] } };
const LINKS = TRANSPORTS.links;
const LINK_PROTOS = TRANSPORTS.protocols || {};
const protoIds = (link) => (LINK_PROTOS[link] || []).map((p) => p[0]);

const MOODS = ["happy", "neutral", "sick", "panic", "celebrate", "sleepy"];
const OPS = [
  ["lt", "< less than"],
  ["lte", "\u2264 at most"],
  ["gt", "> greater than"],
  ["gte", "\u2265 at least"],
  ["eq", "= equals"],
  ["ne", "\u2260 not equal"],
];
const SEVERITIES = ["info", "warning", "critical"];
const KINDS = ["normal", "star"];
const SOURCE_TYPES = ["demo", "datadog", "posthog"];

const TZ = [
  "-12:00", "-11:00", "-10:00", "-09:30", "-09:00", "-08:00", "-07:00", "-06:00", "-05:00",
  "-04:00", "-03:30", "-03:00", "-02:00", "-01:00", "+00:00", "+01:00", "+02:00", "+03:00",
  "+03:30", "+04:00", "+04:30", "+05:00", "+05:30", "+05:45", "+06:00", "+06:30", "+07:00",
  "+08:00", "+08:45", "+09:00", "+09:30", "+10:00", "+10:30", "+11:00", "+12:00", "+12:45",
  "+13:00", "+14:00",
];

const ids = (pairs) => pairs.map((p) => p[0]);
const prefer = (list, want) => (list.includes(want) ? want : list[0] || "");
const packMemberIds = () => Object.values(PACKS).flat().map((m) => m[0]);

const state = {
  id: "gooshi",
  name: "TAMAGOOSHI",
  mascot: "GOOSHI",
  board: BOARDS[0].id,
  tagline: "keep it alive",
  website: "gooshi.me",
  logo: DEFAULT_LOGO ? (DEFAULT_LOGO.name || "logo.png") : "",
  logoMask: DEFAULT_LOGO ? { w: DEFAULT_LOGO.w, h: DEFAULT_LOGO.h, bits: DEFAULT_LOGO.bits } : null,
  timezone: "",

  themes: THEMES.slice(),
  themeDefault: prefer(THEMES, "midnight"),
  typefaces: ids(TYPEFACES),
  typefaceDefault: prefer(ids(TYPEFACES), "pixel"),
  packs: Object.keys(PACKS),
  mascotDefault: prefer(packMemberIds(), "cat"),
  mood: "happy",

  games: ids(GAMES),
  apps: ids(APPS),
  buddy: true,
  agents: ids(AGENTS),
  agentDefault: prefer(ids(AGENTS), "cursor"),

  links: LINKS.reduce((m, [l]) => ((m[l] = l === "ble"), m), {}),
  linkProto: LINKS.reduce((m, [l]) => ((m[l] = protoIds(l)[0] || ""), m), {}),

  customThemes: [],
  sources: [],
  moods: [],
  alerts: [],
};
Object.assign(state, (window.TAMA_PRESET || {}).config || {});

const orderBy = (order, picked) => order.filter((x) => picked.includes(x));
const packMembers = () => state.packs.flatMap((p) => PACKS[p] || []);

const reqMark = () => el("span", { class: "cfg-req", text: "*", title: "Required" });
const fieldLabel = (text, req) => {
  const l = el("label", { text });
  if (req) l.appendChild(reqMark());
  return l;
};

const groups = [];
function group(title, bodyNodes, req) {
  const body = el("div", { class: "cfg-body" }, bodyNodes);
  const titleSpan = el("span", { text: title });
  if (req) titleSpan.appendChild(reqMark());

  const head = el("button", { class: "cfg-head", type: "button" }, [
    titleSpan,
    el("span", { class: "cfg-chev" }),
  ]);
  const g = el("div", { class: "cfg-group" }, [head, body]);
  head.addEventListener("click", () => g.classList.toggle("open"));

  groups.push({ g, body });
  return g;
}

function boundInput(obj, key, ph) {
  const input = el("input", { type: "text", value: obj[key] == null ? "" : obj[key], placeholder: ph || "" });
  input.addEventListener("input", () => { obj[key] = input.value; render(); });
  return input;
}

function boundSelect(obj, key, opts) {
  return singleSelect(() => opts, () => obj[key], (v) => { obj[key] = v; render(); });
}

const duo = (a, b) => el("div", { class: "cfg-duo" }, [a, b]);
const cell = (label, control) => el("div", { class: "cfg-cell" }, [el("span", { text: label }), control]);
const delBtn = (onClick) => {
  const b = el("button", { class: "cfg-del", type: "button", title: "Remove", text: "\u00d7" });
  b.addEventListener("click", onClick);
  return b;
};
const addBtn = (label, onClick) => {
  const b = el("button", { class: "cfg-add", type: "button", text: "+ " + label });
  b.addEventListener("click", onClick);
  return b;
};

const hexToBytes = (hex) => {
  const a = new Uint8Array(Math.floor(hex.length / 2));
  for (let i = 0; i < a.length; i++) a[i] = parseInt(hex.substr(i * 2, 2), 16);
  return a;
};

const isInk = (r, g, b, a) => a >= 128 && 0.299 * r + 0.587 * g + 0.114 * b < 200;

function rasterizeLogo(img, target) {
  const t = target || 24;
  const src = document.createElement("canvas");
  src.width = img.naturalWidth;
  src.height = img.naturalHeight;
  const sctx = src.getContext("2d");
  sctx.drawImage(img, 0, 0);
  const px = sctx.getImageData(0, 0, src.width, src.height).data;

  let x0 = src.width, y0 = src.height, x1 = -1, y1 = -1;
  for (let y = 0; y < src.height; y++) {
    for (let x = 0; x < src.width; x++) {
      const i = (y * src.width + x) * 4;
      if (isInk(px[i], px[i + 1], px[i + 2], px[i + 3])) {
        if (x < x0) x0 = x; if (y < y0) y0 = y; if (x > x1) x1 = x; if (y > y1) y1 = y;
      }
    }
  }
  if (x1 < 0) return null;

  const bw = x1 - x0 + 1, bh = y1 - y0 + 1;
  const s = t / Math.max(bw, bh);
  const w = Math.max(1, Math.round(bw * s));
  const h = Math.max(1, Math.round(bh * s));
  const small = document.createElement("canvas");
  small.width = w; small.height = h;
  const dctx = small.getContext("2d");
  dctx.imageSmoothingEnabled = true;
  dctx.drawImage(src, x0, y0, bw, bh, 0, 0, w, h);
  const dp = dctx.getImageData(0, 0, w, h).data;

  const bytes = new Uint8Array(Math.ceil((w * h) / 8));
  for (let i = 0; i < w * h; i++) {
    const j = i * 4;
    if (isInk(dp[j], dp[j + 1], dp[j + 2], dp[j + 3])) bytes[i >> 3] |= 0x80 >> (i & 7);
  }
  const bits = Array.from(bytes, (v) => v.toString(16).padStart(2, "0")).join("");
  return { w, h, bits };
}

function logoField() {
  const wrap = el("div", { class: "cfg-field" }, [fieldLabel("Logo image")]);
  const preview = el("canvas", { class: "cfg-logo-prev", width: "48", height: "48" });

  const input = el("input", { type: "file", accept: "image/*" });
  const pick = el("button", { class: "cfg-file-btn", type: "button", text: "Choose file" });
  const fname = el("span", { class: "cfg-file-name", text: "No file chosen" });
  const clear = el("button", { class: "cfg-logo-clear", type: "button", text: "Clear" });
  pick.addEventListener("click", () => input.click());

  const drawPreview = () => {
    const ctx = preview.getContext("2d");
    ctx.clearRect(0, 0, preview.width, preview.height);
    if (!state.logoMask) return;

    const { w, h, bits } = state.logoMask;
    const bytes = hexToBytes(bits);
    const scale = Math.max(1, Math.floor(Math.min(preview.width / w, preview.height / h)));
    const ox = Math.floor((preview.width - w * scale) / 2);
    const oy = Math.floor((preview.height - h * scale) / 2);

    ctx.fillStyle = "#18181b";
    for (let i = 0; i < w * h; i++) {
      if ((bytes[i >> 3] >> (7 - (i & 7))) & 1) {
        ctx.fillRect(ox + (i % w) * scale, oy + Math.floor(i / w) * scale, scale, scale);
      }
    }
  };

  input.addEventListener("change", () => {
    const file = input.files && input.files[0];
    if (!file) return;

    fname.textContent = file.name;
    const img = new Image();
    img.onload = () => { state.logoMask = rasterizeLogo(img); drawPreview(); render(); };
    img.src = URL.createObjectURL(file);
  });
  clear.addEventListener("click", () => {
    state.logoMask = null;
    input.value = "";
    fname.textContent = "No file chosen";
    drawPreview();
    render();
  });

  wrap.appendChild(el("div", { class: "cfg-logo" }, [preview, el("div", { class: "cfg-logo-ctrl" }, [pick, fname, input, clear])]));
  wrap.appendChild(el("small", { class: "cfg-hint", text: "Defaults to the brand logo shown above. Pick a file to bake your own into the browser-flash config. Local builds use the Logo path." }));
  drawPreview();
  return wrap;
}

function textField(label, key, placeholder, req) {
  return el("div", { class: "cfg-field" }, [fieldLabel(label, req), boundInput(state, key, placeholder)]);
}

function transportsField() {
  const rows = el("div", { class: "cfg-sub" });

  LINKS.forEach(([link, desc]) => {
    const base = link === "ble";
    const cb = el("input", { type: "checkbox" });
    cb.checked = state.links[link];
    cb.disabled = base;
    cb.addEventListener("change", () => { state.links[link] = cb.checked; render(); });

    const head = el("span", { class: "cfg-transport-name", text: link.toUpperCase() });
    const kids = [cb, head];
    if (desc) kids.push(el("small", { text: desc }));
    kids.push(el("div", { class: "cfg-spacer" }));

    const protos = LINK_PROTOS[link] || [];
    if (protos.length > 1) {
      kids.push(singleSelect(() => protos, () => state.linkProto[link], (v) => { state.linkProto[link] = v; render(); }));
    } else if (protos.length === 1) {
      kids.push(el("span", { class: "cfg-proto", text: protos[0][0] }));
    }

    rows.appendChild(el("div", { class: "cfg-transport" }, kids));
  });

  return el("div", { class: "cfg-field" }, [fieldLabel("Transports"), rows]);
}

function selectField(label, opts, key, req) {
  const ss = singleSelect(opts, () => state[key], (v) => { state[key] = v; render(); });
  const field = el("div", { class: "cfg-field" }, [fieldLabel(label, req), ss]);
  field._fill = ss._fill;
  return field;
}

function multiselect(items, arrKey, onChange) {
  return multiSelect(items, () => state[arrKey], (val) => {
    if (state[arrKey].includes(val)) state[arrKey] = state[arrKey].filter((x) => x !== val);
    else state[arrKey].push(val);
    if (onChange) onChange();
    render();
  });
}

const customThemeNames = () => state.customThemes.map((t) => String(t.name || "").trim()).filter(Boolean);
const themeIds = () => THEMES.concat(customThemeNames());

const themeDefault = selectField("Default", () => orderBy(themeIds(), state.themes), "themeDefault");
const typefaceDefault = selectField("Default", () => orderBy(TYPEFACES.map((t) => t[0]), state.typefaces), "typefaceDefault");
const mascotDefault = selectField("Default", () => packMembers(), "mascotDefault", true);
const agentDefault = selectField("Default agent", () => orderBy(AGENTS.map((a) => a[0]), state.agents), "agentDefault");

function reconcile() {
  const th = orderBy(themeIds(), state.themes);
  if (!th.includes(state.themeDefault)) state.themeDefault = th[0] || "";

  const tf = orderBy(TYPEFACES.map((t) => t[0]), state.typefaces);
  if (!tf.includes(state.typefaceDefault)) state.typefaceDefault = tf[0] || "";

  const mm = packMembers().map((m) => m[0]);
  if (!mm.includes(state.mascotDefault)) state.mascotDefault = mm[0] || "";

  const ag = orderBy(AGENTS.map((a) => a[0]), state.agents);
  if (!ag.includes(state.agentDefault)) state.agentDefault = ag[0] || "";

  themeDefault._fill();
  typefaceDefault._fill();
  mascotDefault._fill();
  agentDefault._fill();
}

const themeEnabledField = el("div", { class: "cfg-field" });
function rebuildThemeEnabled() {
  themeEnabledField.innerHTML = "";
  themeEnabledField.appendChild(fieldLabel("Enabled"));
  themeEnabledField.appendChild(multiselect(themeIds(), "themes", reconcile));
}

function colorCell(obj, key, onInput) {
  const input = el("input", { type: "color", value: obj[key] });
  input.addEventListener("input", () => { obj[key] = input.value; onInput(); render(); });
  return cell(key, input);
}

function customThemeCard(t, onRemove) {
  const preview = el("span", { class: "cfg-theme-prev" }, [el("b", { text: "Aa" }), el("i")]);
  const paint = () => {
    preview.style.background = t.surface;
    preview.firstChild.style.color = t.ink;
    preview.lastChild.style.background = t.accent;
  };
  paint();

  const name = el("input", { type: "text", value: t.name, placeholder: "clay" });
  name.addEventListener("input", () => {
    const prev = String(t.name || "").trim();
    const next = name.value.trim();
    t.name = name.value;

    state.themes = state.themes.map((x) => (x === prev ? next : x)).filter(Boolean);
    if (next && !state.themes.includes(next)) state.themes.push(next);
    if (state.themeDefault === prev) state.themeDefault = next;

    rebuildThemeEnabled();
    reconcile();
    render();
  });

  return el("div", { class: "cfg-row" }, [
    cell("name", name),
    colorCell(t, "surface", paint),
    colorCell(t, "ink", paint),
    colorCell(t, "accent", paint),
    cell("preview", preview),
    delBtn(onRemove),
  ]);
}

const customThemesWrap = el("div", { class: "cfg-sub" });
function drawCustomThemes() {
  customThemesWrap.innerHTML = "";
  state.customThemes.forEach((t) => {
    customThemesWrap.appendChild(customThemeCard(t, () => {
      state.customThemes = state.customThemes.filter((x) => x !== t);
      state.themes = state.themes.filter((x) => x !== String(t.name || "").trim());
      drawCustomThemes();
      rebuildThemeEnabled();
      reconcile();
      render();
    }));
  });

  customThemesWrap.appendChild(addBtn("custom theme", () => {
    let n = 1;
    while (themeIds().includes("custom" + (n > 1 ? n : ""))) n++;
    const name = "custom" + (n > 1 ? n : "");

    state.customThemes.push({ name, surface: "#F0EEE6", ink: "#191919", accent: "#D97757" });
    state.themes.push(name);

    drawCustomThemes();
    rebuildThemeEnabled();
    reconcile();
    render();
  }));
}

rebuildThemeEnabled();
drawCustomThemes();

function buddyField() {
  const cb = el("input", { type: "checkbox" });
  cb.checked = state.buddy;
  cb.addEventListener("change", () => { state.buddy = cb.checked; render(); });
  const row = el("div", { class: "cfg-transport" }, [
    cb,
    el("span", { class: "cfg-transport-name", text: "ENABLED" }),
    el("small", { text: "voice prompts and agent session on the device (needs BLE)" }),
  ]);
  return el("div", { class: "cfg-field" }, [fieldLabel("Buddy"), el("div", { class: "cfg-sub" }, [row])]);
}

function metricRow(src, m, onRemove) {
  const cells = [cell("key", boundInput(m, "key", "mrr")), cell("label", boundInput(m, "label", "MRR"))];
  if (src.type === "demo") {
    cells.push(cell("start", boundInput(m, "value_start", "0")));
    cells.push(cell("drift", boundInput(m, "drift", "0")));
  } else if (src.type === "datadog") {
    cells.push(cell("query", boundInput(m, "query", "avg:system.cpu.user{*}")));
  } else {
    cells.push(cell("query", boundInput(m, "query", "HogQL")));
    cells.push(cell("insight", boundInput(m, "insight", "short_id")));
  }
  cells.push(cell("fmt", boundInput(m, "fmt", "{v}")));
  cells.push(cell("kind", boundSelect(m, "kind", KINDS)));
  cells.push(delBtn(onRemove));
  return el("div", { class: "cfg-row" }, cells);
}

function sourceCard(src, onRemove) {
  const metricsWrap = el("div", { class: "cfg-sub" });
  const drawMetrics = () => {
    metricsWrap.innerHTML = "";
    src.metrics.forEach((m) => {
      metricsWrap.appendChild(metricRow(src, m, () => {
        src.metrics = src.metrics.filter((x) => x !== m);
        drawMetrics();
        render();
      }));
    });
    metricsWrap.appendChild(addBtn("metric", () => {
      src.metrics.push({ key: "metric", label: "METRIC", fmt: "{v}", kind: "normal", value_start: "0", drift: "0", query: "", insight: "" });
      drawMetrics();
      render();
    }));
  };
  drawMetrics();

  const typeSel = singleSelect(() => SOURCE_TYPES, () => src.type,
                               (v) => { src.type = v; drawMetrics(); rebuild(); render(); });
  const card = el("div", { class: "cfg-card" });

  const rebuild = () => {
    card.innerHTML = "";

    const hd = el("div", { class: "cfg-card-hd" }, [cell("type", typeSel), cell("interval", boundInput(src, "interval", "3.0"))]);
    if (src.type === "datadog") { hd.appendChild(cell("site", boundInput(src, "site", "datadoghq.com"))); hd.appendChild(cell("window", boundInput(src, "window_secs", "900"))); }
    if (src.type === "posthog") { hd.appendChild(cell("host", boundInput(src, "host", "https://us.posthog.com"))); hd.appendChild(cell("project_id", boundInput(src, "project_id", "12345"))); }
    hd.appendChild(el("div", { class: "cfg-spacer" }));
    hd.appendChild(delBtn(onRemove));

    card.appendChild(hd);
    card.appendChild(el("div", { class: "cfg-card-bd" }, [metricsWrap]));
  };
  rebuild();

  return card;
}

function repeater(arrKey, makeCard, blank, addLabel) {
  const wrap = el("div", { class: "cfg-sub" });
  const draw = () => {
    wrap.innerHTML = "";
    state[arrKey].forEach((item) => {
      wrap.appendChild(makeCard(item, () => {
        state[arrKey] = state[arrKey].filter((x) => x !== item);
        draw();
        render();
      }));
    });
    wrap.appendChild(addBtn(addLabel, () => { state[arrKey].push(blank()); draw(); render(); }));
  };
  draw();
  wrap._draw = draw;
  return wrap;
}

function metricKeys() {
  const seen = [];
  state.sources.forEach((s) => s.metrics.forEach((m) => {
    const k = bare(m.key);
    if (k && !seen.includes(k)) seen.push(k);
  }));
  return seen;
}

function metricSelect(obj) {
  const keys = metricKeys();
  if (keys.length && !keys.includes(obj.metric)) obj.metric = keys[0];
  return boundSelect(obj, "metric", keys.length ? keys : [["", "(add a metric)"]]);
}

const moodRow = (md, onRemove) => el("div", { class: "cfg-row" }, [
  cell("metric", metricSelect(md)),
  cell("op", boundSelect(md, "op", OPS)),
  cell("value", boundInput(md, "value", "95")),
  cell("mood", boundSelect(md, "mood", MOODS)),
  cell("priority", boundInput(md, "priority", "0")),
  delBtn(onRemove),
]);

const alertRow = (al, onRemove) => el("div", { class: "cfg-card" }, [
  el("div", { class: "cfg-card-bd" }, [
    el("div", { class: "cfg-row" }, [
      cell("id", boundInput(al, "id", "uptime-critical")),
      cell("metric", metricSelect(al)),
      cell("op", boundSelect(al, "op", OPS)),
      cell("value", boundInput(al, "value", "95")),
      cell("severity", boundSelect(al, "severity", SEVERITIES)),
      el("div", { class: "cfg-spacer" }),
      delBtn(onRemove),
    ]),
    el("div", { class: "cfg-row" }, [
      cell("title", boundInput(al, "title", "Uptime dropping")),
      cell("body", boundInput(al, "body", "SLA at risk")),
      cell("source", boundInput(al, "source", "monitor")),
    ]),
  ]),
]);

const qs = (v) => JSON.stringify(String(v == null ? "" : v));
const num = (v) => { const s = String(v == null ? "" : v).trim(); if (s === "") return "0"; const n = Number(s); return Number.isFinite(n) ? String(n) : "0"; };
const bare = (v) => String(v == null ? "" : v).trim();
const flow = (pairs) => "{" + pairs.map(([k, v]) => k + ": " + v).join(", ") + "}";

function metricFlow(type, m) {
  const p = [["key", bare(m.key)], ["label", qs(m.label)]];
  if (type === "demo") {
    p.push(["value_start", num(m.value_start)]);
    p.push(["fmt", qs(m.fmt)]);
    if (m.kind === "star") p.push(["kind", "star"]);
    p.push(["drift", num(m.drift)]);
  } else if (type === "datadog") {
    p.push(["query", qs(m.query)]);
    p.push(["fmt", qs(m.fmt)]);
    if (m.kind === "star") p.push(["kind", "star"]);
  } else {
    if (m.insight && !m.query) p.push(["insight", qs(m.insight)]);
    else p.push(["query", qs(m.query)]);
    p.push(["fmt", qs(m.fmt)]);
    if (m.kind === "star") p.push(["kind", "star"]);
  }
  return flow(p);
}

const cond = (o) => "{metric: " + bare(o.metric) + ", op: " + o.op + ", value: " + num(o.value) + "}";
const moodFlow = (md) => flow([["when", cond(md)], ["mood", md.mood], ["priority", num(md.priority)]]);
const alertFlow = (al) => "{id: " + bare(al.id) + ", when: " + cond(al) + ", severity: " + al.severity +
  ", title: " + qs(al.title) + ", body: " + qs(al.body) + ", source: " + bare(al.source) + "}";

function yaml() {
  const list = (a) => "[" + a.join(", ") + "]";
  let o = "";

  o += "brand:\n";
  o += "  id: " + state.id + "\n";
  o += "  name: " + state.name + "\n";
  o += "  mascot: " + state.mascot + "\n";
  o += "  tagline: " + state.tagline + "\n";
  o += "  website: " + state.website + "\n";
  if (state.logo.trim()) o += "  logo: " + state.logo.trim() + "\n";
  o += "\n";

  o += "device:\n";
  o += "  transports:\n";
  LINKS.forEach(([l]) => { if (state.links[l]) o += "    " + l + ": " + state.linkProto[l] + "\n"; });

  o += "  theme:\n";
  const customs = state.customThemes.filter((t) => bare(t.name));
  if (customs.length) {
    o += "    custom:\n";
    customs.forEach((t) => {
      o += "      - name: " + bare(t.name) + "\n";
      o += '        colors: {surface: "' + t.surface + '", ink: "' + t.ink + '", accent: "' + t.accent + '"}\n';
    });
  }
  o += "    default: " + state.themeDefault + "\n";
  o += "    enabled: " + list(orderBy(themeIds(), state.themes)) + "\n";

  o += "  typeface:\n";
  o += "    default: " + state.typefaceDefault + "\n";
  o += "    enabled: " + list(orderBy(TYPEFACES.map((t) => t[0]), state.typefaces)) + "\n";

  o += "  mascot:\n";
  o += "    default: " + state.mascotDefault + "\n";
  o += "    mood: " + state.mood + "\n";
  o += "    enabled: " + list(state.packs) + "\n";

  o += "  games:\n";
  o += "    enabled: " + list(orderBy(GAMES.map((g) => g[0]), state.games)) + "\n";
  o += "  apps:\n";
  o += "    enabled: " + list(orderBy(APPS.map((a) => a[0]), state.apps)) + "\n";
  o += "  buddy:\n";
  o += "    enabled: " + (state.buddy ? "true" : "false") + "\n";
  if (state.timezone.trim()) o += '  timezone: "' + state.timezone.trim() + '"\n';

  const agentSection = state.buddy && state.agents.length;
  if (agentSection || state.sources.length || state.moods.length || state.alerts.length) {
    o += "\nhub:\n";
    if (agentSection) {
      o += "  agent:\n";
      o += "    default: " + state.agentDefault + "\n";
      o += "    enabled: " + list(orderBy(AGENTS.map((a) => a[0]), state.agents)) + "\n";
    }
    if (state.sources.length) {
      o += "  sources:\n";
      state.sources.forEach((src) => {
        o += "    - type: " + src.type + "\n";
        o += "      interval_secs: " + num(src.interval) + "\n";
        if (src.type === "datadog") { o += "      site: " + qs(src.site) + "\n"; o += "      window_secs: " + num(src.window_secs) + "\n"; }
        if (src.type === "posthog") { o += "      host: " + qs(src.host) + "\n"; o += "      project_id: " + qs(src.project_id) + "\n"; }
        o += "      metrics:\n";
        src.metrics.forEach((m) => { o += "        - " + metricFlow(src.type, m) + "\n"; });
      });
    }
    if (state.moods.length) { o += "\n  moods:\n"; state.moods.forEach((md) => { o += "    - " + moodFlow(md) + "\n"; }); }
    if (state.alerts.length) { o += "\n  alerts:\n"; state.alerts.forEach((al) => { o += "    - " + alertFlow(al) + "\n"; }); }
  }
  return o;
}

const out = el("code", {});
const outName = el("span", { class: "cfg-out-name" });
const copyBtn = el("button", { class: "btn ghost", type: "button", text: "Copy" });
const downloadBtn = el("button", { class: "btn", type: "button", text: "Download" });

let moodsRep = null;
let alertsRep = null;
let lastKeys = null;
let flashSync = null;

function render() {
  const keys = metricKeys().join("\u0001");
  if (keys !== lastKeys) {
    lastKeys = keys;
    if (moodsRep && moodsRep._draw) moodsRep._draw();
    if (alertsRep && alertsRep._draw) alertsRep._draw();
  }

  out.textContent = yaml();
  outName.textContent = "brands/" + (state.id || "brand") + "/config.yaml";
  if (flashSync) flashSync();
}

copyBtn.addEventListener("click", async () => {
  try {
    await navigator.clipboard.writeText(yaml());
    copyBtn.textContent = "Copied";
    setTimeout(() => (copyBtn.textContent = "Copy"), 1400);
  } catch (e) {
    copyBtn.textContent = "Press Ctrl+C";
  }
});

downloadBtn.addEventListener("click", () => {
  const blob = new Blob([yaml()], { type: "text/yaml" });
  const url = URL.createObjectURL(blob);
  const a = el("a", { href: url, download: "config.yaml" });
  document.body.appendChild(a);
  a.click();
  a.remove();
  URL.revokeObjectURL(url);
});

const moodField = selectField("Mood", () => MOODS, "mood");
const tzField = selectField("Timezone", () => [["", "None (UTC)"], ...TZ.map((t) => [t, "UTC" + t])], "timezone");

const sourcesRep = repeater("sources", sourceCard, () => ({
  type: "demo", interval: "3.0", site: "datadoghq.com", window_secs: "900",
  host: "https://us.posthog.com", project_id: "",
  metrics: [{ key: "metric", label: "METRIC", fmt: "{v}", kind: "normal", value_start: "0", drift: "0", query: "", insight: "" }],
}), "source");
moodsRep = repeater("moods", moodRow, () => ({ metric: "", op: "lt", value: "0", mood: "sick", priority: "0" }), "mood rule");
alertsRep = repeater("alerts", alertRow, () => ({ id: "alert-id", metric: "", op: "lt", value: "0", severity: "warning", title: "Title", body: "", source: "hub" }), "alert rule");

function customMascotField() {
  return el("div", { class: "cfg-field" }, [
    fieldLabel("Custom mascot"),
    el("a", { class: "btn ghost", href: "mixer", target: "_blank", rel: "noopener",
              text: "Open the mixer \u2197" }),
    el("small", { class: "cfg-hint",
                  text: "Save the downloaded image next to your config and list it under mascot.custom." }),
  ]);
}

const form = el("div", { class: "cfg-form" }, [
  group("Brand identity", [
    textField("ID", "id", "gooshi", true),
    duo(textField("Name", "name", "TAMAGOOSHI", true), textField("Mascot name", "mascot", "GOOSHI")),
    duo(textField("Tagline", "tagline", "keep it alive"), textField("Website", "website", "gooshi.me")),
    textField("Logo", "logo", "logo.png (in your brand folder)"),
    logoField(),
  ], true),

  group("Theme", [
    themeDefault,
    themeEnabledField,
    el("div", { class: "cfg-field" }, [
      fieldLabel("Custom themes"),
      customThemesWrap,
      el("small", { class: "cfg-hint", text: "Pick surface, ink and accent; the full palette is derived from them and rides along when you flash." }),
    ]),
  ]),

  group("Typeface", [typefaceDefault, el("div", { class: "cfg-field" }, [fieldLabel("Enabled"), multiselect(TYPEFACES, "typefaces", reconcile)])]),

  group("Mascot", [
    el("div", { class: "cfg-field" }, [fieldLabel("Packs", true), multiselect(Object.keys(PACKS), "packs", reconcile)]),
    mascotDefault,
    moodField,
    customMascotField(),
  ], true),

  group("Games", [el("div", { class: "cfg-field" }, [el("label", { text: "Enabled" }), multiselect(GAMES, "games")])]),
  group("Apps", [el("div", { class: "cfg-field" }, [el("label", { text: "Enabled" }), multiselect(APPS, "apps")])]),

  group("Agent buddy", [
    buddyField(),
    agentDefault,
    el("div", { class: "cfg-field" }, [fieldLabel("Agents"), multiselect(AGENTS, "agents", reconcile)]),
  ]),

  group("Device", [transportsField(), tzField]),
  group("Sources & metrics", [sourcesRep]),
  group("Moods", [moodsRep]),
  group("Alerts", [alertsRep]),
]);

const output = el("div", { class: "cfg-out" }, [
  el("div", { class: "cfg-out-bar" }, [outName, el("div", { class: "cfg-actions" }, [copyBtn, downloadBtn])]),
  el("pre", { class: "cfg-yaml" }, [out]),
]);

groups.forEach((grp, i) => {
  if (i < groups.length - 1) {
    const next = el("button", { class: "btn cfg-next", type: "button", text: "Next" });
    next.addEventListener("click", () => {
      grp.g.classList.remove("open");
      const n = groups[i + 1].g;
      n.classList.add("open");
      n.scrollIntoView({ behavior: "smooth", block: "start" });
    });
    grp.body.appendChild(el("div", { class: "cfg-next-row" }, [next]));
  }
});
if (groups[0]) groups[0].g.classList.add("open");

const note = el("p", { class: "cfg-note" }, [
  reqMark(), el("span", { text: " marks required fields. Everything else has a sensible default." }),
]);
root.appendChild(note);
root.appendChild(form);
root.appendChild(output);

const flashRoot = document.getElementById("flash-root");
if (flashRoot) Promise.all([
  import("../gen/wire/blob.js"), import("../gen/wire/flasher.js"),
]).then(([wireBlob, flasher]) => {
  const board = () => BOARDS.find((b) => b.id === state.board) || BOARDS[0];

  const manifest = () => {
    const themes = orderBy(themeIds(), state.themes);
    return {
      brand: { name: state.name, tagline: state.tagline, website: state.website, mascot: state.mascot },
      device: {
        transports: Object.fromEntries(
          LINKS.filter(([l]) => state.links[l]).map(([l]) => [l, state.linkProto[l]])),
        timezone: state.timezone,
        theme: {
          custom: state.customThemes.filter((t) => bare(t.name)).map((t) => (
            { name: bare(t.name), colors: { surface: t.surface, ink: t.ink, accent: t.accent } })),
          default: themes.includes(state.themeDefault) ? state.themeDefault : themes[0] || "",
          enabled: themes,
        },
        typeface: { default: state.typefaceDefault, enabled: orderBy(TYPEFACES.map((t) => t[0]), state.typefaces) },
        mascot: { default: state.mascotDefault, mood: state.mood, enabled: state.packs.slice() },
        games: { enabled: orderBy(GAMES.map((g) => g[0]), state.games) },
        apps: { enabled: orderBy(APPS.map((a) => a[0]), state.apps) },
      },
    };
  };

  const configBlob = (m) => {
    const cfg = wireBlob.fromManifest(m);
    if (state.logoMask) cfg.logo = state.logoMask;
    return wireBlob.encode(cfg);
  };

  const image = (m) => ({ path: flasher.imageUrl(CATALOG.release, m, board()), offset: 0 });
  const customManifest = () => {
    const m = manifest();
    return flasher.installerManifest(board().chipFamily, [
      image(m), flasher.configPart(configBlob(m), board().configOffset),
    ]);
  };
  const stockManifest = () => flasher.installerManifest(board().chipFamily, [image(manifest())]);

  const customBtn = flasher.installButton("Flash my config", "btn", customManifest());
  const stockBtn = flasher.installButton("Flash defaults", "btn ghost", stockManifest());

  const boardSelect = singleSelect(() => BOARDS.map((b) => [b.id, b.name]), () => state.board,
                                   (v) => { state.board = v; flashSync(); },
                                   { variant: "float", ariaLabel: "Board" });

  const barFill = el("span", { class: "flash-bar-fill" });
  const barLabel = el("span", { class: "flash-bar-label", text: "Waiting for device" });
  const bar = el("div", { class: "flash-bar" }, [el("div", { class: "flash-bar-track" }, [barFill]), barLabel]);

  const logBody = el("div", { class: "flash-log-body" }, [el("div", { class: "flash-log-empty", text: "Logs will appear here." })]);
  const logBox = el("div", { class: "flash-log" }, [el("div", { class: "flash-log-bar", text: "Logs" }), logBody]);

  let logStarted = false;
  const log = (msg, kind) => {
    if (!msg) return;
    if (!logStarted) { logBody.innerHTML = ""; logStarted = true; }
    logBody.appendChild(el("div", { class: "flash-log-line" + (kind ? " " + kind : ""), text: msg }));
    logBody.scrollTop = logBody.scrollHeight;
  };
  const setBar = (pct, label, kind) => {
    barFill.style.width = Math.max(0, Math.min(100, pct)) + "%";
    barFill.className = "flash-bar-fill" + (kind ? " " + kind : "");
    barLabel.textContent = label;
  };
  const onState = (e) => {
    const s = e.detail || {};
    const st = String(s.state || "").toLowerCase();

    let pct = null;
    if (s.details) {
      if (typeof s.details.percentage === "number") pct = s.details.percentage;
      else if (s.details.bytesTotal) pct = (s.details.bytesWritten / s.details.bytesTotal) * 100;
    }

    if (st === "writing" && pct != null) setBar(pct, "Writing " + Math.round(pct) + "%");
    else if (st === "finished") setBar(100, "Done", "done");
    else if (st === "error") setBar(0, "Error", "error");
    else if (s.message) barLabel.textContent = s.message;

    log(s.message, st === "error" ? "err" : null);
  };

  const resetFlash = () => {
    logStarted = false;
    logBody.innerHTML = "";
    logBody.appendChild(el("div", { class: "flash-log-empty", text: "Logs will appear here." }));
    setBar(0, "Connecting");
  };

  const DIALOG_CSS =
    "ew-text-button{--md-sys-color-primary:var(--text);background:transparent;border:1px solid var(--border);border-radius:var(--radius);overflow:hidden}" +
    "ew-text-button:last-of-type{--md-sys-color-primary:var(--on-accent);background:var(--text);border-color:var(--text)}";
  const brandDialog = (n) => {
    if (n.__branded) return;
    if (!n.shadowRoot) { requestAnimationFrame(() => brandDialog(n)); return; }
    n.__branded = true;

    const s = document.createElement("style");
    s.textContent = DIALOG_CSS;
    n.shadowRoot.appendChild(s);
  };

  new MutationObserver((muts) => {
    muts.forEach((m) => m.addedNodes.forEach((n) => {
      const name = n.nodeName ? n.nodeName.toLowerCase() : "";
      if (!name.startsWith("ewt-")) return;

      brandDialog(n);
      if (name === "ewt-install-dialog") {
        resetFlash();
        n.addEventListener("state-changed", onState);
      }
    }));
  }).observe(document.body, { childList: true });

  flashRoot.appendChild(el("div", { class: "flash-card" }, [
    el("div", { class: "flash-copy" }, [
      el("b", { text: "Flash from your browser" }),
      el("span", { text: "Pick your board, connect it over USB, then flash. Your config from above rides along as a small partition." }),
    ]),
    el("div", { class: "flash-controls" }, [
      el("label", { class: "flash-board-field" }, [el("span", { text: "Board" }), boardSelect]),
      el("div", { class: "flash-actions" }, [customBtn, stockBtn]),
    ]),
    bar,
    logBox,
  ]));

  flashSync = () => {
    flasher.revokeUrls();
    customBtn.setAttribute("manifest", customManifest());
    stockBtn.setAttribute("manifest", stockManifest());
  };
});

reconcile();
render();
