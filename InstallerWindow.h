#pragma once

#include <Button.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TextView.h>
#include <Window.h>

class BLayoutItem;

extern BString gPendingExec;

// Message constants
static const uint32 MSG_INSTALL_CLICKED     = 'iINS';
static const uint32 MSG_CLOSE_CLICKED       = 'iCLO';
static const uint32 MSG_CHECK_CLAUDE_DONE   = 'iCCD';
static const uint32 MSG_CHECK_NPM_DONE      = 'iCND';
static const uint32 MSG_INSTALL_NPM_DONE    = 'iIND';
static const uint32 MSG_INSTALL_CLAUDE_DONE = 'iICD';
static const uint32 MSG_LOG_APPEND          = 'iLOG';

// Installation states
enum InstallState {
    StateIdle,
    StateCheckingClaude,
    StateCheckingNpm,
    StateInstallingNpm,
    StateInstallingClaude,
    StateSuccess,
    StateError
};

class InstallerWindow : public BWindow {
public:
                        InstallerWindow();
    void                MessageReceived(BMessage* msg) override;

private:
    void                _StartInstallation();
    void                _CheckClaudeInstalled();
    void                _CheckNpmInstalled();
    void                _InstallNpm();
    void                _InstallClaude();
    void                _LogAppend(const BString& text);
    void                _SetState(InstallState state);
    void                _UpdateUI();
    void                _SpawnCommand(const char* cmd, uint32 doneMsg);

    BStringView*        fStatusLabel;
    BStringView*        fDetailLabel;
    BTextView*          fLogView;
    BScrollView*        fLogScroll;
    BButton*            fInstallBtn;
    BButton*            fCloseBtn;

    InstallState        fState;
    bool                fClaudeInstalled;
    bool                fNpmInstalled;
};
