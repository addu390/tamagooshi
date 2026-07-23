export function initScrollSpy() {
  const links = [...document.querySelectorAll(".side a")];
  const sections = links
    .map((a) => [document.getElementById(a.getAttribute("href").slice(1)), a])
    .filter(([sec]) => sec);
  if (!sections.length) return;

  const setActive = (link) =>
    links.forEach((l) => l.classList.toggle("active", l === link));

  const update = () => {
    const atBottom =
      window.innerHeight + window.scrollY >=
      document.documentElement.scrollHeight - 2;
    if (atBottom) return setActive(sections[sections.length - 1][1]);
    const line = 84 + (window.innerHeight - 84) * 0.3;
    let current = sections[0][1];
    for (const [sec, a] of sections) {
      if (sec.getBoundingClientRect().top <= line) current = a;
    }
    setActive(current);
  };

  document.addEventListener("scroll", update, { passive: true });
  window.addEventListener("resize", update);
  update();
}
