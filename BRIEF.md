# HaiClaude — Project Brief

Native Haiku GUI launcher for Claude Code (the Anthropic CLI).
Author: David Masson

## What it does

Presents a small window with two launch modes and a working directory picker:

- **Cloud** — launches `claude` in a Terminal (uses the user's
  `ANTHROPIC_API_KEY` from the environment).
- **API** — launches `claude` with custom API settings. Allows overriding
  `ANTHROPIC_BASE_URL` and `ANTHROPIC_API_KEY`, and setting per-model overrides
  for `ANTHROPIC_DEFAULT_OPUS_MODEL`, `ANTHROPIC_DEFAULT_SONNET_MODEL`, and
  `ANTHROPIC_DEFAULT_HAIKU_MODEL`. Also allows selecting a specific model to pass
  to `--model`. The API URL and key are saved across sessions.
- **Working directory** — a persistent text field (with a **Browse…** button
  that opens a directory panel) sets the directory Claude Code starts in.
  If non-empty, the launch command is prefixed with `cd '<dir>' &&`.
  The field defaults to `/boot/home` and is saved across sessions.

## Files

| File | Purpose |
|---|---|
| `main.cpp` | `BApplication` entry point |
| `LauncherWindow.h/cpp` | Single window — all UI and logic |
| `installer_main.cpp` | Installer `BApplication` entry point |
| `InstallerWindow.h/cpp` | Installer window — npm/Claude Code setup logic |
| `Makefile` | Build rules |

## Build

```
make
./haiclaude
```

Requires Haiku R1β5 or later (64-bit). Links: `-lbe -lroot -ltracker -ltranslation`.
Built with Haiku private headers (`-I/boot/system/develop/headers/private`).

## Claude Code version

Pinned to **`@anthropic-ai/claude-code@2.1.112`** — the last pure-JavaScript npm
release. Newer versions ship a native binary that does not support Haiku. The
installer enforces this version; do not upgrade until Haiku support is confirmed.

**Runtime prerequisite:** Claude Code requires the `nlohmann_json` system package
(`pkgman install nlohmann_json`). This is a dependency of Claude Code at runtime,
not a build dependency of HaiClaude.

## Key implementation notes
- **Show/Hide resize** requires `InvalidateLayout(true)` +
  `SetSizeLimits(w,w,h,h)` before `ResizeTo`, because
  `B_AUTO_UPDATE_SIZE_LIMITS` caches stale limits.
- **Sentinel arithmetic is a bug**: `B_USE_DEFAULT_SPACING = -1002`.
  Never do `B_USE_DEFAULT_SPACING + N` in layout calls — pass sentinels
  as-is.
- **Terminal launch**: uses `fork+execl` on the Terminal binary directly
  (obtained via `be_roster->FindApp` → `BEntry` → `BPath`). Using
  `be_roster->Launch()` causes Terminal to open two windows (one from
  `B_READY_TO_RUN`, one from `B_ARGV_RECEIVED`).
- **Terminal reuse when run from a shell**: if `isatty(STDIN_FILENO)`,
  the command is stored in `gPendingExec` and executed via `execl` after
  `BApplication::Run()` returns, reusing the existing terminal.
- **Directory picker**: `BFilePanel` (from `libtracker`) configured with
  `B_DIRECTORY_NODE`. The selected path arrives as `B_REFS_RECEIVED`.

## Paths

- Claude binary: `/boot/home/.npm-global/bin/claude`
- Terminal signature: `application/x-vnd.Haiku-Terminal`
