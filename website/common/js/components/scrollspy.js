export function initScrollSpy() {
  const links = [...document.querySelectorAll(".side a")];
  const map = new Map();
  links.forEach((a) => {
    const id = a.getAttribute("href").slice(1);
    const sec = document.getElementById(id);
    if (sec) map.set(sec, a);
  });

  const obs = new IntersectionObserver(
    (entries) => {
      entries.forEach((en) => {
        if (en.isIntersecting) {
          links.forEach((l) => l.classList.remove("active"));
          map.get(en.target)?.classList.add("active");
        }
      });
    },
    { rootMargin: "-84px 0px -70% 0px", threshold: 0 }
  );

  map.forEach((_, sec) => obs.observe(sec));
}
