(function () {
  const rad = (d) => (d * Math.PI) / 180;

  function project(p, A, E) {
    const ca = Math.cos(A), sa = Math.sin(A), ce = Math.cos(E), se = Math.sin(E);
    const X1 = p[0] * ca - p[1] * sa;
    const Y1 = p[0] * sa + p[1] * ca;
    const Z1 = p[2];
    return { x: X1, y: -(Z1 * ce - Y1 * se), depth: Y1 * ce + Z1 * se };
  }

  function mountRotatable(el, cfg) {
    const NS = "http://www.w3.org/2000/svg";
    const svg = document.createElementNS(NS, "svg");
    svg.setAttribute("role", "img");
    if (cfg.ariaLabel) svg.setAttribute("aria-label", cfg.ariaLabel);
    el.appendChild(svg);
    el.classList.add("free");

    const [ymin, ymax] = cfg.yawRange;
    const [pmin, pmax] = cfg.pitchRange;
    let yaw = cfg.yawDeg, pitch = cfg.pitchDeg, drag = null;

    function draw() {
      const { inner, viewBox } = cfg.build(rad(yaw), rad(pitch));
      svg.setAttribute("viewBox", `${viewBox.x.toFixed(1)} ${viewBox.y.toFixed(1)} ${viewBox.w.toFixed(1)} ${viewBox.h.toFixed(1)}`);
      svg.innerHTML = inner;
    }
    draw();

    svg.addEventListener("pointerdown", (e) => {
      drag = { x: e.clientX, y: e.clientY, yaw, pitch };
      svg.setPointerCapture(e.pointerId);
      el.classList.add("grabbing");
    });
    svg.addEventListener("pointermove", (e) => {
      if (!drag) return;
      yaw = Math.max(ymin, Math.min(ymax, drag.yaw + (e.clientX - drag.x) * 0.4));
      pitch = Math.max(pmin, Math.min(pmax, drag.pitch - (e.clientY - drag.y) * 0.35));
      draw();
    });
    const end = (e) => { if (drag) { el.classList.remove("grabbing"); drag = null; try { svg.releasePointerCapture(e.pointerId); } catch (_) {} } };
    svg.addEventListener("pointerup", end);
    svg.addEventListener("pointercancel", end);
  }

  window.Iso = { rad, project, mountRotatable };
})();
