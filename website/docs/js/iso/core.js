export const rad = (d) => (d * Math.PI) / 180;

export function project(p, A, E) {
  const ca = Math.cos(A), sa = Math.sin(A), ce = Math.cos(E), se = Math.sin(E);
  const X1 = p[0] * ca - p[1] * sa;
  const Y1 = p[0] * sa + p[1] * ca;
  const Z1 = p[2];
  return { x: X1, y: -(Z1 * ce - Y1 * se), depth: Y1 * ce + Z1 * se };
}
