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
    return HaiClaudeApp().Run();
}
