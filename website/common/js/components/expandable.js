export function initExpandables() {
  document.querySelectorAll(".expandable > .expand-btn").forEach((btn) => {
    btn.addEventListener("click", () => {
      const collapsed = btn.parentElement.toggleAttribute("data-collapsed");
      btn.textContent = collapsed ? "Expand" : "Collapse";
      btn.setAttribute("aria-expanded", String(!collapsed));
    });
  });
}
