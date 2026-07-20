import { theme as D } from "./device.js";

export function initDemo() {
  const mount = document.querySelector(".demo-device");
  if (!mount) return;

  const PRESET = window.TAMA_PRESET || {};
  const SIL = D.SIL, DET = D.DET, DC = D.colors, GRID = DC.grid;
  const lit = (t) => `rgb(${DC.shade0.map((v) => Math.round(v + (255 - v) * t)).join(",")})`;
  const SHELL = Object.assign({
    front: lit(0.85), top: lit(0.55), right: lit(0.3), mascot: lit(0.4),
    body: DC.shell, bezel: DC.bezel,
  }, PRESET.demoShell || {});
  const FW = 140, FH = 280;
  const dx = 34, dy = -22;
  const R = 11;

  const mm = FW / 24;
  const SCR = { x: (FW - 16.5 * mm) / 2, y: 0.09 * FH, w: 16.5 * mm, h: 16.5 * mm * 240 / 135 };
  const BTN = { x: (FW - 12.8 * mm) / 2, y: 0.84 * FH - (2.6 * mm) / 2, w: 12.8 * mm, h: 2.6 * mm };

  const topFace = [[0, 0], [FW, 0], [FW + dx, dy], [dx, dy]];
  const rightFace = [[FW, 0], [FW, FH], [FW + dx, FH + dy], [FW + dx, dy]];
  const DIP = 2.6, obl = Math.hypot(dx, dy);
  const dipX = -DIP * dx / obl, dipY = -DIP * dy / obl;
  const dipTop = 0.80 * FH + dy;
  const outline = [
    [0, 0], [0, FH], [FW, FH],
    [FW + dx + dipX, FH + dy + dipY], [FW + dx + dipX, dipTop + dipY], [FW + dx, dipTop - 3],
    [FW + dx, dy], [dx, dy],
  ];
  const poly = (p) => p.map((q) => `${q[0].toFixed(1)},${q[1].toFixed(1)}`).join(" ");
  const P2 = (p) => ({ x: p[0], y: p[1] });
  const outlineD = D.roundedPoly(outline.map(P2), R);

  const S = (t, g) => [FW + g * dx, t + g * dy];
  const quadPath = (pts, r, fill, stroke, sw, dash) =>
    `<path d="${D.roundedPoly(pts.map(P2), r)}" style="fill:${fill}"${stroke ? ` stroke="${stroke}" stroke-width="${sw}"${dash ? ` stroke-dasharray="4 3"` : ""} vector-effect="non-scaling-stroke"` : ""}/>`;
  const sideSq = (tc, halfT, halfG, r, fill, stroke, sw) =>
    quadPath([S(tc - halfT, 0.5 - halfG), S(tc + halfT, 0.5 - halfG), S(tc + halfT, 0.5 + halfG), S(tc - halfT, 0.5 + halfG)], r, fill, stroke, sw);

  const encA = 0.14 * FH, encB = 0.86 * FH, ctrT = 0.50 * FH;
  const encHT = 13, encG = 0.21;
  const pocketT = 19.5, pocketG = 0.26, capT = 14.5, capG = 0.19;
  const seamLine = (t1, t2) => t2 - t1 < 6 ? "" : `<line x1="${S(t1, 0.5)[0].toFixed(1)}" y1="${S(t1, 0.5)[1].toFixed(1)}" x2="${S(t2, 0.5)[0].toFixed(1)}" y2="${S(t2, 0.5)[1].toFixed(1)}" stroke="${DET}" stroke-width="0.9" stroke-linecap="round"/>`;

  const seam =
    seamLine(6, encA - encHT - 3) + seamLine(encA + encHT + 3, ctrT - pocketT - 3)
    + seamLine(ctrT + pocketT + 3, encB - encHT - 3) + seamLine(encB + encHT + 3, FH - 6)
    + sideSq(encA, encHT, encG, 5, "none", DET, 0.9)
    + sideSq(encB, encHT, encG, 5, "none", DET, 0.9)
    + `<g id="demoSideBtn" style="cursor:pointer">`
    + sideSq(ctrT, pocketT, pocketG, 5, DC.btnPocket, SIL, 1)
    + sideSq(ctrT, pocketT - 2.3, pocketG - 0.036, 4, DC.btnPocketDeep)
    + `<g id="demoSideCap">` + sideSq(ctrT, capT, capG, 3.5, DC.btnCap, SIL, 1.1) + `</g>`
    + `</g>`
    + sideSq(ctrT - pocketT - 36, 10.5, 0.05, 2, DC.hatSlot)
    + sideSq(ctrT + pocketT + 36, 10.5, 0.05, 2, DC.hatSlot);

  const T = (x, y) => [x * FW / 104 + (1 - y / 44) * dx, (1 - y / 44) * dy];
  const topQuad = (x0, x1, y0, y1, r, fill, stroke, sw) =>
    quadPath([T(x0, y0), T(x1, y0), T(x1, y1), T(x0, y1)], r, fill, stroke, sw);

  const hatFeats = (() => {
    const hx0 = 10, hw = 84, hy0 = 7, hh = 22, cols = 9, rows = 2, gpx = 3.4;
    const cw = (hw - gpx * (cols + 1)) / cols, ch = (hh - gpx * (rows + 1)) / rows;
    const colX = (c) => hx0 + gpx + c * (cw + gpx);

    let out = topQuad(hx0, hx0 + hw, hy0, hy0 + hh, 4, DC.hat, SIL, 1);
    for (let r = 0; r < rows; r++) for (let c = 0; c < cols; c++) {
      const x = colX(c), y = hy0 + gpx + r * (ch + gpx);
      out += topQuad(x, x + cw, y, y + ch, 1.5, DC.hatHole);
    }

    const vy = 0.78 * 44, slotY = hy0 + hh, slotTop = vy + ch * 0.4;
    const sw = 2 * cw + gpx;
    out += topQuad(colX(0), colX(0) + sw, slotY, slotTop, 1.5, DC.hatSlot);

    const gx0 = colX(7);
    const tpoly = (pts, op) => `<polygon points="${pts.map((p) => { const q = T(p[0], p[1]); return `${q[0].toFixed(1)},${q[1].toFixed(1)}`; }).join(" ")}" fill="#ffffff" opacity="${op}"/>`;
    out += topQuad(gx0, gx0 + sw, slotY, slotTop, 1.5, DC.hatGlass, DC.hatGlassLine, 0.6);
    out += tpoly([[gx0 + 0.16 * sw, slotY], [gx0 + 0.38 * sw, slotY], [gx0 + 0.24 * sw, slotTop], [gx0 + 0.02 * sw, slotTop]], 0.16);
    out += tpoly([[gx0 + 0.52 * sw, slotY], [gx0 + 0.60 * sw, slotY], [gx0 + 0.46 * sw, slotTop], [gx0 + 0.38 * sw, slotTop]], 0.07);

    return out + [-1, 0, 1].map((k) => topQuad(52 + k * 7 - 2.2, 52 + k * 7 + 2.2, vy - 2.2, vy + 2.2, 2.2, DC.vent)).join("");
  })();

  const liftX = -dx * (1.2 / 13.5), liftY = -dy * (1.2 / 13.5);
  const btnPill = (ox, oy, fill, stroke) =>
    `<rect x="${(BTN.x + ox).toFixed(2)}" y="${(BTN.y + oy).toFixed(2)}" width="${BTN.w.toFixed(2)}" height="${BTN.h.toFixed(2)}" rx="${(BTN.h / 2).toFixed(2)}" style="fill:${fill}"${stroke ? ` stroke="${SIL}" stroke-width="1.1" vector-effect="non-scaling-stroke"` : ""}/>`;
  const btnParts = (k) => {
    const lx = liftX * k, ly = liftY * k;
    let b = btnPill(0, 0, "var(--accent)", true);
    for (let s = 1; s <= 10; s++) b += btnPill(lx * s / 10, ly * s / 10, "var(--accent)", false);
    return b + btnPill(lx, ly, "color-mix(in srgb, var(--accent) 74%, #fff)", true);
  };
  const btnBody = `<g id="demoBtnA" style="cursor:pointer">${btnParts(1)}</g>`;

  const MASCOT = D.mascotArt;
  const n = MASCOT[0].length, m = MASCOT.length;
  const ps = Math.min(SCR.w / n, SCR.h / m) * 0.58;
  const gx = SCR.x + (SCR.w - n * ps) / 2, gy = SCR.y + (SCR.h - m * ps) / 2;
  let mascot = "";
  for (let rr = 0; rr < m; rr++) for (let cc = 0; cc < n; cc++) if (MASCOT[rr][cc] === "#")
    mascot += `<rect x="${(gx + cc * ps).toFixed(2)}" y="${(gy + rr * ps).toFixed(2)}" width="${(ps + 0.4).toFixed(2)}" height="${(ps + 0.4).toFixed(2)}" fill="${SHELL.mascot}"/>`;

  let minx = Infinity, miny = Infinity, maxx = -Infinity, maxy = -Infinity;
  const acc = (p) => { if (p[0] < minx) minx = p[0]; if (p[0] > maxx) maxx = p[0]; if (p[1] < miny) miny = p[1]; if (p[1] > maxy) maxy = p[1]; };
  outline.forEach(acc);
  const vbx = minx - 58, vby = miny - 108, vbw = maxx - minx + 250, vbh = maxy - miny + 216;

  const G = 46, insf = 0.14, cm = 9;
  const baseX0 = 0, baseX1 = FW + dx, baseY0 = FH + dy, baseY1 = FH;
  const fpt = (ci, cj) => [ci * G + cj * dx, FH + cj * dy];
  const inVB = (p) => p[0] >= vbx && p[0] <= vbx + vbw && p[1] >= vby && p[1] <= vby + vbh;
  let floor = "";
  for (let i = -30; i < 24; i++) for (let j = -8; j < 28; j++) {
    const box = [fpt(i + insf, j + insf), fpt(i + 1 - insf, j + insf), fpt(i + 1 - insf, j + 1 - insf), fpt(i + insf, j + 1 - insf)];
    if (!box.some(inVB)) continue;
    let bx0 = 1e9, by0 = 1e9, bx1 = -1e9, by1 = -1e9;
    for (const p of box) { if (p[0] < bx0) bx0 = p[0]; if (p[0] > bx1) bx1 = p[0]; if (p[1] < by0) by0 = p[1]; if (p[1] > by1) by1 = p[1]; }
    if (bx1 > baseX0 - cm && bx0 < baseX1 + cm && by1 > baseY0 - cm && by0 < baseY1 + cm) continue;
    floor += `<polygon points="${poly(box)}" fill="none" stroke="${GRID}" stroke-width="1"/>`;
  }

  const edge = (x1, y1, x2, y2) => {
    const ex = x2 - x1, ey = y2 - y1, l = Math.hypot(ex, ey) || 1, t = R / l;
    return `<line x1="${(x1 + ex * t).toFixed(1)}" y1="${(y1 + ey * t).toFixed(1)}" x2="${(x2 - ex * t).toFixed(1)}" y2="${(y2 - ey * t).toFixed(1)}" stroke="${DET}" stroke-width="0.9" stroke-linecap="round"/>`;
  };
  const svgInner = `
    ${floor}
    <clipPath id="demoScr"><rect x="${SCR.x}" y="${SCR.y}" width="${SCR.w}" height="${SCR.h}" rx="6"/></clipPath>
    <clipPath id="demoBody"><path d="${outlineD}"/></clipPath>
    <g class="dev-body">
    <path d="${outlineD}" fill="${SHELL.body}"/>
    <g clip-path="url(#demoBody)">
    ${quadPath(topFace, R, SHELL.top)}
    ${hatFeats}
    ${quadPath(rightFace, R, SHELL.right)}
    ${seam}
    <rect width="${FW}" height="${FH}" rx="${R}" fill="${SHELL.front}"/>
    </g>
    ${edge(0, 0, FW, 0)}
    ${edge(FW, 0, FW, FH)}
    ${edge(FW, 0, FW + dx, dy)}
    <rect x="${SCR.x - 5}" y="${SCR.y - 5}" width="${SCR.w + 10}" height="${SCR.h + 10}" rx="11" fill="${DET}"/>
    <rect x="${SCR.x - 2}" y="${SCR.y - 2}" width="${SCR.w + 4}" height="${SCR.h + 4}" rx="9" fill="${SHELL.bezel}"/>
    <rect x="${SCR.x}" y="${SCR.y}" width="${SCR.w}" height="${SCR.h}" rx="6" fill="#0e0f12"/>
    <g clip-path="url(#demoScr)">${mascot}</g>
    <foreignObject id="demoFO" x="${SCR.x}" y="${SCR.y}" width="${SCR.w}" height="${SCR.h}" clip-path="url(#demoScr)"></foreignObject>
    <rect x="${SCR.x}" y="${SCR.y}" width="${SCR.w}" height="${SCR.h}" rx="6" fill="none" stroke="${SIL}" stroke-width="1.2" vector-effect="non-scaling-stroke"/>
    ${btnBody}
    <path d="${outlineD}" fill="none" stroke="${SIL}" stroke-width="1.6" stroke-linejoin="round"/>
    </g>`;

  const NS = "http://www.w3.org/2000/svg";
  const svg = document.createElementNS(NS, "svg");
  svg.setAttribute("viewBox", `${vbx.toFixed(1)} ${vby.toFixed(1)} ${vbw.toFixed(1)} ${vbh.toFixed(1)}`);
  svg.setAttribute("role", "img");
  svg.setAttribute("aria-label", "ESP32 handheld with the demo screen facing forward");
  svg.innerHTML = svgInner;
  mount.appendChild(svg);

  const landTx = (vbx + vbw / 2) - (minx + maxx) / 2;
  mount.style.setProperty("--land-tx", `${landTx.toFixed(1)}px`);

  const pressPlay = (apply) => {
    const t0 = performance.now(), dur = 280;
    const step = (now) => {
      const p = Math.min(1, (now - t0) / dur);
      apply(Math.sin(p * Math.PI));
      if (p < 1) requestAnimationFrame(step); else apply(0);
    };
    requestAnimationFrame(step);
  };
  const btnA = svg.querySelector("#demoBtnA");
  btnA.addEventListener("pointerdown", () => {
    pressPlay((v) => { btnA.innerHTML = btnParts(1 - v); });
    window.dispatchEvent(new CustomEvent("tama:accent-next"));
  });
  const sideCap = svg.querySelector("#demoSideCap");
  svg.querySelector("#demoSideBtn").addEventListener("pointerdown", () =>
    pressPlay((v) => sideCap.setAttribute("transform", `translate(${(-2.2 * v).toFixed(2)},0)`)));

  const screen = document.createElement("div");
  screen.className = "demo-screen";
  svg.querySelector("#demoFO").appendChild(screen);

  const DEMO = PRESET.demo || {};
  const SCENARIOS = DEMO.scenarios || [
    { id: "mood", title: "Live mood", desc: "The mascot idles and fidgets while a live metric ticks, easing its mood as the numbers move.", metrics: true,
      markers: [{ a: 0.35, kind: "warn" }, { a: 0.42, kind: "crit" }, { a: 0.585, kind: "warn" }] },
    { id: "metrics", title: "Powered by your metrics", desc: "Your live numbers flow in from Stripe, PostHog and Datadog. The device cycles through them as a launch spike lands.",
      markers: [{ a: 0.29, kind: "hi" }] },
    { id: "agents", title: "Co-work with Claude & Cursor", desc: "The mascot works alongside an agent session and celebrates the landed task. Then you press A, speak, and Claude answers on the device.",
      markers: [{ a: 0.28, kind: "hi" }, { a: 0.46, kind: "hi" }, { a: 0.56, kind: "warn" }, { a: 0.70, kind: "hi" }] },
    { id: "alerts", title: "Approvals & alerts", desc: "An approval slides up to approve or deny, then an alert fires as a threshold is crossed.",
      markers: [{ a: 0.05, kind: "warn" }, { a: 0.25, kind: "warn" }, { a: 0.50, kind: "crit" }] },
    { id: "brand", title: "Make it yours", desc: "A quick mini-game, then the shell gets rebranded live: the Clawd mascot pack and the terra theme.", gameWin: { start: 50 / 290, end: 99 / 290 },
      markers: [{ a: 0.53, kind: "hi" }, { a: 0.62, kind: "hi" }] },
  ];

  const video = document.createElement("video");
  video.muted = true;
  video.loop = true;
  video.playsInline = true;
  video.setAttribute("playsinline", "");
  video.preload = "none";
  video.style.display = "none";
  video.addEventListener("loadeddata", () => {
    video.style.display = "block";
    video.playbackRate = 0.9;
    if (visible && !userPaused) video.play().catch(() => {});
  });
  video.addEventListener("error", () => { video.style.display = "none"; });
  screen.appendChild(video);

  const pct = (v) => v.toFixed(1) + "%";
  const intf = (v) => String(Math.round(v));
  const UPTIME_KF = [[0, 99.9], [0.25, 99.9], [0.35, 99.3], [0.4, 96.0], [0.45, 93.0], [0.55, 93.0], [0.6, 95.8], [0.7, 98.4], [0.8, 99.7], [1, 99.9]];
  const METRICS = [
    { key: "uptime", label: "UPTIME", fmt: pct, kf: UPTIME_KF,
      health: (v) => (v >= 99 ? "ok" : v >= 95 ? "warn" : "crit") },
    { key: "signups", label: "SIGNUPS", fmt: intf,
      kf: [[0, 322], [1, 360]], health: () => "ok" },
  ];

  function interp(kf, f) {
    let p = kf[0];
    for (let i = 1; i < kf.length; i++) {
      const c = kf[i];
      if (f <= c[0]) {
        const span = c[0] - p[0];
        const t = span <= 0 ? 0 : (f - p[0]) / span;
        return p[1] + (c[1] - p[1]) * t;
      }
      p = c;
    }
    return kf[kf.length - 1][1];
  }

  const stage = document.querySelector(".demo-stage");
  const metricsEl = document.createElement("div");
  metricsEl.className = "demo-metrics";
  metricsEl.setAttribute("aria-hidden", "true");
  const tiles = METRICS.map((m) => {
    const el = document.createElement("div");
    el.className = "mtile";
    el.innerHTML = `<span class="mtile-v"></span><span class="mtile-a"></span><span class="mtile-k">${m.label}</span>`;
    metricsEl.appendChild(el);
    return { def: m, el, arrow: el.querySelector(".mtile-a"), val: el.querySelector(".mtile-v") };
  });
  const cap = document.createElement("div");
  cap.className = "demo-cap";
  metricsEl.appendChild(cap);
  if (stage) stage.appendChild(metricsEl);

  const LEGENDS = DEMO.legends || {
    metrics: { h: "POWERED BY", rows: [["MRR", "Stripe"], ["SIGNUPS", "PostHog"], ["ACTIVE", "PostHog"], ["UPTIME", "Datadog"]] },
    agents: { h: "WORKS WITH", rows: [["Claude", "live"], ["Cursor", "live"], ["Voice", "hold A"]] },
    alerts: { h: "IN THE LOOP", rows: [["Approvals", "Cursor"], ["Alerts", "Datadog"]] },
    brand: { h: "MAKE IT YOURS", rows: [["Logo", "yours"], ["Theme", "5"], ["Mascot", "swap"], ["Games", "3"]] },
  };
  const legendEl = document.createElement("div");
  legendEl.className = "demo-sources";
  legendEl.setAttribute("aria-hidden", "true");
  if (stage) stage.appendChild(legendEl);

  const ICON_PLAY = '<svg viewBox="0 0 12 12" aria-hidden="true"><path d="M2.5 1.3 10.4 6 2.5 10.7Z" fill="currentColor"/></svg>';
  const ICON_PAUSE = '<svg viewBox="0 0 12 12" aria-hidden="true"><rect x="2.6" y="1.5" width="2.4" height="9" fill="currentColor"/><rect x="7" y="1.5" width="2.4" height="9" fill="currentColor"/></svg>';
  const scrub = document.createElement("div");
  scrub.className = "demo-scrub";
  scrub.innerHTML =
    '<button class="scrub-btn" type="button"></button>' +
    '<div class="scrub-track"><div class="scrub-fill"></div><div class="scrub-marks"></div><div class="scrub-knob"></div></div>' +
    '<span class="scrub-time">0:00</span>';
  if (stage) stage.appendChild(scrub);
  const playBtn = scrub.querySelector(".scrub-btn");
  const track = scrub.querySelector(".scrub-track");
  const fill = scrub.querySelector(".scrub-fill");
  const marks = scrub.querySelector(".scrub-marks");
  const knob = scrub.querySelector(".scrub-knob");
  const timeEl = scrub.querySelector(".scrub-time");

  function renderMarks(list) {
    marks.innerHTML = (list || [])
      .map((k) => `<span class="scrub-mark ${k.kind}" data-at="${k.a}" style="left:${(k.a * 100).toFixed(2)}%"></span>`)
      .join("");
  }

  function paintMarks(p) {
    marks.querySelectorAll(".scrub-mark").forEach((m) =>
      m.classList.toggle("passed", p >= parseFloat(m.dataset.at)));
  }

  let scrubbing = false;
  let resumeAfter = false;
  let visible = false;
  let userPaused = false;
  let pendingLoad = false;
  let rafId = null;

  function fmtTime(t) {
    if (!isFinite(t) || t < 0) t = 0;
    const s = Math.floor(t);
    return Math.floor(s / 60) + ":" + String(s % 60).padStart(2, "0");
  }

  function setPlayIcon() {
    const paused = video.paused;
    playBtn.innerHTML = paused ? ICON_PLAY : ICON_PAUSE;
    playBtn.setAttribute("aria-label", paused ? "Play" : "Pause");
  }

  function updateScrub() {
    const dur = video.duration;
    if (!dur) { fill.style.width = "0%"; knob.style.left = "0%"; paintMarks(0); timeEl.textContent = "0:00"; return; }

    const p = Math.min(1, video.currentTime / dur);
    fill.style.width = (p * 100).toFixed(2) + "%";
    knob.style.left = (p * 100).toFixed(2) + "%";
    paintMarks(p);
    timeEl.textContent = fmtTime(video.currentTime) + " / " + fmtTime(dur);
  }

  function seekToClientX(clientX) {
    const rect = track.getBoundingClientRect();
    if (!video.duration || rect.width <= 0) return;
    const p = Math.min(1, Math.max(0, (clientX - rect.left) / rect.width));
    video.currentTime = p * video.duration;
    updateScrub();
  }

  playBtn.addEventListener("click", () => {
    if (video.paused) {
      userPaused = false;
      if (pendingLoad) { pendingLoad = false; video.load(); } else video.play().catch(() => {});
    } else {
      userPaused = true;
      video.pause();
    }
  });

  track.addEventListener("pointerdown", (e) => {
    e.preventDefault();
    scrubbing = true;
    resumeAfter = !video.paused;
    video.pause();
    try { track.setPointerCapture(e.pointerId); } catch (_) {}
    seekToClientX(e.clientX);
  });
  track.addEventListener("pointermove", (e) => { if (scrubbing) seekToClientX(e.clientX); });

  const endScrub = (e) => {
    if (!scrubbing) return;
    scrubbing = false;
    try { track.releasePointerCapture(e.pointerId); } catch (_) {}
    if (resumeAfter) video.play().catch(() => {});
  };
  track.addEventListener("pointerup", endScrub);
  track.addEventListener("pointercancel", endScrub);

  video.addEventListener("play", setPlayIcon);
  video.addEventListener("pause", setPlayIcon);
  setPlayIcon();

  function renderLegend(id) {
    const L = LEGENDS[id];
    legendEl.classList.toggle("on", !!L);
    if (!L) return;
    legendEl.innerHTML =
      `<div class="src-h">${L.h}</div>` +
      L.rows.map((r) => `<div class="src-row"><span class="src-m">${r[0]}</span><span class="src-dot"></span><span class="src-s">${r[1]}</span></div>`).join("");
  }

  let current = null;
  let metricsOn = false;

  function tick() {
    if (current && current.gameWin && video.duration) {
      const f = (video.currentTime / video.duration) % 1;
      const land = f >= current.gameWin.start && f <= current.gameWin.end;
      mount.classList.toggle("landscape", land);
      legendEl.classList.toggle("on", !land && !!LEGENDS[current.id]);
    }

    if (metricsOn && video.duration) {
      const dur = video.duration;
      const f = (video.currentTime / dur) % 1;
      const back = (f - 0.02 + 1) % 1;

      tiles.forEach((t) => {
        const now = interp(t.def.kf, f);
        const prev = interp(t.def.kf, back);
        const h = t.def.health(now);
        const cls = "m-" + h;
        const d = now - prev;
        const arrow = d > 1e-6 ? "\u25B2" : d < -1e-6 ? "\u25BC" : "\u2022";
        t.val.textContent = t.def.fmt(now);
        t.arrow.textContent = arrow;
        t.arrow.className = "mtile-a " + cls;
        t.el.className = "mtile" + (h === "ok" ? "" : " " + h);
      });

      const up = interp(UPTIME_KF, f);
      cap.textContent = up < 95 ? "SLA breached, uptime critical" : up < 99 ? "SLA at risk, uptime slipping" : "";
      cap.className = "demo-cap" + (up < 99 ? " on " + (up < 95 ? "crit" : "warn") : "");
    }

    updateScrub();
    rafId = requestAnimationFrame(tick);
  }

  function startLoop() { if (rafId === null) rafId = requestAnimationFrame(tick); }
  function stopLoop() { if (rafId !== null) { cancelAnimationFrame(rafId); rafId = null; } }

  const list = document.querySelector(".demo-walk");
  if (list && DEMO.note) {
    const li = document.createElement("li");
    li.className = "walk-item walk-note";
    li.innerHTML = `<span class="walk-n">0</span><span class="walk-t"><b>${DEMO.note.title}</b><em>${DEMO.note.desc}</em></span>`;
    list.appendChild(li);
  }
  const items = SCENARIOS.map((s, i) => {
    const li = document.createElement("li");
    li.className = "walk-item";
    li.tabIndex = 0;
    li.setAttribute("role", "button");
    li.innerHTML = `<span class="walk-n">${i + 1}</span><span class="walk-t"><b>${s.title}</b><em>${s.desc}</em></span>`;
    li.addEventListener("click", () => select(i));
    li.addEventListener("keydown", (e) => { if (e.key === "Enter" || e.key === " ") { e.preventDefault(); select(i); } });
    list.appendChild(li);
    return li;
  });

  function select(i) {
    items.forEach((li, k) => li.classList.toggle("on", k === i));
    const s = SCENARIOS[i];
    current = s;
    metricsOn = !!s.metrics;

    metricsEl.classList.toggle("on", metricsOn);
    mount.classList.remove("landscape");
    renderLegend(s.id);
    renderMarks(s.markers);

    userPaused = false;
    video.innerHTML = `<source src="assets/videos/${s.id}.webm" type="video/webm"><source src="assets/videos/${s.id}.mp4" type="video/mp4">`;
    video.style.display = "none";
    if (visible) { pendingLoad = false; video.load(); } else pendingLoad = true;
  }

  if (list) select(0);

  const io = new IntersectionObserver((entries) => {
    visible = entries[0].isIntersecting;
    if (visible) {
      startLoop();
      if (pendingLoad) { pendingLoad = false; video.load(); }
      else if (video.paused && !userPaused) video.play().catch(() => {});
    } else {
      video.pause();
      stopLoop();
    }
  }, { threshold: 0.2 });
  io.observe(mount);
}
