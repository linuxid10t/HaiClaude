# HaiClaude — Project Brief

Native Haiku GUI launcher for Claude Code (the Anthropic CLI).
Author: David Masson

## What it does

Presents a small window with two launch modes:

- **Cloud** — launches `claude` directly in a Terminal (uses the user's
  `ANTHROPIC_API_KEY` from the environment).
- **Local** — launches `claude` with `ANTHROPIC_BASE_URL` and
  `ANTHROPIC_API_KEY=ollama` set, targeting a local OpenAI-compatible
  server (Ollama, LM Studio, etc.). A **Refresh** button fetches the
  server's model list via `GET <baseUrl>/models` and populates a dropdown.

## Files

| File | Purpose |
|---|---|
| `main.cpp` | `BApplication` entry point |
| `LauncherWindow.h/cpp` | Single window — all UI and logic |
| `Makefile` | Build rules |

## Build

```
make
./haiclaude
```

Requires Haiku with `libnetservices2` (present in R1β5+).

## Key implementation notes

- **HTTP fetch** runs on a background thread (`spawn_thread`) using
  `BPrivate::Network::BHttpSession`. Results are posted back to the
  window as `MSG_MODELS_READY` via `BMessenger`.
- **JSON parsing** is a simple `strstr` loop scanning for `"id":"…"` —
  no JSON library.
- **Show/Hide resize** requires `InvalidateLayout(true)` +
  `SetSizeLimits(w,w,h,h)` before `ResizeTo`, because
  `B_AUTO_UPDATE_SIZE_LIMITS` caches stale limits.
- **Sentinel arithmetic is a bug**: `B_USE_DEFAULT_SPACING = -1002`.
  Never do `B_USE_DEFAULT_SPACING + N` in layout calls — pass sentinels
  as-is.

## Paths

- Claude binary: `/boot/home/.npm-global/bin/claude`
- Terminal signature: `application/x-vnd.Haiku-Terminal`
