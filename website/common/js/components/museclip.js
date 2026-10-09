// The hero Muse clip plays on click. While it plays, the rotor and device carousel hold still.
export function initMuseClip() {
  const video = document.querySelector(".hero-jolly");
  if (!video) return;

  const hold = (on) => window.dispatchEvent(new CustomEvent("tama:hold", { detail: on }));
  const play = () => {
    if (!video.paused) return;
    video.currentTime = 0;
    video.play().catch(() => {});
  };

  video.addEventListener("click", play);
  video.addEventListener("keydown", (e) => {
    if (e.key === "Enter" || e.key === " ") { e.preventDefault(); play(); }
  });
  video.addEventListener("play", () => hold(true));
  video.addEventListener("pause", () => hold(false));
  video.addEventListener("ended", () => { video.currentTime = 0; });
}
