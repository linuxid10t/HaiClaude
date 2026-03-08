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
#include <AppFileInfo.h>
#include <File.h>
#include <Path.h>
#include <Resources.h>
#include <Roster.h>
#include <TranslationUtils.h>
#include <Bitmap.h>
#include <String.h>
#include <IconUtils.h>

#include <unistd.h>

BString gPendingExec;

class HaiClaudeApp : public BApplication {
public:
    HaiClaudeApp()
        : BApplication("application/x-vnd.DavidMasson.HaiClaude") {}

    void ReadyToRun() override {
        SetAppIcon();
        (new LauncherWindow())->Show();
    }

    bool QuitRequested() override {
        return true;
    }

private:
    void SetAppIcon() {
        // Find our executable path
        app_info info;
        if (GetAppInfo(&info) != B_OK)
            return;

        BFile file(&info.ref, B_READ_WRITE);
        if (file.InitCheck() != B_OK)
            return;

        // Load PNG from resources
        BResources* res = AppResources();
        if (!res)
            return;

        size_t size;
        const void* data = res->LoadResource('PNG ', 101, &size);
        if (!data)
            return;

        // Decode PNG to bitmap
        BMemoryIO memIO(data, size);
        BBitmap* bitmap = BTranslationUtils::GetBitmap(&memIO);
        if (!bitmap)
            return;

        int srcW = bitmap->Bounds().IntegerWidth() + 1;
        int srcH = bitmap->Bounds().IntegerHeight() + 1;
        uint8* srcBits = (uint8*)bitmap->Bits();
        int srcBPR = bitmap->BytesPerRow();

        // Create scaled icons in RGBA32 first
        BBitmap* largeRGBA = new BBitmap(BRect(0, 0, 31, 31), B_RGBA32);
        BBitmap* miniRGBA = new BBitmap(BRect(0, 0, 15, 15), B_RGBA32);

        // Scale source to 32x32
        uint8* dstBits = (uint8*)largeRGBA->Bits();
        int dstBPR = largeRGBA->BytesPerRow();
        for (int y = 0; y < 32; y++) {
            for (int x = 0; x < 32; x++) {
                int sx = (x * srcW) / 32;
                int sy = (y * srcH) / 32;
                uint8* src = srcBits + sy * srcBPR + sx * 4;
                uint8* dst = dstBits + y * dstBPR + x * 4;
                dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
            }
        }

        // Scale to 16x16
        dstBits = (uint8*)miniRGBA->Bits();
        dstBPR = miniRGBA->BytesPerRow();
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 16; x++) {
                int sx = (x * srcW) / 16;
                int sy = (y * srcH) / 16;
                uint8* src = srcBits + sy * srcBPR + sx * 4;
                uint8* dst = dstBits + y * dstBPR + x * 4;
                dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
            }
        }

        // Convert to CMAP8 for SetIcon
        BBitmap* largeIcon = new BBitmap(BRect(0, 0, 31, 31), B_CMAP8);
        BBitmap* miniIcon = new BBitmap(BRect(0, 0, 15, 15), B_CMAP8);

        BIconUtils::ConvertToCMAP8((uint8*)largeRGBA->Bits(), 32, 32, largeRGBA->BytesPerRow(), largeIcon);
        BIconUtils::ConvertToCMAP8((uint8*)miniRGBA->Bits(), 16, 16, miniRGBA->BytesPerRow(), miniIcon);

        // Set icons on the executable
        BAppFileInfo appInfo(&file);
        appInfo.SetIcon(largeIcon, B_LARGE_ICON);
        appInfo.SetIcon(miniIcon, B_MINI_ICON);

        delete bitmap;
        delete largeRGBA;
        delete miniRGBA;
        delete largeIcon;
        delete miniIcon;
    }
};

int main()
{
    HaiClaudeApp().Run();

    if (gPendingExec.Length() > 0)
        execl("/bin/sh", "sh", "-c", gPendingExec.String(), (char*)nullptr);

    return 0;
}
