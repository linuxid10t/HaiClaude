# HaiClaude

Native Haiku GUI launcher for [Claude Code](https://docs.anthropic.com/en/docs/claude-code) (the Anthropic CLI).

## Features

- **Cloud mode** — launches Claude Code in a Terminal using your `ANTHROPIC_API_KEY`.
- **Local mode** — launches Claude Code against a local OpenAI-compatible server (Ollama, LM Studio, etc.) with:
  - Configurable **Base URL** (default: `http://localhost:11434/v1`)
  - **Model** text field — type any model name, or use **Refresh** to fetch the server's model list and pick from a dropdown
  - **Context tokens** field — sets `OLLAMA_NUM_CTX` and `LM_STUDIO_NUM_CTX` so the server loads the configured context window (minimum 32768, default 32768)
  - Isolated config dir (`~/.claude-local`) so local and cloud credentials don't conflict
- **Working directory** picker — persistent text field with a **Browse…** button; Claude Code starts in the chosen directory.
- All settings (mode, base URL, model, context tokens, working directory) are saved across sessions.

## Screenshot

![HaiClaude window in Local mode](screenshot.png)

## Requirements

- Haiku R1β5 or later (requires `libnetservices2`)
- [Claude Code](https://docs.anthropic.com/en/docs/claude-code) installed at `/boot/home/.npm-global/bin/claude`

## Build

```sh
make
./haiclaude
```

Links against: `-lbe -lroot -lnetservices2 -lbnetapi -ltracker`

## Install

Copy the `haiclaude` binary anywhere on your path, or add it to the Deskbar.

## License

MIT — see [LICENSE](LICENSE).
