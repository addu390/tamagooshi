const SAFE_ROWS = 6;
const OBSTACLE_P = 0.38;
const SECOND_OBSTACLE_P = 0.3;
const COIN_RUN_P = 0.16;

export class Track {
  constructor(lanes) {
    this.lanes = lanes;
    this.reset();
  }

  reset() {
    this.items = new Map();
    this._generated = -SAFE_ROWS;
    this._coinRun = null;
  }

  ensure(minRow) {
    while (this._generated > minRow) {
      this._generated -= 1;
      this._spawnRow(this._generated);
    }
  }

  prune(maxRow) {
    for (const key of this.items.keys()) {
      if (Number(key.split(":")[0]) > maxRow) this.items.delete(key);
    }
  }

  at(row, lane) {
    return this.items.get(`${row}:${lane}`);
  }

  take(row, lane) {
    this.items.delete(`${row}:${lane}`);
  }

  *visible(rowMin, rowMax) {
    for (const [key, type] of this.items) {
      const [row, lane] = key.split(":").map(Number);
      if (row >= rowMin && row <= rowMax) yield { row, lane, type };
    }
  }

  _spawnRow(row) {
    const blocked = new Set();
    if (row % 2 === 0) {
      if (Math.random() < OBSTACLE_P) blocked.add(this._randLane());
      if (blocked.size && Math.random() < SECOND_OBSTACLE_P) blocked.add(this._randLane());
    }
    for (const lane of blocked) this.items.set(`${row}:${lane}`, "block");

    if (this._coinRun) {
      if (!blocked.has(this._coinRun.lane)) {
        this.items.set(`${row}:${this._coinRun.lane}`, "coin");
      }
      if (--this._coinRun.left <= 0) this._coinRun = null;
    } else if (Math.random() < COIN_RUN_P) {
      const lane = this._randLane();
      if (!blocked.has(lane)) this.items.set(`${row}:${lane}`, "coin");
      this._coinRun = { lane, left: 2 + Math.floor(Math.random() * 3) };
    }
  }

  _randLane() {
    return Math.floor(Math.random() * this.lanes);
  }
}
