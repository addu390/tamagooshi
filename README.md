<p align="left">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/assets/images/wordmark-dark.svg">
    <img src="docs/assets/images/wordmark-light.svg" alt="Tamagooshi" height="34">
  </picture>
</p>

<p align="left"><a href="https://addu390.github.io/tamagooshi/">Docs</a> · <a href="https://addu390.github.io/tamagooshi/#config">Configure</a> · <a href="https://addu390.github.io/tamagooshi/#build">Flash from the browser</a></p>

<p align="left">Pixel-art pet for M5Stack StickC Plus and StickS3 that turns live metrics into its mood. Local hub feeds readings from metric sources such as Datadog and PostHog over BLE/MQTT. <code>config.yaml</code> selects the mascot, themes, games, and more that ship in the firmware. Follows sessions from coding agents such as Claude and Cursor, down to approving or denying their requests from the device.</p>

## Quick start

```bash
make hub        # run the hub, pairs with your device over BLE
make sim        # desktop simulator, no board needed
```

`make` lists every target.

## Hardware

M5Stack StickC Plus, StickC Plus SE, and StickS3. Flash from the browser via the [docs](https://addu390.github.io/tamagooshi/#build), or locally:

```bash
cd firmware
pio run -e m5stickc-plus -t upload   # or m5stickc-plus-se, m5sticks3
```

## License

MIT. See [LICENSE](LICENSE).
