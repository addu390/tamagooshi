export function initPickRow(rowId, apply) {
  const row = document.getElementById(rowId);
  if (!row) return;

  const chips = [...row.querySelectorAll(".pickchip")];
  chips.forEach((chip) => chip.addEventListener("click", () => {
    chips.forEach((c) => c.classList.toggle("on", c === chip));
    apply(chip.dataset);
  }));
}
