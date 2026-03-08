/*  installer_main.cpp – HaiClaude Installer
 *
 *  Native Haiku installer for Claude Code.
 *
 *  Build:
 *      make installer
 *
 *  Author: David Masson
 */

#include "InstallerWindow.h"

#include <Application.h>
#include <String.h>

#include <unistd.h>

BString gPendingExec;

class InstallerApp : public BApplication {
public:
    InstallerApp()
        : BApplication("application/x-vnd.DavidMasson.HaiClaudeInstaller") {}

    void ReadyToRun() override {
        (new InstallerWindow())->Show();
    }

    bool QuitRequested() override {
        return true;
    }
};

int main()
{
    InstallerApp().Run();

    if (gPendingExec.Length() > 0)
        execl("/bin/sh", "sh", "-c", gPendingExec.String(), (char*)nullptr);

    return 0;
}
