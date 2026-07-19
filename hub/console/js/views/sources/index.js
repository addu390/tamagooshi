import { api } from "../../core/api.js";
import { $, renderList } from "../../core/dom.js";
import { isHeld, releaseHold, setReloader, sourceTypes } from "./state.js";
import { sourceCard } from "./card.js";
import { openWizard } from "./wizard.js";

async function loadSources(force) {
  if (isHeld() && !force) return;
  if (force) releaseHold();

  try {
    const [sources, types] = await Promise.all([api("GET", "/api/sources"), sourceTypes()]);
    renderList($("source-list"), $("sources-empty"), sources,
               (s) => sourceCard(s, types.find((t) => t.type === s.type)));
  } catch {}
}

setReloader(loadSources);

function init() {
  $("add-source").addEventListener("click", async () => openWizard(await sourceTypes()));
}

export default {
  id: "sources",
  title: "Sources",
  init,
  load: () => null,
  render: () => loadSources(),
};
