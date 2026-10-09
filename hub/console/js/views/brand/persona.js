import { el } from "../../core/dom.js";
import { fieldInput, switchControl } from "../../components/controls.js";
import { settingRow } from "../../components/rows.js";
import { readFields } from "../../components/schema.js";
import { FLASH_HINT, wireDeviceCard } from "./brand.js";

export function personaCard(manifest, onDirty) {
  const card = el("div", "set card");
  const persona = (manifest.device || {}).persona || {};

  card.append(settingRow("Name", `Status bar + about screen. ${FLASH_HINT}`,
                         fieldInput("persona_name", {}, persona.name, { ctl: true })));
  card.append(settingRow("About", `Shown on the about screen. ${FLASH_HINT}`,
                         fieldInput("persona_about", {}, persona.about, { ctl: true })));
  card.append(settingRow("Avatar",
                         "Brand-relative path to a 6-expression PNG strip.",
                         fieldInput("persona_avatar", { default: "persona.png" },
                                    persona.avatar, { ctl: true })));

  let asMascot = !!persona.mascot;
  const mascotSwitch = switchControl(asMascot, () => {
    asMascot = !asMascot;
    mascotSwitch.classList.toggle("on", asMascot);
    onDirty();
  });
  card.append(settingRow("As mascot",
                         `Offer avatar in the mascot picker. ${FLASH_HINT}`,
                         mascotSwitch));

  return wireDeviceCard(card, {
    saveLabel: "Save persona",
    onDirty,
    apply(out) {
      const flat = readFields(card);
      const name = (flat.persona_name || "").trim();
      const about = (flat.persona_about || "").trim();
      const avatar = (flat.persona_avatar || "").trim();
      if (name && avatar) {
        out.persona = { name, ...(about && { about }), avatar, mascot: asMascot };
      } else {
        delete out.persona;
      }
    },
  });
}
