/*  main.cpp – HaiClaude
 *
 *  Native Haiku launcher for Claude Code.
 *
 *  Build:
 *      make
 *
 *  Author: David Masson
 */

#include "LauncherWindow.h"

#include <Application.h>
#include <String.h>

#include <unistd.h>

BString gPendingExec;

class HaiClaudeApp : public BApplication {
public:
    HaiClaudeApp()
        : BApplication("application/x-vnd.DavidMasson.HaiClaude") {}

    void ReadyToRun() override {
        (new LauncherWindow())->Show();
    }

    bool QuitRequested() override {
        return true;
    }
};

int main()
{
    HaiClaudeApp().Run();

    if (gPendingExec.Length() > 0)
        execl("/bin/sh", "sh", "-c", gPendingExec.String(), (char*)nullptr);

    return 0;
}
