# HaiClaude

Native Haiku GUI launcher for [Claude Code](https://docs.anthropic.com/en/docs/claude-code) (the Anthropic CLI).

## Features

- **Cloud mode** — launches Claude Code using OAuth or your existing `ANTHROPIC_API_KEY` environment variable.
  - **Model selection** — choose between Opus, Sonnet, or Haiku; selector appears under Cloud and hides when API mode is active
- **API mode** — launches Claude Code with a custom API configuration:
  - **API URL** — defaults to Anthropic's API, but can be changed for proxies or alternative endpoints
  - **API Key** — your Anthropic API key (input is masked)
  - **Model overrides** — optionally override the default Opus, Sonnet, and Haiku model versions
  - **Current model override** — force a specific model via `--model`
- **Working directory** picker — persistent text field with a **Browse…** button; Claude Code starts in the chosen directory
- **Persistent settings** — all settings (mode, model, directory, API configuration) are saved across sessions; defaults to Cloud mode on first run

## Screenshot

![HaiClaude window](screenshot.png)

## Requirements

- Haiku R1β5 or later
- [Claude Code](https://docs.anthropic.com/en/docs/claude-code) installed at `/boot/home/.npm-global/bin/claude`

## Build

```sh
make
./haiclaude
```

Links against: `-lbe -lroot -lbnetapi -ltracker`

## Install

Copy the `haiclaude` binary anywhere on your path, or add it to the Deskbar.

## License

MIT — see [LICENSE](LICENSE).
