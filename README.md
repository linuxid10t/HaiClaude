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
- **64-bit Haiku** required for the installer (npm packages are 64-bit only)
- The launcher itself works on 32-bit Haiku if Claude Code is already installed
- **GCC 13+** required when building for 32-bit

## Installation

### Using the Installer

Run the installer which will automatically check for and install Claude Code if needed:

```sh
./haiclaude-installer
```

The installer will:
1. Check if npm is available
2. Install npm if needed (via `pkgman install npm nodejs20`)
3. Check if Claude Code is already installed (looks for `~/.npm-global/bin/claude`)
4. Install Claude Code globally via npm if needed
5. Set up the `claude` binary at `~/.npm-global/bin/claude`

### Manual Installation

If you prefer to install manually:

1. Install npm: `pkgman install npm`
2. Install Claude Code: `npm install -g @anthropic-ai/claude-code`
3. Build and run the launcher

## Build

```sh
make              # build both launcher and installer
```

Or build individually:
```sh
make haiclaude            # build launcher only
make haiclaude-installer  # build installer only
```

Links against: `-lbe -lroot -ltracker`

## License

MIT — see [LICENSE](LICENSE).
