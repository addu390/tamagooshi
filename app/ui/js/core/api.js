import { $ } from "./dom.js";

export async function api(method, path, body) {
  const r = await fetch(path, {
    method,
    headers: body !== undefined ? { "Content-Type": "application/json" } : {},
    body: body !== undefined ? JSON.stringify(body) : undefined,
  });

  const data = await r.json().catch(() => ({}));
  if (!r.ok) throw new Error(data.detail || r.statusText);
  return data;
}

export function awaitRestart(noteId) {
  $(noteId).classList.remove("hide");

  const started = Date.now();
  const poll = setInterval(async () => {
    try {
      const r = await fetch("/healthz", { cache: "no-store" });
      if (r.ok && Date.now() - started > 2000) {
        clearInterval(poll);
        location.reload();
      }
    } catch {}

    if (Date.now() - started > 30000) clearInterval(poll);
  }, 1000);
}

export async function mutate(fn, noteId) {
  try {
    await fn();
    awaitRestart(noteId);
  } catch (err) {
    alert(err.message);
  }
}
