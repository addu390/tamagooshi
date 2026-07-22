import { Roller, tiltSign } from "../../roller.js";

const ROLL_DURATION = 0.3;

export class World {
  constructor() {
    this.roller = new Roller();
    this.rolls = 0;
  }

  get col() {
    return this.roller.col;
  }

  get row() {
    return this.roller.row;
  }

  get roll() {
    return this.roller.roll;
  }

  center() {
    return this.roller.center();
  }

  step(frame, dt) {
    if (this.roller.advance(dt)) this.rolls += 1;
    if (this.roller.rolling) return;

    const dx = tiltSign(frame.x);
    const dy = tiltSign(frame.y);
    if (!dx && !dy) return;

    if (Math.abs(frame.x) >= Math.abs(frame.y)) this.roller.start(dx, 0, ROLL_DURATION);
    else this.roller.start(0, dy, ROLL_DURATION);
  }
}
