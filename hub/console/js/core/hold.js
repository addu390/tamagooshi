export function editHold() {
  let held = false;
  return {
    hold: () => { held = true; },
    release: () => { held = false; },
    isHeld: () => held,
  };
}
