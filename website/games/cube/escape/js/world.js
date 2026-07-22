import { Roller, tiltSign } from "../../core/roller.js";
import { Track } from "./track.js";

export const LANES = 7;

const FORWARD_DURATION = { slow: 0.66, normal: 0.42, boost: 0.24 };
const LATERAL_DURATION = 0.16;
const RELEASE = 0.22;
const FLICK_TTL = 0.45;
const REVERT_LIMIT = 0.3;
const SPAWN_AHEAD = 26;
const PRUNE_BEHIND = 3;

export class World {
  constructor() {
    this.lanes = LANES;
    this.track = new Track(LANES);
    this.roller = new Roller();
    this.reset();
  }

  reset() {
    this.roller.reset(Math.floor(LANES / 2), 0);
    this.coins = 0;
    this.crashed = false;
    this._latch = false;
    this._flick = null;
    this._forwardRolls = 1;
    this.track.reset();
  }

  get lane() {
    return this.roller.col;
  }

  get row() {
    return this.roller.row;
  }

  get roll() {
    return this.roller.roll;
  }

  get distance() {
    return -this.row;
  }

  center() {
    return this.roller.center();
  }

  step(frame, dt) {
    if (this.crashed) return;

    this._captureFlick(frame, dt);
    this._yieldToFlick();

    const landed = this.roller.advance(dt);
    if (landed) this._land(landed);
    if (!this.roller.rolling) this._nextRoll(frame);

    this.track.ensure(this.row - SPAWN_AHEAD);
    this.track.prune(this.row + PRUNE_BEHIND);
  }

  pace(frame) {
    const tilt = tiltSign(frame.y);
    if (tilt < 0) return "boost";
    if (tilt > 0) return "slow";
    return "normal";
  }

  _captureFlick(frame, dt) {
    if (this._flick && (this._flick.age += dt) > FLICK_TTL) this._flick = null;

    if (Math.abs(frame.x) < RELEASE) {
      this._latch = false;
      return;
    }
    if (tiltSign(frame.x) && !this._latch) {
      this._latch = true;
      this._flick = { side: Math.sign(frame.x), age: 0 };
    }
  }

  _yieldToFlick() {
    const roll = this.roller.roll;
    if (!this._flick || !roll || roll.dx) return;

    const lane = this.lane + this._flick.side;
    if (lane < 0 || lane >= LANES) return;

    if (this.roller.progress() < REVERT_LIMIT) roll.revert = true;
    else roll.rush = true;
  }

  _nextRoll(frame) {
    const queued = this._flick?.side ?? 0;
    this._flick = null;
    const held = tiltSign(frame.x);
    const side = queued || (this._forwardRolls >= 1 ? held : 0);

    if (side) {
      const lane = this.lane + side;
      if (lane >= 0 && lane < LANES) {
        this.roller.start(side, 0, LATERAL_DURATION);
        return;
      }
    }
    this.roller.start(0, -1, FORWARD_DURATION[this.pace(frame)]);
  }

  _land(landed) {
    this._forwardRolls = landed.dx ? 0 : this._forwardRolls + 1;

    const item = this.track.at(this.row, this.lane);
    if (item === "coin") {
      this.coins += 1;
      this.track.take(this.row, this.lane);
    } else if (item === "block") {
      this.crashed = true;
    }
  }
}
