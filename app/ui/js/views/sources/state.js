import { api } from "../../core/api.js";
import { editHold } from "../../core/hold.js";

export const expanded = new Set();

const guard = editHold();
export const holdEdits = guard.hold;
export const isHeld = guard.isHeld;
export const releaseHold = guard.release;

let typesCache = null;
export async function sourceTypes() {
  if (!typesCache) typesCache = await api("GET", "/api/sources/types");
  return typesCache;
}

let reloader = () => {};
export const setReloader = (fn) => { reloader = fn; };
export const reload = (force) => reloader(force);
