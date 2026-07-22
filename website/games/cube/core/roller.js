const QUARTER = Math.PI / 2;
const RUSH = 3;

export class Roller {
  constructor() {
    this.reset(0, 0);
  }

  reset(col, row) {
    this.col = col;
    this.row = row;
    this.roll = null;
  }

  get rolling() {
    return this.roll !== null;
  }

  start(dx, dy, duration) {
    this.roll = { dx, dy, theta: 0, duration, rush: false, revert: false };
  }

  progress() {
    return this.roll ? Math.min(1, this.roll.theta / QUARTER) : 0;
  }

  center() {
    const t = this.progress();
    return {
      x: this.col + 0.5 + (this.roll?.dx ?? 0) * t,
      y: this.row + 0.5 + (this.roll?.dy ?? 0) * t,
    };
  }

  advance(dt) {
    if (!this.roll) return null;

    const rate = (QUARTER / this.roll.duration) * (this.roll.rush ? RUSH : 1);
    if (this.roll.revert) {
      this.roll.theta -= rate * dt;
      if (this.roll.theta <= 0) this.roll = null;
      return null;
    }

    this.roll.theta += rate * dt;
    if (this.roll.theta < QUARTER) return null;

    const landed = this.roll;
    this.col += landed.dx;
    this.row += landed.dy;
    this.roll = null;
    return landed;
  }
}

export const FACES = [
  [0, 1, 2, 3], [4, 5, 6, 7], [0, 1, 5, 4],
  [3, 2, 6, 7], [0, 3, 7, 4], [1, 2, 6, 5],
];

export function boxVerts(x0, z0, size, height) {
  return [
    [x0, 0, z0], [x0 + size, 0, z0], [x0 + size, 0, z0 + size], [x0, 0, z0 + size],
    [x0, height, z0], [x0 + size, height, z0], [x0 + size, height, z0 + size],
    [x0, height, z0 + size],
  ];
}

export function rollingVerts(x0, z0, size, roll) {
  let verts = boxVerts(x0, z0, size, size);
  if (!roll) return verts;

  const { dx, dy, theta } = roll;
  const cos = Math.cos(theta);
  const sin = Math.sin(theta);
  if (dx) {
    const px = dx > 0 ? x0 + size : x0;
    verts = verts.map(([x, y, z]) => [
      px + (x - px) * cos + dx * y * sin,
      -dx * (x - px) * sin + y * cos,
      z,
    ]);
  } else {
    const pz = dy > 0 ? z0 + size : z0;
    verts = verts.map(([x, y, z]) => [
      x,
      -dy * (z - pz) * sin + y * cos,
      pz + (z - pz) * cos + dy * y * sin,
    ]);
  }
  return verts;
}

const TILT = 0.35;

export const tiltSign = (v) => (Math.abs(v) > TILT ? Math.sign(v) : 0);
