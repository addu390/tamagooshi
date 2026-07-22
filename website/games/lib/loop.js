const MAX_DT = 0.05;

export function fitCanvas(canvas, ctx) {
  const resize = () => {
    const dpr = devicePixelRatio || 1;
    canvas.width = canvas.clientWidth * dpr;
    canvas.height = canvas.clientHeight * dpr;
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  };
  addEventListener("resize", resize);
  resize();
}

export function startLoop(step) {
  let last = performance.now();
  const tick = (now) => {
    const dt = Math.min(MAX_DT, (now - last) / 1000);
    last = now;
    step(dt, now / 1000);
    requestAnimationFrame(tick);
  };
  requestAnimationFrame(tick);
}
