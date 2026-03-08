/*  InstallerWindow.cpp – HaiClaude Installer */

#include "InstallerWindow.h"

#include <Application.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <Messenger.h>
#include <Rect.h>
#include <String.h>

#include <pthread.h>
#include <cstdio>
#include <cstring>

// ---------------------------------------------------------------------------
// Command thread data
// ---------------------------------------------------------------------------

struct CommandData {
    BMessenger  messenger;
    BString     command;
    uint32      doneMsg;
};

static void*
command_thread(void* arg)
{
    CommandData* data = (CommandData*)arg;

    // Send log messages back
    BMessage logMsg(MSG_LOG_APPEND);

    FILE* fp = popen(data->command.String(), "r");
    if (!fp) {
        BMessage resultMsg(data->doneMsg);
        resultMsg.AddBool("success", false);
        resultMsg.AddString("output", "Failed to run command");
        data->messenger.SendMessage(&resultMsg);
        delete data;
        return nullptr;
    }

    BString output;
    char buf[256];
    while (fgets(buf, sizeof(buf), fp)) {
        output << buf;
        // Send each line to log view
        BMessage lineMsg(MSG_LOG_APPEND);
        lineMsg.AddString("text", buf);
        data->messenger.SendMessage(&lineMsg);
    }

    int exitCode = pclose(fp);

    BMessage resultMsg(data->doneMsg);
    resultMsg.AddBool("success", exitCode == 0);
    resultMsg.AddString("output", output);
    data->messenger.SendMessage(&resultMsg);

    delete data;
    return nullptr;
}

// ---------------------------------------------------------------------------
// InstallerWindow
// ---------------------------------------------------------------------------

InstallerWindow::InstallerWindow()
    : BWindow(BRect(100, 100, 550, 400),
              "Claude Code Installer",
              B_TITLED_WINDOW,
              B_QUIT_ON_WINDOW_CLOSE | B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS)
    , fState(StateIdle)
    , fClaudeInstalled(false)
    , fNpmInstalled(false)
{
    fStatusLabel = new BStringView("status", "Ready to install Claude Code");
    fStatusLabel->SetFontSize(14);

    fDetailLabel = new BStringView("detail", "Click Install to begin.");

    BStringView* logLabel = new BStringView("logLabel", "Installation log:");

    fLogView = new BTextView("log");
    fLogView->MakeEditable(false);

    fLogScroll = new BScrollView("logScroll", fLogView, B_FRAME_EVENTS | B_WILL_DRAW,
                                  false, true);

    fInstallBtn = new BButton("install", "Install", new BMessage(MSG_INSTALL_CLICKED));
    fInstallBtn->MakeDefault(true);

    fCloseBtn = new BButton("close", "Close", new BMessage(MSG_CLOSE_CLICKED));

    BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
        .SetInsets(B_USE_DEFAULT_SPACING)
        .Add(fStatusLabel)
        .Add(fDetailLabel)
        .Add(logLabel)
        .Add(fLogScroll, 1.0f)
        .AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
            .AddGlue()
            .Add(fInstallBtn)
            .Add(fCloseBtn)
            .End()
        .End();

    InvalidateLayout(true);
    BSize preferred = GetLayout()->PreferredSize();
    ResizeTo(preferred.width, preferred.height);
    CenterOnScreen();
}

