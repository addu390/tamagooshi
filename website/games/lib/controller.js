const DEADZONE = 0.08;
const KEY_RAMP = 8;

const AXIS_KEYS = {
  ArrowLeft: ["x", -1], ArrowRight: ["x", 1],
  ArrowUp: ["y", -1], ArrowDown: ["y", 1],
  a: ["x", -1], d: ["x", 1], w: ["y", -1], s: ["y", 1],
};

const shape = (v) => (Math.abs(v) < DEADZONE ? 0 : v);

export class Controller {
  constructor() {
    this.frame = {
      x: 0, y: 0,
      a: false, b: false,
      aHit: false, bHit: false,
      connected: false,
    };
    this._keys = new Set();
    this._keyAxis = { x: 0, y: 0 };

    addEventListener("keydown", (e) => this._keys.add(e.key));
    addEventListener("keyup", (e) => this._keys.delete(e.key));
    addEventListener("blur", () => this._keys.clear());
  }

  poll(dt) {
    const wasA = this.frame.a;
    const wasB = this.frame.b;

    const pad = navigator.getGamepads().find((g) => g && g.connected);
    if (pad) {
      this.frame.x = shape(pad.axes[0] ?? 0);
      this.frame.y = shape(pad.axes[1] ?? 0);
      this.frame.a = Boolean(pad.buttons[0]?.pressed);
      this.frame.b = Boolean(pad.buttons[1]?.pressed);
      this.frame.connected = true;
    } else {
      const target = { x: 0, y: 0 };
      for (const key of this._keys) {
        const bind = AXIS_KEYS[key];
        if (bind) target[bind[0]] = bind[1];
      }
      const blend = Math.min(1, KEY_RAMP * dt);
      this._keyAxis.x += (target.x - this._keyAxis.x) * blend;
      this._keyAxis.y += (target.y - this._keyAxis.y) * blend;

      this.frame.x = shape(this._keyAxis.x);
      this.frame.y = shape(this._keyAxis.y);
      this.frame.a = this._keys.has(" ");
      this.frame.b = this._keys.has("Enter");
      this.frame.connected = false;
    }

    this.frame.aHit = this.frame.a && !wasA;
    this.frame.bHit = this.frame.b && !wasB;
    return this.frame;
  }
}
