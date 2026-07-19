import { api, awaitRestart } from "../../core/api.js";
import { $, el, showResult } from "../../core/dom.js";
import { formActions, panelHead, secHead, settingRow, typeTile } from "../../components/rows.js";
import { fieldInput, pollingControl } from "../../components/controls.js";
import { connectionProps, credentialProps, readFields, resolveItems } from "../../components/schema.js";
import { queryFields } from "./card.js";

export function openWizard(types) {
  const wizard = $("wizard");
  wizard.classList.remove("hide");
  $("add-source").classList.add("hide");
  const close = () => {
    wizard.classList.add("hide");
    $("add-source").classList.remove("hide");
  };

  function showCatalog() {
    wizard.replaceChildren();
    wizard.append(panelHead("Choose a source", "Where should this brand pull its metrics from?"));

    const grid = el("div", "type-grid");
    for (const t of types) {
      const pick = el("button", "type-card");
      pick.append(typeTile(t.type, t.label, true));

      const text = el("div", "type-text");
      text.append(el("div", "type-name", t.label || t.type));
      text.append(el("div", "type-desc", t.description || ""));
      pick.append(text);

      pick.addEventListener("click", () => showForm(t));
      grid.append(pick);
    }
    wizard.append(grid);

    const cancel = el("button", "btn", "Cancel");
    cancel.addEventListener("click", close);
    wizard.append(formActions([cancel]));
  }

  function showForm(t) {
    wizard.replaceChildren();

    const head = el("div", "wiz-head");
    const back = el("button", "btn", "‹ Back");
    back.addEventListener("click", showCatalog);
    head.append(back, typeTile(t.type, t.label));

    const text = el("div", "type-text");
    text.append(el("div", "type-name", t.label || t.type));
    text.append(el("div", "type-desc", t.description || ""));
    head.append(text);
    wizard.append(head);

    const form = el("div", "wiz-form");
    const props = t.schema.properties || {};
    const itemSchema = resolveItems(t.schema, props.metrics || {});

    form.append(secHead("Settings"));

    const settings = el("div", "sec");
    settings.append(settingRow("Poll every", "How often queries run while the source is on.",
                               pollingControl({})));
    for (const [name, prop] of connectionProps(t.schema)) {
      settings.append(settingRow(name.replaceAll("_", " "), null,
                                 fieldInput(name, prop, undefined, { ctl: true })));
    }
    form.append(settings);

    const credentials = credentialProps(t.schema);
    if (credentials.length) {
      form.append(secHead("Credentials"));

      const creds = el("div", "sec");
      for (const [name, prop] of credentials) {
        const input = el("input", "field ctl secret-value");
        input.type = "password";
        input.dataset.env = prop.default || name.toUpperCase();

        creds.append(settingRow(name.replace("_env", "").replaceAll("_", " "),
                                "Stored in this app's data folder, never in the brand file.",
                                input));
      }
      form.append(creds);
    }

    const addQuery = el("button", "ghost", "+ Add query");
    form.append(secHead("Queries", addQuery));

    const queriesWrap = el("div", "qlist");
    form.append(queriesWrap);

    const appendQuery = () => {
      const box = el("div", "qedit wiz-query");
      box.append(queryFields(itemSchema, {}));

      const drop = el("button", "row-x", "×");
      drop.addEventListener("click", () => box.remove());
      box.append(drop);

      queriesWrap.append(box);
    };
    appendQuery();
    addQuery.addEventListener("click", appendQuery);
    wizard.append(form);

    const result = el("p", "form-result hide");

    async function collect() {
      for (const input of wizard.querySelectorAll("input.secret-value")) {
        if (input.value) {
          await api("PUT", "/api/secrets", { name: input.dataset.env, value: input.value });
        }
      }

      const config = { type: t.type, ...readFields(settings) };
      config.metrics = [...queriesWrap.querySelectorAll(".qedit")]
        .map((box) => readFields(box))
        .filter((m) => Object.keys(m).length);
      return config;
    }

    function report(text, isError) {
      showResult(result, text, isError);
    }

    const cancel = el("button", "btn", "Cancel");
    cancel.addEventListener("click", close);

    const test = el("button", "btn", "Test connection");
    test.addEventListener("click", async () => {
      report("Testing…");
      try {
        const { metrics } = await api("POST", "/api/sources/test", await collect());
        report("Looks good: " + metrics.map((m) => `${m.key} = ${m.value}`).join(", "));
      } catch (err) { report(err.message, true); }
    });

    const save = el("button", "btn primary", "Add source");
    save.addEventListener("click", async () => {
      try {
        await api("POST", "/api/sources", await collect());
        close();
        awaitRestart("sources-note");
      } catch (err) { report(err.message, true); }
    });

    wizard.append(result, formActions([cancel, test, save]));
  }

  showCatalog();
}
