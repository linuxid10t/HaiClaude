# HaiClaude — Claude Code Notes

## Build

```sh
make                  # builds both haiclaude and haiclaude-installer
make haiclaude        # launcher only
make haiclaude-installer  # installer only
```

Compiler: g++ with `-std=c++17`. Links: `-lbe -lroot -ltracker -ltranslation`.
Icon resources are attached with `xres -o <binary> icons.rsrc` after linking.

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
- **Terminal launch:** uses `fork+execl` on the Terminal binary (found via `be_roster->FindApp` → `BEntry` → `BPath`). `be_roster->Launch()` opens two windows.
- **Terminal reuse:** if `isatty(STDIN_FILENO)`, stores command in `gPendingExec` and runs `execl` after `BApplication::Run()` returns.
- **Directory picker:** `BFilePanel` with `B_DIRECTORY_NODE`; selected path arrives as `B_REFS_RECEIVED`.
- **Shell escape:** all user input is wrapped with single-quote escaping (`shellEscape()` in LauncherWindow.cpp) before use in shell commands.
- **API key backup:** in API mode, `.credentials.json` is moved aside and restored via shell `trap` to guarantee restoration even if claude crashes.
