# HaiClaude

Native Haiku GUI launcher for [Claude Code](https://docs.anthropic.com/en/docs/claude-code) (the Anthropic CLI).

## Features

- **Cloud mode** — launches Claude Code using OAuth or your existing `ANTHROPIC_API_KEY` environment variable.
  - **Model selection** — choose between Opus, Sonnet, or Haiku; selector appears under Cloud and hides when API mode is active
- **API mode** — launches Claude Code with a custom API configuration:
  - **API URL** — defaults to Anthropic's API, but can be changed for proxies or alternative endpoints
  - **API Key** — your Anthropic API key (input is masked)
  - **Remember key** — opt-in checkbox to save the API key to disk
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

## Build

```sh
make              # build both launcher and installer
```

Or build individually:
```sh
make haiclaude            # build launcher only
make haiclaude-installer  # build installer only
```

Links against: `-lbe -lroot -ltracker -ltranslation`
- Built with Haiku private headers (`-I/boot/system/develop/headers/private`)

## Claude Code Setup

### Using the Installer

After building, run the installer which will automatically check for and install Claude Code if needed:

```sh
./haiclaude-installer
```

The installer will:
1. Check if npm is available
2. Install npm if needed (via `pkgman install npm nodejs20`)
3. Check if Claude Code 2.1.112 is already installed
4. Install `@anthropic-ai/claude-code@2.1.112` globally via npm if needed
5. Write `/etc/profile.d/npm-global.sh` so every new terminal has `~/.npm-global/bin` on PATH automatically

> **Version note:** Claude Code is pinned to **2.1.112**. This is the last pure-JavaScript npm release; newer versions use a native binary that does not support Haiku. Do not upgrade until Haiku support is confirmed.

> **Runtime prerequisite:** Claude Code requires the `nlohmann_json` system package. Install it with `pkgman install nlohmann_json`. This is a runtime dependency of Claude Code, not a build dependency of HaiClaude.

### Manual Setup

If you prefer to install Claude Code manually:

1. Install npm: `pkgman install npm nodejs20`
2. Configure the prefix: `npm config set prefix ~/.npm-global`
3. Install Claude Code: `npm install -g @anthropic-ai/claude-code@2.1.112`
4. Add to PATH: `echo 'export PATH="$HOME/.npm-global/bin:$PATH"' >> /etc/profile.d/npm-global.sh`

## Usage

Run the launcher:

```sh
./haiclaude
```

Select your preferred mode (Cloud or API), choose a model, pick a working directory, and click **Launch**.

## Security Notes

- **API key storage is opt-in** — the key is only saved to disk if you check "Remember key"
- **Shell injection protection** — all user input is properly escaped before use in shell commands
- **Input validation** — working directory must exist; API fields cannot be empty
- **Credentials backup** — in API mode, existing credentials are backed up and restored automatically (using shell `trap` to guarantee restoration even on crash)

## License

MIT — see [LICENSE](LICENSE).
