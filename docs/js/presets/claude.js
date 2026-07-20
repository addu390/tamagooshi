(function () {
  const CLAWD_ART = [
    ".#########.",
    ".#########.",
    ".##.###.##.",
    "###########",
    "###########",
    ".#########.",
    ".#.#...#.#.",
    ".#.#...#.#.",
  ];

  window.TAMA_PRESET = {
    chrome: {
      href: "claude", logo: "C", accent: true, pre: "Claude &times; ",
      note: "An independent project, not affiliated with Anthropic PBC",
      footer: "An independent, fan-made project. Claude and Clawd belong to Anthropic PBC.",
    },

    deviceName: "CLAWD",
    deviceColors: {
      sil: "#221d1a", det: "#c4bcae", grid: "#efece3",
      dimline: "#d8d2c4", dimtext: "#a2988a", call: "#b6ad9e",
      shell: "#d8d2c6", shade0: [224, 219, 208], bezel: "#e2ddd2",
      well: "#f6f4ec", btnPocket: "#c9c1b1", btnPocketDeep: "#b9b09e",
      btnCap: "#dad4c6", btnCapHi: "#eae5d9",
    },

    logo: {
      w: 24, h: 24, name: "logo.png",
      bits: "00000000000000000000080001088000998000d900085b18067e6003ffc000ff8007ffe01ffff800ff0001ffc0077ee00c7b1000db00009900010880010800000000000000000000",
    },

    agentsLabel: "CLAUDE",
    agentsSub: "your agent",
    aiCaption: "Co-work with Claude.",
    mascotArt: CLAWD_ART,
    aiMascotArt: CLAWD_ART,
    mascotSadArt: [
      ".#########.",
      ".#########.",
      ".#########.",
      "##.#####.##",
      "###########",
      ".#########.",
      ".#.#...#.#.",
      ".#.#...#.#.",
    ],

    text: {
      logoText: "CLAUDE",
      brandModeCaption: "Claude on every surface: logo, name, theme.",
      mascotCaption: "Clawd, on its home screen",
      mascotLabel: "CLAWD",
      mascotSub: "claude pack",
      themeCaption: "Terra light, clay dark",
      themeLabel: "CLAY THEME",
      themeSub: "dark terra",
      gameMascotLabel: "CLAWD IN GAME",
      gameMascotSub: "your pixel pilot",
      coworkLabel: "CO-WORK WITH CLAUDE",
      coworkCaption: "Co-work with Claude: Clawd reacts live",
    },

    config: {
      id: "claude",
      name: "CLAUDE",
      mascot: "Clawd",
      tagline: "Off the screen, on the case",
      website: "claude.com",
      packs: ["claude"],
      mascotDefault: "clawd",
      themes: ["terra", "clay"],
      themeDefault: "terra",
      typefaceDefault: "dejavu",
      customThemes: [{ name: "clay", surface: "#262624", ink: "#F0EEE6", accent: "#D97757" }],
      agents: ["claude"],
      agentDefault: "claude",
    },

    demo: {
      note: {
        title: "Everything else comes standard",
        desc: "The Claude edition is the full firmware: games, apps, metrics, alerts and theming all work as usual. These clips show what Claude adds on top.",
      },
      scenarios: [
        { id: "claude-home", title: "Clawd at home", desc: "Clawd idles on the terra home screen, wandering and blinking while it waits for work.",
          markers: [] },
        { id: "claude-work", title: "Co-work with Claude", desc: "A Claude Code session starts, Clawd heads down to work and celebrates the landed task. Then you press A, speak, and Claude answers on the device.",
          markers: [{ a: 0.29, kind: "hi" }, { a: 0.45, kind: "hi" }, { a: 0.56, kind: "warn" }, { a: 0.70, kind: "hi" }] },
        { id: "claude-approve", title: "Approve from the device", desc: "Claude asks before it acts: approve a push from the device, then deny a risky cleanup.",
          markers: [{ a: 0.08, kind: "warn" }, { a: 0.33, kind: "crit" }] },
      ],
      legends: {
        "claude-home": { h: "OUT OF THE BOX", rows: [["Mascot", "Clawd"], ["Theme", "terra"], ["Mood", "live"]] },
        "claude-work": { h: "WORKS WITH", rows: [["Claude Code", "live"], ["Voice", "press A"]] },
        "claude-approve": { h: "IN THE LOOP", rows: [["Approvals", "Claude"], ["Deny", "on device"]] },
      },
    },
  };
})();