void
InstallerWindow::MessageReceived(BMessage* msg)
{
    switch (msg->what) {
        case MSG_INSTALL_CLICKED:
            _StartInstallation();
            break;

        case MSG_CLOSE_CLICKED:
            be_app->PostMessage(B_QUIT_REQUESTED);
            break;

        case MSG_LOG_APPEND: {
            BString text;
            if (msg->FindString("text", &text) == B_OK) {
                _LogAppend(text);
            }
            break;
        }

        case MSG_CHECK_CLAUDE_DONE: {
            bool success = false;
            BString output;
            msg->FindBool("success", &success);
            msg->FindString("output", &output);

            // Check if which found the claude binary
            fClaudeInstalled = output.Length() > 0 && output.FindFirst("claude") >= 0;

            if (fClaudeInstalled) {
                _LogAppend("Claude Code is already installed!\n");
                _SetState(StateSuccess);
            } else {
                _LogAppend("Claude Code not found. Checking for npm...\n");
                _SetState(StateCheckingNpm);
                _CheckNpmInstalled();
            }
            break;
        }

        case MSG_CHECK_NPM_DONE: {
            bool success = false;
            BString output;
            msg->FindBool("success", &success);
            msg->FindString("output", &output);

            // Check if npm path was found
            fNpmInstalled = output.Length() > 0 && output.FindFirst("npm") >= 0;

            if (fNpmInstalled) {
                _LogAppend("npm is installed. Installing Claude Code...\n");
                _SetState(StateInstallingClaude);
                _InstallClaude();
            } else {
                _LogAppend("npm not found. Installing npm via pkgman...\n");
                _SetState(StateInstallingNpm);
                _InstallNpm();
            }
            break;
        }

        case MSG_INSTALL_NPM_DONE: {
            bool success = false;
            msg->FindBool("success", &success);

            if (success) {
                _LogAppend("npm installed successfully. Installing Claude Code...\n");
                _SetState(StateInstallingClaude);
                _InstallClaude();
            } else {
                _LogAppend("Failed to install npm. Please check your internet connection.\n");
                _SetState(StateError);
            }
            break;
        }

        case MSG_INSTALL_CLAUDE_DONE: {
            bool success = false;
            msg->FindBool("success", &success);

            if (success) {
                _LogAppend("\nClaude Code installed successfully!\n");
                _LogAppend("You can now close this window and use HaiClaude launcher.\n");
                _SetState(StateSuccess);
            } else {
                _LogAppend("\nFailed to install Claude Code. Check the log for details.\n");
                _SetState(StateError);
            }
            break;
        }

        default:
            BWindow::MessageReceived(msg);
    }
}

void
InstallerWindow::_StartInstallation()
{
    fInstallBtn->SetEnabled(false);
    fLogView->SetText("");
    _LogAppend("Starting installation...\n");
    _LogAppend("Checking if Claude Code is already installed...\n");

    _SetState(StateCheckingClaude);
    _CheckClaudeInstalled();
}

void
InstallerWindow::_CheckClaudeInstalled()
{
    _SpawnCommand("which claude 2>&1", MSG_CHECK_CLAUDE_DONE);
}

void
InstallerWindow::_CheckNpmInstalled()
{
    _SpawnCommand("which npm 2>&1", MSG_CHECK_NPM_DONE);
}

void
InstallerWindow::_InstallNpm()
{
    _SpawnCommand("pkgman install -y npm nodejs20 2>&1", MSG_INSTALL_NPM_DONE);
}

void
InstallerWindow::_InstallClaude()
{
    _SpawnCommand("npm install -g @anthropic-ai/claude-code 2>&1", MSG_INSTALL_CLAUDE_DONE);
}

void
InstallerWindow::_LogAppend(const BString& text)
{
    fLogView->Insert(fLogView->TextLength(), text.String(), text.Length());

    // Scroll to bottom
    fLogView->Select(fLogView->TextLength(), fLogView->TextLength());
    fLogView->ScrollToSelection();
}

void
InstallerWindow::_SetState(InstallState state)
{
    fState = state;
    _UpdateUI();
}

void
InstallerWindow::_UpdateUI()
{
    switch (fState) {
        case StateIdle:
            fStatusLabel->SetText("Ready to install Claude Code");
            fDetailLabel->SetText("Click Install to begin.");
            fInstallBtn->SetEnabled(true);
            break;

        case StateCheckingClaude:
            fStatusLabel->SetText("Checking Claude Code...");
            fDetailLabel->SetText("Verifying existing installation.");
            fInstallBtn->SetEnabled(false);
            break;

        case StateCheckingNpm:
            fStatusLabel->SetText("Checking npm...");
            fDetailLabel->SetText("Looking for npm package manager.");
            break;

        case StateInstallingNpm:
            fStatusLabel->SetText("Installing npm...");
            fDetailLabel->SetText("This may take a few minutes.");
            break;

        case StateInstallingClaude:
            fStatusLabel->SetText("Installing Claude Code...");
            fDetailLabel->SetText("Downloading and installing via npm.");
            break;

        case StateSuccess:
            fStatusLabel->SetText("Installation Complete!");
            fDetailLabel->SetText("Claude Code is ready to use.");
            fInstallBtn->SetEnabled(false);
            break;

        case StateError:
            fStatusLabel->SetText("Installation Failed");
            fDetailLabel->SetText("Check the log for error details.");
            fInstallBtn->SetEnabled(true);
            break;
    }
}

void
InstallerWindow::_SpawnCommand(const char* cmd, uint32 doneMsg)
{
    CommandData* data = new CommandData;
    data->messenger = BMessenger(this);
    data->command = cmd;
    data->doneMsg = doneMsg;

    pthread_t thread;
    pthread_create(&thread, nullptr, command_thread, data);
    pthread_detach(thread);
}
