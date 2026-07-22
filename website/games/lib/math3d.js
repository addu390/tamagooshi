export function normOf(a) {
  const l = Math.hypot(...a) || 1;
  return [a[0] / l, a[1] / l, a[2] / l];
}

export const sub = (a, b) => [a[0] - b[0], a[1] - b[1], a[2] - b[2]];
export const dot = (a, b) => a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
export const lerp3 = (a, b, t) => [a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t,
                                   a[2] + (b[2] - a[2]) * t];
export const cross = (a, b) => [
  a[1] * b[2] - a[2] * b[1],
  a[2] * b[0] - a[0] * b[2],
  a[0] * b[1] - a[1] * b[0],
];

export function hash2(a, b) {
  const n = Math.sin(a * 127.1 + b * 311.7) * 43758.5453;
  return n - Math.floor(n);
}

export class Camera {
  constructor(w, h, target, back, up, fov) {
    this.pos = [target[0], up, target[2] + back];
    this.fwd = normOf(sub(target, this.pos));
    this.right = normOf(cross(this.fwd, [0, 1, 0]));
    this.up = cross(this.right, this.fwd);
    this.cx = w / 2;
    this.cy = h * 0.54;
    this.f = Math.min(w, h) * fov / 2;
  }

  project(p) {
    const d = sub(p, this.pos);
    const z = dot(d, this.fwd);
    if (z < 0.2) return null;
    return {
      x: this.cx + this.f * dot(d, this.right) / z,
      y: this.cy - this.f * dot(d, this.up) / z,
      z,
    };
  }

  depth(p) {
    return dot(sub(p, this.pos), this.fwd);
  }
}

const SNAP_DISTANCE = 5;

export class Follower {
  constructor(rate = 9) {
    this.rate = rate;
    this.value = null;
    this.time = null;
  }

  update(target, time) {
    const jump = this.value &&
      Math.hypot(target.x - this.value.x, target.y - this.value.y) > SNAP_DISTANCE;
    if (!this.value || jump) {
      this.value = { ...target };
      this.time = time;
      return this.value;
    }

    const dt = Math.max(0, time - this.time);
    this.time = time;
    const k = 1 - Math.exp(-this.rate * dt);
    this.value.x += (target.x - this.value.x) * k;
    this.value.y += (target.y - this.value.y) * k;
    return this.value;
  }
}
