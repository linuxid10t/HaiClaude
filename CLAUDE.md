# HaiClaude — Claude Code Notes

## Build

```sh
make                  # builds both haiclaude and haiclaude-installer
make haiclaude        # launcher only
make haiclaude-installer  # installer only
```

Compiler: g++ with `-std=c++17`. Links: `-lbe -lroot -ltracker -ltranslation`.
Icon resources are attached with `xres -o <binary> icons.rsrc` after linking.
Built with Haiku private headers (`-I/boot/system/develop/headers/private`).

## Claude Code version

**Pinned to `@anthropic-ai/claude-code@2.1.112`.** This is the last version
distributed as a pure-JavaScript npm package (`bin → cli.js`). Newer versions
use a native binary that has no Haiku build. Do not change the version until
Haiku support is officially available.

Install command (what the installer runs, and what works manually):
```sh
npm install -g @anthropic-ai/claude-code@2.1.112
```
Do **not** add `--save-exact` or `--force` — these flags cause npm to resolve
to a different (native-binary) variant of the package that cannot run on Haiku.

**Runtime prerequisite:** Claude Code requires the `nlohmann_json` system package
(`pkgman install nlohmann_json`). This is a runtime dependency of Claude Code,
not a build dependency of HaiClaude — the launcher source never `#include`s it.

## Auto-updater must be disabled

Claude Code has a built-in auto-updater. On first run after install, it silently
replaces the pinned 2.1.112 pure-JS install with the latest version (native
binary, no Haiku build). Symptom: works once after install, then fails on every
subsequent run with `claude native binary not installed`.

The launcher sets `DISABLE_AUTOUPDATER=1` in the environment before invoking
claude — both in the isatty=true exec path and in the Terminal-spawn wrapper
script. Do not remove this.

To recover a broken install:
```sh
npm uninstall -g @anthropic-ai/claude-code
npm install -g @anthropic-ai/claude-code@2.1.112
readlink ~/.npm-global/bin/claude  # must show cli.js, not claude.exe
```

## Installer behaviour

`InstallerWindow.cpp` runs these steps in order:
1. `which npm` — if missing, runs `pkgman install -y npm nodejs20`
2. `/boot/home/.npm-global/bin/claude --version | grep 2.1.112` — if found, done
3. `mkdir -p ~/.npm-global && npm config set prefix ~/.npm-global && npm install -g @anthropic-ai/claude-code@2.1.112`
4. Writes `/etc/profile.d/npm-global.sh` (idempotent) to add `~/.npm-global/bin` to PATH for all future terminals

## Key paths

| Path | Purpose |
|---|---|
| `/boot/home/.npm-global/bin/claude` | Claude Code executable (symlink → cli.js) |
| `/etc/profile.d/npm-global.sh` | PATH snippet written by installer |
| `~/.config/settings/HaiClaude` | Persistent launcher settings (BMessage flatfile) |

## Haiku-specific implementation notes

- **Show/hide resize:** call `InvalidateLayout(true)` + `SetSizeLimits(w,w,h,h)` before `ResizeTo` — `B_AUTO_UPDATE_SIZE_LIMITS` caches stale limits.
- **Sentinel arithmetic is a bug:** `B_USE_DEFAULT_SPACING = -1002`. Never do `B_USE_DEFAULT_SPACING + N` in layout calls.
- **Terminal launch:** uses `fork+execl` on the Terminal binary (found via `be_roster->FindApp` → `BEntry` → `BPath`). `be_roster->Launch()` opens two windows. The Terminal is given a temp script under `~/.cache/haiclaude-launch.sh` that sources `/etc/profile.d/npm-global.sh`, exports `DISABLE_AUTOUPDATER=1`, runs the command, and pauses on non-zero exit so startup errors are visible.
- **Terminal reuse:** if `isatty(STDIN_FILENO)`, stores command in `gPendingExec` (prepended with `DISABLE_AUTOUPDATER=1 `) and runs `execl` after `BApplication::Run()` returns.
- **Directory picker:** `BFilePanel` with `B_DIRECTORY_NODE`; selected path arrives as `B_REFS_RECEIVED`.
- **Shell escape:** all user input is wrapped with single-quote escaping (`shellEscape()` in LauncherWindow.cpp) before use in shell commands.
- **API key backup:** in API mode, `.credentials.json` is moved aside and restored via shell `trap` to guarantee restoration even if claude crashes.
